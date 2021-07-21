#include "wasm3.h"
#include "../srv.h"
#include "string.h"
#include "stdio.h"

struct exec_m3_t {
    EXEC_ENGINE handle;
    IM3Environment env;
};

m3ApiRawFunction(m3_srv_send);
EXEC_INST* m3_srv_load(EXEC_ENGINE* self, PROGRAM* p) {

    IM3Environment env = ((struct exec_m3_t*)self)->env;
    IM3Runtime runtime = m3_NewRuntime(env, 2048, p);
    if (!runtime) { puts("m3_load: failed to load runtime"); return NULL; }

    M3Result res;
    IM3Module mod;
    res = m3_ParseModule(env, &mod, p->code, p->code_size);
    if (res) goto err;

    m3_FixStart(mod, "_start");

    res = m3_LoadModule(runtime, mod);
    if (res) { m3_FreeModule(mod); goto err; }

    res = m3_LinkRawFunction(mod, "srv", "send", "i(iii)", m3_srv_send);
    if (res) goto err;

    // Check exports
    IM3Function f;
    res = m3_FindFunction(&f, runtime, SRV_PACKET_ALOC);
    if (res) goto err;
    res = "'" SRV_PACKET_ALOC "' function should return a i32 value";
    if (m3_GetRetCount(f) != 1) goto err;
    if (m3_GetRetType(f, 0) != c_m3Type_i32) goto err;
    if (m3_GetArgCount(f)) { res = "'" SRV_PACKET_ALOC "' function should not take argument"; goto err; }

    res = m3_FindFunction(&f, runtime, SRV_PACKET_HNDL);
    if (res) goto err;
    res = "'" SRV_PACKET_HNDL "' function should return a i32 value";
    if (m3_GetRetCount(f) != 1) goto err;
    if (m3_GetRetType(f, 0) != c_m3Type_i32) goto err;
    res = "'" SRV_PACKET_HNDL "' function should take a i32 value";
    if (m3_GetArgCount(f) != 1) goto err;
    if (m3_GetArgType(f, 0) != c_m3Type_i32) goto err;

    res = m3_RunStart(mod);
    if (res) goto err;

    return (EXEC_INST*)runtime;

err:
    printf("m3_load: %s\n", res);
    m3_FreeRuntime(runtime);
    return NULL;
}

m3ApiRawFunction(m3_srv_send) {
    m3ApiReturnType(int32_t);
    m3ApiGetArgMem(const char*, path);
    m3ApiGetArgMem(const uint8_t*, data);
    m3ApiGetArg(uint32_t, len);
    m3ApiCheckMem(path, strlen(path));
    m3ApiCheckMem(data, len);
    PROGRAM *p = m3_GetUserData(runtime);
    m3ApiReturn(p ? srv_send(path, data, len, p) : -2);
}

int m3_srv_call(EXEC_INST* inst, const char* sub, const uint8_t *data, size_t len) {
    IM3Runtime runtime = (IM3Runtime)inst;
    void* _mem = m3_GetMemory(runtime, NULL, 0);

    const uint32_t sublen = strlen(sub);
    const uint32_t subsize = sublen + 1;

    IM3Function faloc, fhndl;
    // NOTE: functions are checked during get_instance
    m3_FindFunction(&faloc, runtime, SRV_PACKET_ALOC);
    m3_FindFunction(&fhndl, runtime, SRV_PACKET_HNDL);

    M3Result res;
    uint32_t ret = 0;
    const void *retptr = &ret;
    size_t sent = 0;
    while (sent < len) {
        // Allocate packet
        res = m3_Call(faloc, 0, NULL);
        if (res) goto err;
        uint32_t offset;
        const void *offptr = &offset;
        res = m3_GetResults(faloc, 1, &offptr);
        if (res) goto err;
        // Check packet
        if (!offset) return -4;
        srv_packet_t *packet = m3ApiOffsetToPtr(offset);
        if (m3ApiInvalidMem(packet, 2*sizeof(uint32_t))) return -4;
        size_t p_size = SRV_PACKET_SIZE(*packet);
        if (p_size <= sublen+1) return -4;
        if (m3ApiInvalidMem(packet, p_size)) return -4;
        // Write packet
        packet->sublen = sublen;
        memcpy(&packet->buf[0], sub, subsize);
        packet->datalen = p_size - subsize > len - sent ?
            len - sent : p_size - subsize;
        memcpy(&packet->buf[subsize], data + sent, packet->datalen);
        sent += packet->datalen;
        // Send packet
        res = m3_Call(fhndl, 1, &offptr);
        if (res) goto err;
        res = m3_GetResults(fhndl, 1, &retptr);
        if (res) goto err;
        if (ret < 0) return ret;
    }

    return ret;
err:
    printf("m3_call: %s\n", res);
    return -4;
}

struct exec_m3_t exec_m3 = {0};
EXEC_ENGINE* exec_load_m3() {
    if (exec_m3.env) return &exec_m3.handle;

    exec_m3.env = m3_NewEnvironment();
    if (!exec_m3.env) return NULL;

    exec_m3.handle.srv_load = m3_srv_load;
    exec_m3.handle.srv_call = m3_srv_call;

    return &exec_m3.handle;
}
