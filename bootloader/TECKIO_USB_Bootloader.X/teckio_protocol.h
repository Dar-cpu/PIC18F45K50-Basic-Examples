#ifndef TECKIO_PROTOCOL_H
#define TECKIO_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#include "bootloader_config.h"

#define TKBL_PROTOCOL_VERSION 0x01u
#define TKBL_MAX_PAYLOAD      (4u + TKBL_MAX_DATA_CHUNK)
#define TKBL_MAX_FRAME        (12u + TKBL_MAX_PAYLOAD)

enum tkbl_command {
    TKBL_CMD_HELLO  = 0x01,
    TKBL_CMD_BEGIN  = 0x02,
    TKBL_CMD_DATA   = 0x03,
    TKBL_CMD_END    = 0x04,
    TKBL_CMD_VERIFY = 0x05,
    TKBL_CMD_RESET  = 0x06,
    TKBL_CMD_ABORT  = 0x7F,
    TKBL_CMD_ACK    = 0x80,
    TKBL_CMD_NACK   = 0x81
};

enum tkbl_status {
    TKBL_OK             = 0,
    TKBL_ERR_COMMAND    = 1,
    TKBL_ERR_LENGTH     = 2,
    TKBL_ERR_STATE      = 3,
    TKBL_ERR_ADDRESS    = 4,
    TKBL_ERR_FLASH      = 5,
    TKBL_ERR_CRC        = 6,
    TKBL_ERR_PROTECTED  = 7,
    TKBL_ERR_IMAGE_SIZE = 8
};

typedef struct {
    uint8_t (*begin)(uint32_t start, uint32_t end, uint32_t byte_count,
                     uint32_t image_crc);
    uint8_t (*write)(uint32_t address, const uint8_t *data, uint8_t length);
    uint8_t (*finish)(void);
    uint8_t (*verify)(uint32_t start, uint32_t end, uint32_t byte_count,
                      uint32_t image_crc);
    void (*abort)(void);
    void (*request_reset)(void);
    void (*send)(const uint8_t *data, uint8_t length);
} tkbl_ops_t;

typedef struct {
    const tkbl_ops_t *ops;
    uint8_t frame[TKBL_MAX_FRAME];
    uint8_t frame_length;
    uint8_t expected_length;

    uint8_t last_reply[16];
    uint8_t last_reply_length;
    uint16_t last_sequence;
    uint8_t last_command;
    bool last_reply_valid;

    bool update_started;
    bool update_ended;
    bool update_verified;
    uint32_t start;
    uint32_t end;
    uint32_t expected_count;
    uint32_t expected_crc;
    uint32_t received_count;
    uint32_t running_crc;
    uint32_t next_min_address;
} tkbl_context_t;

void tkbl_init(tkbl_context_t *ctx, const tkbl_ops_t *ops);
void tkbl_feed(tkbl_context_t *ctx, const uint8_t *data, uint8_t length);
uint32_t tkbl_crc32_update(uint32_t crc, const uint8_t *data, uint8_t length);

#endif
