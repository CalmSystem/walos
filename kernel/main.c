#include "native.h"
#include "loader.h"
#include "wax/wax.h"
#include "stdio.h"
#include "srv_internal.h"

void _start(BOOT_INFO* bootinfo) {

    if (bootinfo->lfb && bootinfo->font) graphics_use_lfb(bootinfo->lfb, bootinfo->font);
    else graphics_use_dummy();

    puts("Hello Walos !\n...");

    uintn_t CR3;
    __asm__ __volatile__("mov %%cr3, %0" : "=a"(CR3));
    printf("CR3 %lx\n", CR3);

    EXEC_ENGINE *engine = load_m3_engine();
    assert(engine && "Failed to load WASM runtime");
    puts("Engine: Loaded");

    if (bootinfo->services.ptr) srv_use(bootinfo->services, engine);

    srv_add_internals();

    puts("Go !!!");
    int res = srv_send("hello:", NULL, 0, NULL);
    if (res < 0) printf("err -%d", -res);
    else printf("got %d\n", res);

    res = srv_send("rec:", (const uint8_t*)"9\n", 3, NULL);
    if (res < 0) printf("err -%d", -res);
    else printf("got %d\n", res);

    while(1) {
        __asm__ __volatile__("sti");
        __asm__ __volatile__("hlt");
        __asm__ __volatile__("cli");
        putchar('.');
    }
}
