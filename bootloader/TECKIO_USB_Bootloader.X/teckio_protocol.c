#include "teckio_protocol.h"

#include <stdbool.h>

#define TKBL_SOF0 0x54u
#define TKBL_SOF1 0x4Bu

enum update_state {
    UPDATE_IDLE,
    UPDATE_RECEIVING,
    UPDATE_ENDED,
    UPDATE_VERIFIED
};

static uint8_t frame[TKBL_MAX_FRAME];
static uint8_t frame_length;
static uint8_t expected_length;
static uint8_t last_reply[16];
static uint8_t last_reply_length;
static uint8_t last_command;
static uint16_t last_sequence;
static bool last_reply_valid;
static uint8_t update_state;
static uint16_t image_end;
static uint16_t expected_count;
static uint16_t received_count;
static uint16_t next_min_address;
static uint32_t expected_crc;
static uint32_t running_crc;

static uint16_t get_u16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void put_u16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put_u32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

uint32_t tkbl_crc32_update(uint32_t crc, const uint8_t *data, uint8_t length)
{
    uint8_t bit;
    while (length-- != 0u) {
        crc ^= *data++;
        for (bit = 0; bit < 8u; ++bit) {
            crc = (crc >> 1) ^ ((crc & 1UL) ? 0xEDB88320UL : 0UL);
        }
    }
    return crc;
}

static void reply(uint8_t command, uint8_t status, uint16_t sequence)
{
    uint8_t payload_length = 2u;
    uint32_t crc;

    last_reply[0] = TKBL_SOF0;
    last_reply[1] = TKBL_SOF1;
    last_reply[2] = TKBL_PROTOCOL_VERSION;
    last_reply[3] = status == TKBL_OK ? TKBL_CMD_ACK : TKBL_CMD_NACK;
    put_u16(&last_reply[4], sequence);
    if (command == TKBL_CMD_HELLO && status == TKBL_OK) {
        payload_length = 4u;
        put_u16(&last_reply[10], TKBL_MAX_DATA_CHUNK);
    }
    put_u16(&last_reply[6], payload_length);
    last_reply[8] = command;
    last_reply[9] = status;
    crc = tkbl_crc32_update(0xFFFFFFFFUL, &last_reply[2],
                            (uint8_t)(6u + payload_length));
    put_u32(&last_reply[8u + payload_length], crc ^ 0xFFFFFFFFUL);

    last_reply_length = (uint8_t)(12u + payload_length);
    last_command = command;
    last_sequence = sequence;
    last_reply_valid = true;
    tkbl_platform_send(last_reply, last_reply_length);
}

static bool upper_word_is_zero(const uint8_t *p)
{
    return p[2] == 0u && p[3] == 0u;
}

