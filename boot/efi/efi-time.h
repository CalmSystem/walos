/*
 * efi-time.h
 *
 * UEFI Time representation.
 */

#ifndef __EFI_TIME_H
#define __EFI_TIME_H

#include <efi.h>


#define EFI_TIME_ADJUST_DAYLIGHT    0x01
#define EFI_TIME_IN_DAYLIGHT        0x02

#define EFI_UNSPECIFIED_TIMEZONE    0x07ff


typedef struct EFI_TIME {
    uint16_t  Year;
    uint8_t   Month;
    uint8_t   Day;
    uint8_t   Hour;
    uint8_t   Minute;
    uint8_t   Second;
    uint8_t   Pad1;
    uint32_t  Nanosecond;
    int16_t   TimeZone;
    uint8_t   Daylight;
    uint8_t   PAD2;
} EFI_TIME;

typedef struct EFI_TIME_CAPABILITIES {
    uint32_t  Resolution;
    uint32_t  Accuracy;
    bool8_t SetsToZero;
} EFI_TIME_CAPABILITIES;


#endif /* __EFI_TIME_H */
