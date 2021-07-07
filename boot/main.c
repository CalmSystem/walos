#include "main.h"
#include "protocol/efi-lip.h"
#include "stdio.h"

EFI_HANDLE          image_handle;
EFI_SYSTEM_TABLE    *system_table;

EFI_STATUS efi_main(EFI_HANDLE ih, EFI_SYSTEM_TABLE *st) 
{
    image_handle = ih;
    system_table = st;

    printf("Hello world !\n");
    //SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);

    EFI_LOADED_IMAGE_PROTOCOL *loaded_image = (void*)0;
    EFI_STATUS status;
    EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

    status = system_table->BootServices->HandleProtocol(image_handle, &loaded_image_guid, (void **)&loaded_image);
    printf("Image base: 0x%lx\n", (unsigned long)loaded_image->ImageBase);

    while(1);

    return(EFI_SUCCESS);
}