static void process_frame(void)
{
    uint8_t *payload = &frame[8];
    uint8_t command = frame[3];
    uint8_t payload_length = frame[6];
    uint16_t sequence = get_u16(&frame[4]);
    uint8_t status = TKBL_OK;

    if ((tkbl_crc32_update(0xFFFFFFFFUL, &frame[2],
            (uint8_t)(6u + payload_length)) ^ 0xFFFFFFFFUL)
            != get_u32(&frame[8u + payload_length])) {
        reply(command, TKBL_ERR_CRC, sequence);
        last_reply_valid = false;
        return;
    }
    if (frame[2] != TKBL_PROTOCOL_VERSION) {
        reply(command, TKBL_ERR_COMMAND, sequence);
        return;
    }
    if (last_reply_valid && command == last_command
        && sequence == last_sequence) {
        tkbl_platform_send(last_reply, last_reply_length);
        return;
    }

    if (command == TKBL_CMD_HELLO) {
        status = payload_length == 0u ? TKBL_OK : TKBL_ERR_LENGTH;
    } else if (command == TKBL_CMD_BEGIN) {
        uint16_t start;
        uint16_t count;
        if (payload_length != 16u) {
            status = TKBL_ERR_LENGTH;
        } else if (!upper_word_is_zero(&payload[0])
                   || !upper_word_is_zero(&payload[4])
                   || !upper_word_is_zero(&payload[8])) {
            status = TKBL_ERR_ADDRESS;
        } else {
            start = get_u16(&payload[0]);
            image_end = get_u16(&payload[4]);
            count = get_u16(&payload[8]);
            expected_crc = get_u32(&payload[12]);
            if (start != TKBL_APP_START || image_end > TKBL_APP_END
                || image_end <= start || count == 0u
                || count > (uint16_t)(image_end - start)) {
                status = TKBL_ERR_ADDRESS;
            } else {
                status = tkbl_platform_begin();
                if (status == TKBL_OK) {
                    expected_count = count;
                    received_count = 0u;
                    next_min_address = TKBL_APP_START;
                    running_crc = 0xFFFFFFFFUL;
                    update_state = UPDATE_RECEIVING;
                }
            }
        }
    } else if (command == TKBL_CMD_DATA) {
        uint16_t address;
        uint8_t data_length;
        if (payload_length < 5u || payload_length > TKBL_MAX_PAYLOAD) {
            status = TKBL_ERR_LENGTH;
        } else if (update_state != UPDATE_RECEIVING) {
            status = TKBL_ERR_STATE;
        } else if (!upper_word_is_zero(payload)) {
            status = TKBL_ERR_ADDRESS;
        } else {
            address = get_u16(payload);
            data_length = (uint8_t)(payload_length - 4u);
            if (address < next_min_address || address < TKBL_APP_START
                || address >= image_end
                || data_length > (uint16_t)(image_end - address)) {
                status = TKBL_ERR_ADDRESS;
            } else if (data_length > (uint16_t)(expected_count - received_count)) {
                status = TKBL_ERR_IMAGE_SIZE;
            } else {
                status = tkbl_platform_write(address, &payload[4], data_length);
                if (status == TKBL_OK) {
                    running_crc = tkbl_crc32_update(running_crc, &payload[4],
                                                    data_length);
                    received_count += data_length;
                    next_min_address = address + data_length;
                }
            }
        }
    } else if (command == TKBL_CMD_END) {
        if (payload_length != 4u) {
            status = TKBL_ERR_LENGTH;
        } else if (update_state != UPDATE_RECEIVING) {
            status = TKBL_ERR_STATE;
        } else if (get_u32(payload) != expected_crc
                   || received_count != expected_count
                   || (running_crc ^ 0xFFFFFFFFUL) != expected_crc) {
            status = TKBL_ERR_CRC;
        } else {
            status = tkbl_platform_finish();
            if (status == TKBL_OK) {
                update_state = UPDATE_ENDED;
            }
        }
    } else if (command == TKBL_CMD_VERIFY) {
        if (payload_length != 0u) {
            status = TKBL_ERR_LENGTH;
        } else if (update_state != UPDATE_ENDED) {
            status = TKBL_ERR_STATE;
        } else {
            status = tkbl_platform_commit(TKBL_APP_START, image_end,
                                          expected_count, expected_crc);
            if (status == TKBL_OK) {
                update_state = UPDATE_VERIFIED;
            }
        }
    } else if (command == TKBL_CMD_RESET) {
        if (payload_length != 0u) {
            status = TKBL_ERR_LENGTH;
        } else if (update_state != UPDATE_VERIFIED) {
            status = TKBL_ERR_STATE;
        }
    } else if (command == TKBL_CMD_ABORT) {
        if (payload_length != 0u) {
            status = TKBL_ERR_LENGTH;
        } else {
            tkbl_platform_abort();
            update_state = UPDATE_IDLE;
        }
    } else {
        status = TKBL_ERR_COMMAND;
    }

    reply(command, status, sequence);
    if (command == TKBL_CMD_RESET && status == TKBL_OK) {
        tkbl_platform_reset();
    }
}

void tkbl_init(void)
{
    frame_length = 0u;
    expected_length = 0u;
    last_reply_valid = false;
    update_state = UPDATE_IDLE;
}

void tkbl_feed(const uint8_t *data, uint8_t length)
{
    uint8_t byte;
    while (length-- != 0u) {
        byte = *data++;
        if (frame_length == 0u) {
            if (byte == TKBL_SOF0) {
                frame[frame_length++] = byte;
            }
        } else if (frame_length == 1u) {
            if (byte == TKBL_SOF1) {
                frame[frame_length++] = byte;
            } else if (byte != TKBL_SOF0) {
                frame_length = 0u;
            }
        } else {
            frame[frame_length++] = byte;
            if (frame_length == 8u) {
                if (frame[7] != 0u || frame[6] > TKBL_MAX_PAYLOAD) {
                    reply(frame[3], TKBL_ERR_LENGTH, get_u16(&frame[4]));
                    last_reply_valid = false;
                    frame_length = 0u;
                } else {
                    expected_length = (uint8_t)(12u + frame[6]);
                }
            } else if (expected_length != 0u
                       && frame_length == expected_length) {
                process_frame();
                frame_length = 0u;
                expected_length = 0u;
            }
        }
    }
}
