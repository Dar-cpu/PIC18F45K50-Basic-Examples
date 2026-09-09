#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "bootloader_config.h"
#include "pic18_flash.h"
#include "teckio_protocol.h"
#include "usb.h"

#define INVALID_ROW 0xFFFFFFFFUL
#define META_COMMIT 0x51AA3CC3UL

static uint8_t row_buffer[TKBL_FLASH_ROW_SIZE];
static uint32_t row_base = INVALID_ROW;
static bool row_dirty = false;

static const uint8_t metadata_magic[8] = {
    'T', 'K', 'A', 'P', 'P', '0', '1', 0xA5
};

static void set_table_pointer(uint32_t address)
{
    TBLPTRU = (uint8_t)(address >> 16);
    TBLPTRH = (uint8_t)(address >> 8);
    TBLPTRL = (uint8_t)address;
}

static uint8_t flash_read_byte(uint32_t address)
{
    set_table_pointer(address);
    __asm("TBLRD*");
    return TABLAT;
}

static uint32_t read_u32_flash(uint32_t address)
{
    uint32_t value;
    value = flash_read_byte(address);
    value |= (uint32_t)flash_read_byte(address + 1UL) << 8;
    value |= (uint32_t)flash_read_byte(address + 2UL) << 16;
    value |= (uint32_t)flash_read_byte(address + 3UL) << 24;
    return value;
}

static void put_u32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static void unlock_write(void)
{
    uint8_t saved_gie = INTCONbits.GIE;
    INTCONbits.GIE = 0;
    EECON2 = 0x55;
    EECON2 = 0xAA;
    EECON1bits.WR = 1;
    INTCONbits.GIE = saved_gie;
}

static bool erase_row(uint32_t address)
{
    if ((address & (TKBL_FLASH_ROW_SIZE - 1u)) != 0UL
        || address < TKBL_APP_START || address >= TKBL_FLASH_END) {
        return false;
    }

    set_table_pointer(address);
    EECON1bits.EEPGD = 1;
    EECON1bits.CFGS = 0;
    EECON1bits.FREE = 1;
    EECON1bits.WREN = 1;
    unlock_write();
    EECON1bits.WREN = 0;
    EECON1bits.FREE = 0;

    return flash_read_byte(address) == 0xFFu;
}

static bool program_row(uint32_t address, const uint8_t *data)
{
    uint8_t i;

    if ((address & (TKBL_FLASH_ROW_SIZE - 1u)) != 0UL
        || address < TKBL_APP_START || address >= TKBL_FLASH_END) {
        return false;
    }

    set_table_pointer(address);
    for (i = 0; i < TKBL_FLASH_ROW_SIZE; ++i) {
        TABLAT = data[i];
        __asm("TBLWT*+");
    }

    /* The write uses the row selected by TBLPTR, so restore its base. */
    set_table_pointer(address);
    EECON1bits.EEPGD = 1;
    EECON1bits.CFGS = 0;
    EECON1bits.FREE = 0;
    EECON1bits.WREN = 1;
    unlock_write();
    EECON1bits.WREN = 0;

    for (i = 0; i < TKBL_FLASH_ROW_SIZE; ++i) {
        if (flash_read_byte(address + i) != data[i]) {
            return false;
        }
    }
    return true;
}

static uint8_t flush_row(void)
{
    if (!row_dirty) {
        return TKBL_OK;
    }
    if (!program_row(row_base, row_buffer)) {
        return TKBL_ERR_FLASH;
    }
    row_base = INVALID_ROW;
    row_dirty = false;
    return TKBL_OK;
}

static uint8_t tk_eeprom_read(uint8_t address)
{
    EEADR = address;
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    EECON1bits.RD = 1;
    return EEDATA;
}

static void tk_eeprom_write(uint8_t address, uint8_t value)
{
    EEADR = address;
    EEDATA = value;
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    EECON1bits.WREN = 1;
    unlock_write();
    while (EECON1bits.WR) {}
    EECON1bits.WREN = 0;
}

bool pic18_boot_request_take(void)
{
    if (tk_eeprom_read(TKBL_EEPROM_REQUEST_ADDR) != TKBL_EEPROM_REQUEST_KEY) {
        return false;
    }
    tk_eeprom_write(TKBL_EEPROM_REQUEST_ADDR, 0xFFu);
    return true;
}

