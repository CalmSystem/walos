#include "string.h"
#include "srv.h"
#include "errno.h"

SERVICE_TABLE table;
EXEC_ENGINE *engine;

void srv_setup(SERVICE_TABLE st, EXEC_ENGINE* eg) {
    table = st;
    engine = eg;
}

#define BAD_EXEC_INST (EXEC_INST*)UINT64_MAX
#define INTERNAL_PROG (PROGRAM*)UINT64_MAX

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

int _srv_add(const char* name, PROGRAM* program, EXEC_INST* inst, SERVICE** out) {
    if (srv_find(name)) return -1;
    if (table.free_services == 0) return -2;

    SERVICE* s = table.ptr;
    while(s->program) s++;

    table.free_services--;
    s->name = name;
    s->program = program;
    s->instance = inst;
    if (out) *out = s;
    return 1;
}
int srv_add(const char* name, PROGRAM* program, SERVICE** out) {
    return _srv_add(name, program, NULL, out);
}
int srv_add_internal(const char* name, ssize_t (*fn)(const char*, struct iovec*, size_t), SERVICE** out) {
    return _srv_add(name, INTERNAL_PROG, fn, out);
}

static inline int has_right(const PROGRAM_RIGHTS* p, const char* srv, size_t srvlen, const char* path) {
    for (size_t i = 0; i < p->count; i++) {
        PROGRAM_RIGHT* r = p->ptr+i;
        if (strncmp(r->service, srv, srvlen) == 0 &&
            strlen(r->service) == srvlen &&
            strcmp(r->path, path) == 0)
            return 1;
    }
    return -1;
}

ssize_t service_send(const char *path, struct iovec *iovec, size_t iovcnt, const PROGRAM_RIGHTS* rights) {
    char* sep = strchr(path, SRV_SEPARATOR);
    if (sep == NULL) return -2;

    uint32_t srvlen = sep-path;
    if (rights && !has_right(rights, path, srvlen, sep+1))
        return -2;

    SERVICE* srv = srv_findn(path, srvlen);
    if (srv == NULL) return -1;

    if (srv->program == INTERNAL_PROG) {
        if (srv->instance == NULL) return -3;
        return ((ssize_t (*)(const char*, struct iovec*, size_t))srv->instance)(sep+1, iovec, iovcnt);
    } else {
        if (srv->instance == NULL) {
            srv->instance = engine->srv_load(engine, srv->program);
            if (srv->instance == NULL) srv->instance = BAD_EXEC_INST;
        }
        if (srv->instance == BAD_EXEC_INST) return -3;

        return engine->srv_call(srv->instance, sep+1, iovec, iovcnt);
    }
}
