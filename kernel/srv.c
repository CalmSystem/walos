#include "string.h"
#include "srv.h"

SERVICE_TABLE table;
EXEC_ENGINE *engine;

void srv_use(SERVICE_TABLE st, EXEC_ENGINE* eg) {
    table = st;
    engine = eg;
}

SERVICE* srv_find(const char* name) {
    for (SERVICE* s = table.ptr; s->program; s++) {
        if (strcmp(name, s->name) == 0) return s;
    }
    return NULL;
}
SERVICE *srv_findn(const char *path, size_t name_len) {
    for (SERVICE* s = table.ptr; s->program; s++) {
        if (strncmp(path, s->name, name_len) == 0 &&
            strlen(s->name) == name_len) return s;
    }
    return NULL;
}

int srv_add(const char* name, PROGRAM* program, SERVICE** out) {
    if (srv_find(name)) return -1;
    if (table.free_services == 0) return -2;

    SERVICE* s = table.ptr;
    while(s->program) s++;

    table.free_services--;
    s->name = name;
    s->program = program;
    if (out) *out = s;
    return program->code_size;
}

int srv_send(const char *path, const uint8_t *data, size_t len) {
    char *sep = strchr(path, SRV_SEPARATOR);
    if (!sep) return -2;

    SERVICE* srv = srv_findn(path, sep-path);
    if (!srv) return -1;

    if (!srv->instance) srv->instance = engine->load(engine, srv->program, X_START);
    if (!srv->instance || (uint64_t)srv->instance == UINT64_MAX) {
        srv->instance = (EXEC_INST*)UINT64_MAX;
        return -3;
    }

    //FIXME: bad type cast...
    uint32_t argv[3] = {(uint32_t)(uint64_t)sep+1, (uint32_t)(uint64_t)data, len};
    int res = engine->call(engine, srv->instance, "handle", 3, argv);
    if (res < 0) return -4;

    return 1;
}
