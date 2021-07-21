#include "native.h"
#include "loader.h"
#include "interrupt.h"
#include "memory.h"
#include "wax/wax.h"
#include "stdio.h"
#include "srv_builtin.h"

void _start(BOOT_INFO* bootinfo) {

    interrupts_setup();

    if (bootinfo->lfb && bootinfo->font)
    graphics_use_lfb(bootinfo->lfb, bootinfo->font);
    else graphics_use_dummy();

    puts("Hello Walos !\n...");

    memory_setup(bootinfo->mmap);

    EXEC_ENGINE *engine = exec_load_m3();
    assert(engine && "Failed to load WASM runtime");
    puts("Engine: Loaded");

    if (bootinfo->services.ptr) srv_use(bootinfo->services, engine);

    srv_register_builtin();

    puts("Go !!!");
    int res = srv_send("hello:", NULL, 0, NULL);
    if (res < 0) printf("err -%d", -res);
    else printf("got %d\n", res);

    res = srv_send("rec:", (const uint8_t*)"9\n", 3, NULL);
    if (res < 0) printf("err -%d", -res);
    else printf("got %d\n", res);

    while(1) {
        __asm__ __volatile__("sti":::"memory");
        __asm__ __volatile__("hlt":::"memory");
        __asm__ __volatile__("cli":::"memory");
    }
}
