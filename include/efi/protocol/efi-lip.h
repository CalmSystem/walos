/*
 * efi-lip.h
 * 
 * UEFI Loaded Image Protocol.
 */

#ifndef __EFI_LIP_H
#define __EFI_LIP_H

#include "../efi.h"


#define EFI_LOADED_IMAGE_PROTOCOL_GUID  {0x5b1b31a1, 0x9562, 0x11d2, {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}

#define EFI_LOADED_IMAGE_PROTOCOL_REVISION  0x1000


typedef EFI_STATUS (*EFI_IMAGE_UNLOAD)(EFI_HANDLE ImageHandle);

typedef struct EFI_LOADED_IMAGE_PROTOCOL {
    uint32_t                      Revision;
    EFI_HANDLE                  ParentHandle;
    EFI_SYSTEM_TABLE            *SystemTable;
    EFI_HANDLE                  DeviceHandle;
    EFI_DEVICE_PATH_PROTOCOL    *FilePath;
    void                        *Reserved;
    uint32_t                      LoadOptionsSize;
    void                        *LoadOptions;
    void                        *ImageBase;
    uint64_t                      ImageSize;
    uintn_t                       ImageCodeType;
    EFI_IMAGE_UNLOAD            UnLoad;
} EFI_LOADED_IMAGE_PROTOCOL;


#endif /* __EFI_LIP_H */
