#include "console.h"
#include "protocol/efi-lip.h"
#include "stdio.h"

EFI_STATUS efi_main(EFI_HANDLE ih, EFI_SYSTEM_TABLE *st) 
{
    st->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
    
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image = (void*)0;
    EFI_STATUS status;
    EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

    status = st->BootServices->HandleProtocol(ih, &loaded_image_guid, (void **)&loaded_image);
    printf("Image base: 0x%lx\n", (unsigned long)loaded_image->ImageBase);

    status = console_use_gop(st);
    if (status) {
        console_use_efi(st);
        printf("Can not use GOP !\n");
        return EFI_SUCCESS;
    }
    printf("Hello with GOP !\n");
    for(int n = 0; n < 99999; n++) printf("%d...\n", n);

    while(1);
}