bool pic18_application_is_valid(void)
{
    uint8_t i;
    uint32_t start;
    uint32_t end;
    uint32_t byte_count;
    uint32_t crc;
    uint32_t commit;

    for (i = 0; i < sizeof(metadata_magic); ++i) {
        if (flash_read_byte(TKBL_METADATA_START + i) != metadata_magic[i]) {
            return false;
        }
    }

    start = read_u32_flash(TKBL_METADATA_START + 8UL);
    end = read_u32_flash(TKBL_METADATA_START + 12UL);
    byte_count = read_u32_flash(TKBL_METADATA_START + 16UL);
    if (start != TKBL_APP_START || end > TKBL_APP_END || start >= end
        || byte_count == 0UL || byte_count > (end - start)) {
        return false;
    }
    crc = read_u32_flash(TKBL_METADATA_START + 20UL);
    if (read_u32_flash(TKBL_METADATA_START + 24UL) != ~crc) {
        return false;
    }
    commit = read_u32_flash(TKBL_METADATA_START + 28UL);
    if (commit != META_COMMIT
        || read_u32_flash(TKBL_METADATA_START + 32UL) != ~commit) {
        return false;
    }

    /* A valid relocated reset vector cannot be completely erased. */
    return flash_read_byte(TKBL_APP_START) != 0xFFu
        || flash_read_byte(TKBL_APP_START + 1UL) != 0xFFu;
}

uint8_t pic18_flash_begin(uint32_t start, uint32_t end,
                          uint32_t byte_count, uint32_t image_crc)
{
    uint32_t address;

    (void)start;
    (void)end;
    (void)byte_count;
    (void)image_crc;

    row_base = INVALID_ROW;
    row_dirty = false;

    /* Invalidate first, then erase all application rows to remove stale code. */
    if (!erase_row(TKBL_METADATA_START)) {
        return TKBL_ERR_FLASH;
    }
    for (address = TKBL_APP_START; address < TKBL_APP_END;
         address += TKBL_FLASH_ROW_SIZE) {
        if (!erase_row(address)) {
            return TKBL_ERR_FLASH;
        }
    }
    return TKBL_OK;
}

uint8_t pic18_flash_write(uint32_t address, const uint8_t *data, uint8_t length)
{
    uint8_t i;

    for (i = 0; i < length; ++i, ++address) {
        uint32_t base = address & ~(uint32_t)(TKBL_FLASH_ROW_SIZE - 1u);
        uint8_t offset = (uint8_t)(address - base);

        if (base < TKBL_APP_START || base >= TKBL_APP_END) {
            return TKBL_ERR_PROTECTED;
        }
        if (row_base != base) {
            uint8_t status = flush_row();
            if (status != TKBL_OK) {
                return status;
            }
            memset(row_buffer, 0xFF, sizeof(row_buffer));
            row_base = base;
        }
        row_buffer[offset] = data[i];
        row_dirty = true;

        if (offset == (TKBL_FLASH_ROW_SIZE - 1u)) {
            uint8_t status = flush_row();
            if (status != TKBL_OK) {
                return status;
            }
        }
    }
    return TKBL_OK;
}

uint8_t pic18_flash_finish(void)
{
    return flush_row();
}

uint8_t pic18_flash_verify(uint32_t start, uint32_t end,
                           uint32_t byte_count, uint32_t image_crc)
{
    uint8_t metadata[TKBL_FLASH_ROW_SIZE];
    uint8_t i;

    if (start != TKBL_APP_START || end > TKBL_APP_END || start >= end
        || byte_count == 0UL) {
        return TKBL_ERR_ADDRESS;
    }
    if (flash_read_byte(TKBL_APP_START) == 0xFFu
        && flash_read_byte(TKBL_APP_START + 1UL) == 0xFFu) {
        return TKBL_ERR_IMAGE_SIZE;
    }

    memset(metadata, 0xFF, sizeof(metadata));
    for (i = 0; i < sizeof(metadata_magic); ++i) {
        metadata[i] = metadata_magic[i];
    }
    put_u32(&metadata[8], start);
    put_u32(&metadata[12], end);
    put_u32(&metadata[16], byte_count);
    put_u32(&metadata[20], image_crc);
    put_u32(&metadata[24], ~image_crc);
    put_u32(&metadata[28], META_COMMIT);
    put_u32(&metadata[32], ~META_COMMIT);

    if (!program_row(TKBL_METADATA_START, metadata)) {
        return TKBL_ERR_FLASH;
    }
    return TKBL_OK;
}

void pic18_flash_abort(void)
{
    row_base = INVALID_ROW;
    row_dirty = false;
    (void)erase_row(TKBL_METADATA_START);
}

void pic18_jump_to_application(void)
{
    INTCONbits.GIE = 0;
    INTCONbits.PEIE = 0;
    USB_INTERRUPT_ENABLE = 0;
    usb_close();
    __asm("goto 0x2000");
}
