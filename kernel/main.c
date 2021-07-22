#include "memory.h"
#include "loader.h"
#include "interrupt.h"
#include "srv_builtin.h"
#include "stdio.h"

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
    int res = service_send("hi:", NULL, 0, NULL);
    if (res < 0) printf("err -%d\n", -res);
    else printf("got %d\n", res);

    struct iovec iov = { "9\n", 3 };
    res = service_send("rec:", &iov, 1, NULL);
    if (res < 0) printf("err -%d", -res);
    else printf("got %d\n", res);

    while(1) {
        __asm__ __volatile__("sti":::"memory");
        __asm__ __volatile__("hlt":::"memory");
        __asm__ __volatile__("cli":::"memory");
    }
}
