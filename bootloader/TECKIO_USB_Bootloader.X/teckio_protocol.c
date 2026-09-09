#include "teckio_protocol.h"

#include <string.h>

#define TKBL_SOF0 0x54u
#define TKBL_SOF1 0x4Bu

static uint16_t get_u16_le(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t get_u32_le(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static void put_u16_le(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put_u32_le(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

uint32_t tkbl_crc32_update(uint32_t crc, const uint8_t *data, uint8_t length)
{
    uint8_t i;
    uint8_t bit;

    for (i = 0; i < length; ++i) {
        crc ^= data[i];
        for (bit = 0; bit < 8u; ++bit) {
            if (crc & 1UL) {
                crc = (crc >> 1) ^ 0xEDB88320UL;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static void send_reply(tkbl_context_t *ctx, uint8_t reply_command,
                       uint8_t original_command, uint8_t status,
                       uint16_t sequence, bool include_chunk)
{
    uint8_t *out = ctx->last_reply;
    uint16_t payload_length = include_chunk ? 4u : 2u;
    uint32_t crc;

    out[0] = TKBL_SOF0;
    out[1] = TKBL_SOF1;
    out[2] = TKBL_PROTOCOL_VERSION;
    out[3] = reply_command;
    put_u16_le(&out[4], sequence);
    put_u16_le(&out[6], payload_length);
    out[8] = original_command;
    out[9] = status;
    if (include_chunk) {
        put_u16_le(&out[10], TKBL_MAX_DATA_CHUNK);
    }

    crc = tkbl_crc32_update(0xFFFFFFFFUL, &out[2], (uint8_t)(6u + payload_length));
    crc ^= 0xFFFFFFFFUL;
    put_u32_le(&out[8u + payload_length], crc);

    ctx->last_reply_length = (uint8_t)(12u + payload_length);
    ctx->last_sequence = sequence;
    ctx->last_command = original_command;
    ctx->last_reply_valid = true;
    ctx->ops->send(out, ctx->last_reply_length);
}

static void nack(tkbl_context_t *ctx, uint8_t command, uint8_t status,
                 uint16_t sequence)
{
    send_reply(ctx, TKBL_CMD_NACK, command, status, sequence, false);
}

static void ack(tkbl_context_t *ctx, uint8_t command, uint16_t sequence,
                bool include_chunk)
{
    send_reply(ctx, TKBL_CMD_ACK, command, TKBL_OK, sequence, include_chunk);
}

static bool valid_range(uint32_t start, uint32_t end, uint32_t count)
{
    if (start != TKBL_APP_START || end > TKBL_APP_END || start >= end) {
        return false;
    }
    if (count == 0UL || count > (end - start)) {
        return false;
    }
    return true;
}

static void process_frame(tkbl_context_t *ctx)
{
    const uint8_t *frame = ctx->frame;
    const uint8_t *payload = &frame[8];
    uint8_t command = frame[3];
    uint16_t sequence = get_u16_le(&frame[4]);
    uint16_t payload_length = get_u16_le(&frame[6]);
    uint32_t received_crc = get_u32_le(&frame[8u + payload_length]);
    uint32_t calculated_crc;
    uint8_t status;

    calculated_crc = tkbl_crc32_update(0xFFFFFFFFUL, &frame[2],
                                       (uint8_t)(6u + payload_length));
    calculated_crc ^= 0xFFFFFFFFUL;
    if (calculated_crc != received_crc) {
        nack(ctx, command, TKBL_ERR_CRC, sequence);
        /* A transport-corrupted retry with the same sequence must be accepted. */
        ctx->last_reply_valid = false;
        return;
    }
    if (frame[2] != TKBL_PROTOCOL_VERSION) {
        nack(ctx, command, TKBL_ERR_COMMAND, sequence);
        return;
    }

    /* Android retries the same sequence after a timeout. Never write twice. */
    if (ctx->last_reply_valid && sequence == ctx->last_sequence
        && command == ctx->last_command) {
        ctx->ops->send(ctx->last_reply, ctx->last_reply_length);
        return;
    }

    switch (command) {
    case TKBL_CMD_HELLO:
        if (payload_length != 0u) {
            nack(ctx, command, TKBL_ERR_LENGTH, sequence);
        } else {
            ack(ctx, command, sequence, true);
        }
        break;

    case TKBL_CMD_BEGIN:
        if (payload_length != 16u) {
            nack(ctx, command, TKBL_ERR_LENGTH, sequence);
            break;
        }
        ctx->start = get_u32_le(&payload[0]);
        ctx->end = get_u32_le(&payload[4]);
        ctx->expected_count = get_u32_le(&payload[8]);
        ctx->expected_crc = get_u32_le(&payload[12]);
        if (!valid_range(ctx->start, ctx->end, ctx->expected_count)) {
            nack(ctx, command, TKBL_ERR_ADDRESS, sequence);
            break;
        }
        status = ctx->ops->begin(ctx->start, ctx->end, ctx->expected_count,
                                 ctx->expected_crc);
        if (status != TKBL_OK) {
            nack(ctx, command, status, sequence);
            break;
        }
        ctx->update_started = true;
        ctx->update_ended = false;
        ctx->update_verified = false;
        ctx->received_count = 0;
        ctx->running_crc = 0xFFFFFFFFUL;
        ctx->next_min_address = ctx->start;
        ack(ctx, command, sequence, false);
        break;

    case TKBL_CMD_DATA: {
        uint32_t address;
        uint8_t data_length;
        if (payload_length < 5u || payload_length > (4u + TKBL_MAX_DATA_CHUNK)) {
            nack(ctx, command, TKBL_ERR_LENGTH, sequence);
            break;
        }
        if (!ctx->update_started || ctx->update_ended) {
            nack(ctx, command, TKBL_ERR_STATE, sequence);
            break;
        }
        address = get_u32_le(payload);
        data_length = (uint8_t)(payload_length - 4u);
        if (address < ctx->start || address < ctx->next_min_address
            || address >= ctx->end
            || (uint32_t)data_length > (ctx->end - address)) {
            nack(ctx, command, TKBL_ERR_ADDRESS, sequence);
            break;
        }
        if ((uint32_t)data_length > (ctx->expected_count - ctx->received_count)) {
            nack(ctx, command, TKBL_ERR_IMAGE_SIZE, sequence);
            break;
        }
        status = ctx->ops->write(address, &payload[4], data_length);
        if (status != TKBL_OK) {
            nack(ctx, command, status, sequence);
            break;
        }
        ctx->running_crc = tkbl_crc32_update(ctx->running_crc, &payload[4],
                                             data_length);
        ctx->received_count += data_length;
        ctx->next_min_address = address + data_length;
        ack(ctx, command, sequence, false);
        break;
    }

    case TKBL_CMD_END:
        if (payload_length != 4u) {
            nack(ctx, command, TKBL_ERR_LENGTH, sequence);
            break;
        }
        if (!ctx->update_started || ctx->update_ended) {
            nack(ctx, command, TKBL_ERR_STATE, sequence);
            break;
        }
        if (get_u32_le(payload) != ctx->expected_crc
            || ctx->received_count != ctx->expected_count
            || (ctx->running_crc ^ 0xFFFFFFFFUL) != ctx->expected_crc) {
            nack(ctx, command, TKBL_ERR_CRC, sequence);
            break;
        }
        status = ctx->ops->finish();
        if (status != TKBL_OK) {
            nack(ctx, command, status, sequence);
            break;
        }
        ctx->update_ended = true;
        ack(ctx, command, sequence, false);
        break;

    case TKBL_CMD_VERIFY:
        if (payload_length != 0u) {
            nack(ctx, command, TKBL_ERR_LENGTH, sequence);
            break;
        }
        if (!ctx->update_started || !ctx->update_ended || ctx->update_verified) {
            nack(ctx, command, TKBL_ERR_STATE, sequence);
            break;
        }
        status = ctx->ops->verify(ctx->start, ctx->end, ctx->expected_count,
                                  ctx->expected_crc);
        if (status != TKBL_OK) {
            nack(ctx, command, status, sequence);
            break;
        }
        ctx->update_verified = true;
        ack(ctx, command, sequence, false);
        break;

    case TKBL_CMD_RESET:
        if (payload_length != 0u) {
            nack(ctx, command, TKBL_ERR_LENGTH, sequence);
            break;
        }
        if (!ctx->update_verified) {
            nack(ctx, command, TKBL_ERR_STATE, sequence);
            break;
        }
        ack(ctx, command, sequence, false);
        ctx->ops->request_reset();
        break;

    case TKBL_CMD_ABORT:
        if (payload_length != 0u) {
            nack(ctx, command, TKBL_ERR_LENGTH, sequence);
            break;
        }
        ctx->ops->abort();
        ctx->update_started = false;
        ctx->update_ended = false;
        ctx->update_verified = false;
        ack(ctx, command, sequence, false);
        break;

    default:
        nack(ctx, command, TKBL_ERR_COMMAND, sequence);
        break;
    }
}

void tkbl_init(tkbl_context_t *ctx, const tkbl_ops_t *ops)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->ops = ops;
}

static void parser_reset(tkbl_context_t *ctx)
{
    ctx->frame_length = 0;
    ctx->expected_length = 0;
}

void tkbl_feed(tkbl_context_t *ctx, const uint8_t *data, uint8_t length)
{
    uint8_t i;

    for (i = 0; i < length; ++i) {
        uint8_t byte = data[i];

        if (ctx->frame_length == 0u) {
            if (byte == TKBL_SOF0) {
                ctx->frame[ctx->frame_length++] = byte;
            }
            continue;
        }
        if (ctx->frame_length == 1u) {
            if (byte == TKBL_SOF1) {
                ctx->frame[ctx->frame_length++] = byte;
            } else if (byte == TKBL_SOF0) {
                ctx->frame[0] = byte;
            } else {
                parser_reset(ctx);
            }
            continue;
        }

        ctx->frame[ctx->frame_length++] = byte;
        if (ctx->frame_length == 8u) {
            uint16_t payload_length = get_u16_le(&ctx->frame[6]);
            if (payload_length > TKBL_MAX_PAYLOAD) {
                nack(ctx, ctx->frame[3], TKBL_ERR_LENGTH,
                     get_u16_le(&ctx->frame[4]));
                ctx->last_reply_valid = false;
                parser_reset(ctx);
                continue;
            }
            ctx->expected_length = (uint8_t)(12u + payload_length);
        }
        if (ctx->expected_length != 0u
            && ctx->frame_length == ctx->expected_length) {
            process_frame(ctx);
            parser_reset(ctx);
        }
    }
}
