#ifndef TECKIO_PROTOCOL_H
#define TECKIO_PROTOCOL_H

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

void tkbl_init(void);
void tkbl_feed(const uint8_t *data, uint8_t length);
uint32_t tkbl_crc32_update(uint32_t crc, const uint8_t *data, uint8_t length);

uint8_t tkbl_platform_begin(void);
uint8_t tkbl_platform_write(uint16_t address, const uint8_t *data,
                            uint8_t length);
uint8_t tkbl_platform_finish(void);
uint8_t tkbl_platform_commit(uint16_t start, uint16_t end,
                             uint16_t byte_count, uint32_t image_crc);
void tkbl_platform_abort(void);
void tkbl_platform_reset(void);
void tkbl_platform_send(const uint8_t *data, uint8_t length);

#endif
