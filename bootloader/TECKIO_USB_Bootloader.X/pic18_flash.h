#ifndef TECKIO_PIC18_FLASH_H
#define TECKIO_PIC18_FLASH_H

#include <stdbool.h>

bool pic18_application_is_valid(void);
bool pic18_boot_request_take(void);
void pic18_jump_to_application(void);

#endif
