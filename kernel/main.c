#include "memory.h"
#include "loader.h"
#include "interrupt.h"
#include "srv_builtin.h"
#include "stdio.h"
#include "asm.h"
#include "acpi.h"
#include "syslog.h"

void _start(BOOT_INFO* bootinfo) {

    interrupts_setup();

    if (bootinfo->lfb && bootinfo->font) {
    graphics_use_lfb(bootinfo->lfb, bootinfo->font);
        syslogs((syslog_ctx){"GRAPHICS", NULL, INFO}, "LFB Ready");
        syslog_handlers = &syslog_graphics_handler;
        syslogs((syslog_ctx){"LOG", NULL, DEBUG}, "Using graphics");
    } else {
        graphics_use_dummy();
        syslogs((syslog_ctx){"GRAPHICS", NULL, ERROR}, "Can not setup");
        syslog_handlers = &syslog_serial_handler;
        syslogs((syslog_ctx){"LOG", NULL, DEBUG}, "Using serial");
    }
    syslogs((syslog_ctx){"LOG", NULL, INFO}, "Ready");

    syslogs((syslog_ctx){"OS", NULL, INFO}, "Hello Walos !");

    memory_setup(bootinfo->mmap);

    if (bootinfo->acpi_rsdp) {
        acpi_load_root(bootinfo->acpi_rsdp);
        uint32_t ncore;
        apic_read(APIC_TYPE_LOCAL, NULL, &ncore);
        syslogf((syslog_ctx){"APIC", NULL, INFO}, "Found %u CPU logical cores", ncore);
    } else {
        syslogs((syslog_ctx){"APIC", NULL, WARN}, "Not available");
    }

    EXEC_ENGINE *engine = exec_load_m3();
    if (engine) {
        syslogs((syslog_ctx){"WASM", NULL, INFO}, "Engine loaded");
    } else {
        syslogs((syslog_ctx){"WASM", NULL, FATAL}, "Failed to load engine");
        return;
    }

    if (bootinfo->services.ptr) {
        srv_setup(bootinfo->services, engine);
    srv_register_builtin();
    }

    puts("Go !!!");
    int res = service_send("hi:", NULL, 0, NULL);
    if (res < 0) printf("err -%d\n", -res);
    else printf("got %d\n", res);

    while(1) {
        preempt_force();
    }
}
