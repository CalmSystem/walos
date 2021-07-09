#include "graphics.h"
#include "stdio.h"

void _start(BOOT_INFO* bootinfo) {
    
    if (bootinfo->lfb && bootinfo->font) graphics_use_lfb(bootinfo->lfb, bootinfo->font);
    else graphics_use_dummy();

    printf("Hello Walos !\n...");

    uintn_t CR3;
    __asm__ __volatile__("mov %%cr3, %0" : "=a"(CR3));
    printf("CR3 %lx\n", CR3);

    while(1);
}
