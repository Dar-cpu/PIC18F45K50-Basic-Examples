#ifndef TECKIO_PIC18_FLASH_H
#define TECKIO_PIC18_FLASH_H

#include <stdbool.h>
#include <stdint.h>

uint8_t pic18_flash_begin(uint32_t start, uint32_t end,
                          uint32_t byte_count, uint32_t image_crc);
uint8_t pic18_flash_write(uint32_t address, const uint8_t *data, uint8_t length);
uint8_t pic18_flash_finish(void);
uint8_t pic18_flash_verify(uint32_t start, uint32_t end,
                           uint32_t byte_count, uint32_t image_crc);
void pic18_flash_abort(void);

bool pic18_application_is_valid(void);
bool pic18_boot_request_take(void);
void pic18_jump_to_application(void);

#endif
