#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "teckio_protocol.h"

static uint8_t reply[32];
static uint8_t reply_length;
static unsigned begin_calls;
static unsigned write_calls;
static unsigned finish_calls;
static unsigned verify_calls;
static bool reset_requested;

static void put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static uint8_t make_frame(uint8_t *out, uint8_t command, uint16_t sequence,
                          const uint8_t *payload, uint16_t payload_length)
{
    uint32_t crc;
    out[0] = 0x54;
    out[1] = 0x4B;
    out[2] = 1;
    out[3] = command;
    put_u16(&out[4], sequence);
    put_u16(&out[6], payload_length);
    if (payload_length != 0) {
        memcpy(&out[8], payload, payload_length);
    }
    crc = tkbl_crc32_update(0xFFFFFFFFUL, &out[2], (uint8_t)(6 + payload_length));
    put_u32(&out[8 + payload_length], crc ^ 0xFFFFFFFFUL);
    return (uint8_t)(12 + payload_length);
}

uint8_t tkbl_platform_begin(void)
{
    ++begin_calls;
    return TKBL_OK;
}

uint8_t tkbl_platform_write(uint16_t address, const uint8_t *data,
                            uint8_t length)
{
    (void)address; (void)data; (void)length;
    ++write_calls;
    return TKBL_OK;
}

uint8_t tkbl_platform_finish(void)
{
    ++finish_calls;
    return TKBL_OK;
}

uint8_t tkbl_platform_commit(uint16_t start, uint16_t end, uint16_t count,
                             uint32_t crc)
{
    (void)start; (void)end; (void)count; (void)crc;
    ++verify_calls;
    return TKBL_OK;
}

void tkbl_platform_abort(void) {}
void tkbl_platform_reset(void) { reset_requested = true; }

void tkbl_platform_send(const uint8_t *data, uint8_t length)
{
    memcpy(reply, data, length);
    reply_length = length;
}

int main(void)
{
    uint8_t frame[96];
    uint8_t payload[32];
    uint8_t image[] = {1, 2, 3, 4, 5, 6};
    uint8_t length;
    uint32_t image_crc;

    tkbl_init();
    assert((tkbl_crc32_update(0xFFFFFFFFUL, (const uint8_t *)"123456789", 9)
            ^ 0xFFFFFFFFUL) == 0xCBF43926UL);

    length = make_frame(frame, TKBL_CMD_HELLO, 1, NULL, 0);
    tkbl_feed(frame, length);
    assert(reply_length == 16 && reply[3] == TKBL_CMD_ACK);
    assert(reply[8] == TKBL_CMD_HELLO && reply[10] == 64 && reply[11] == 0);

    image_crc = tkbl_crc32_update(0xFFFFFFFFUL, image, sizeof(image)) ^ 0xFFFFFFFFUL;
    put_u32(&payload[0], TKBL_APP_START);
    put_u32(&payload[4], TKBL_APP_START + sizeof(image));
    put_u32(&payload[8], sizeof(image));
    put_u32(&payload[12], image_crc);
    length = make_frame(frame, TKBL_CMD_BEGIN, 2, payload, 16);
    tkbl_feed(frame, 3);
    tkbl_feed(&frame[3], (uint8_t)(length - 3));
    assert(begin_calls == 1 && reply[3] == TKBL_CMD_ACK);

    put_u32(payload, TKBL_APP_START);
    memcpy(&payload[4], image, sizeof(image));
    length = make_frame(frame, TKBL_CMD_DATA, 3, payload, 4 + sizeof(image));
    tkbl_feed(frame, 8);
    tkbl_feed(&frame[8], (uint8_t)(length - 8));
    assert(write_calls == 1 && reply[3] == TKBL_CMD_ACK);

    /* Same sequence is a retry: reply again, but do not write Flash twice. */
    tkbl_feed(frame, length);
    assert(write_calls == 1 && reply[3] == TKBL_CMD_ACK);

    put_u32(payload, image_crc);
    length = make_frame(frame, TKBL_CMD_END, 4, payload, 4);
    tkbl_feed(frame, length);
    assert(finish_calls == 1 && reply[3] == TKBL_CMD_ACK);

    length = make_frame(frame, TKBL_CMD_VERIFY, 5, NULL, 0);
    tkbl_feed(frame, length);
    assert(verify_calls == 1 && reply[3] == TKBL_CMD_ACK);

    length = make_frame(frame, TKBL_CMD_RESET, 6, NULL, 0);
    tkbl_feed(frame, length);
    assert(reset_requested && reply[3] == TKBL_CMD_ACK);

    puts("TECKIO protocol tests: OK");
    return 0;
}
