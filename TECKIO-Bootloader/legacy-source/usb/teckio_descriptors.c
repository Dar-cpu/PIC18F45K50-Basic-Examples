/* TECKIO branding shared by the bootloader and offset application.
 * Reuse the pinned stack's device/configuration descriptors unchanged.
 * Only replace the string lookup table; keep language and serial identity.
 */
#define g_string_descriptors upstream_string_descriptors
#define g_size_of_sd upstream_size_of_sd
#include "../third_party/USB-Stack/USB_Stack/Examples/CDC_Examples/Shared_Files/usb_descriptors.c"
#undef g_string_descriptors
#undef g_size_of_sd

static const uint8_t teckio_manufacturer[] = {
    14, STRING_DESC,
    'T',0, 'E',0, 'C',0, 'K',0, 'I',0, 'O',0
};

static const uint8_t teckio_product[] = {
    34, STRING_DESC,
    'T',0, 'E',0, 'C',0, 'K',0, 'I',0, 'O',0, ' ',0,
    'P',0, 'I',0, 'C',0, '1',0, '8',0, ' ',0, 'U',0, 'S',0, 'B',0
};

const uint16_t g_string_descriptors[] = {
    (uint16_t)&string_zero_descriptor,
    (uint16_t)&teckio_manufacturer,
    (uint16_t)&teckio_product,
    (uint16_t)&serial_string_descriptor
};
const uint8_t g_size_of_sd = sizeof(g_string_descriptors);
