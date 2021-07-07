/*
 * efi-stop.h
 *
 * UEFI Simple Text Output Protocol.
 */

#ifndef __EFI_STOP_H
#define __EFI_STOP_H

#include <efi.h>


#define EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID    {0x387477c2, 0x69c7, 0x11d2, {0x82, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}


struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef EFI_STATUS (*EFI_TEXT_RESET)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, bool8_t ExtendedVerification);
typedef EFI_STATUS (*EFI_TEXT_STRING)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, char16_t *String);
typedef EFI_STATUS (*EFI_TEXT_TEST_STRING)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, char16_t *String);
typedef EFI_STATUS (*EFI_TEXT_QUERY_MODE)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, uintn_t ModeNumber, uintn_t *Columns, uintn_t *Rows);
typedef EFI_STATUS (*EFI_TEXT_SET_MODE)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, uintn_t ModeNumber);
typedef EFI_STATUS (*EFI_TEXT_SET_ATTRIBUTE)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, uintn_t Attribute);
typedef EFI_STATUS (*EFI_TEXT_CLEAR_SCREEN)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);
typedef EFI_STATUS (*EFI_TEXT_SET_CURSOR_POSITION)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, uintn_t Column, uintn_t Row);
typedef EFI_STATUS (*EFI_TEXT_ENABLE_CURSOR)(struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, bool8_t Visible);

typedef struct SIMPLE_TEXT_OUTPUT_MODE {
    int32_t   MaxMode;
    int32_t   Mode;
    int32_t   Attribute;
    int32_t   CursorColumn;
    int32_t   CursorRow;
    bool8_t CursorVisible;
} SIMPLE_TEXT_OUTPUT_MODE;

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_TEXT_RESET                  Reset;
    EFI_TEXT_STRING                 OutputString;
    EFI_TEXT_TEST_STRING            TestString;
    EFI_TEXT_QUERY_MODE             QueryMode;
    EFI_TEXT_SET_MODE               SetMode;
    EFI_TEXT_SET_ATTRIBUTE          SetAttribute;
    EFI_TEXT_CLEAR_SCREEN           ClearScreen;
    EFI_TEXT_SET_CURSOR_POSITION    SetCursorPosition;
    EFI_TEXT_ENABLE_CURSOR          EnableCursor;
    SIMPLE_TEXT_OUTPUT_MODE         *Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;


#endif /* __EFI_STOP_H */
