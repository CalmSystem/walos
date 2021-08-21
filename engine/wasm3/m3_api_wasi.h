//
//  m3_api_wasi.h
//
//  Created by Volodymyr Shymanskyy on 11/20/19.
//  Copyright © 2019 Volodymyr Shymanskyy. All rights reserved.
//

#ifndef m3_api_wasi_h
#define m3_api_wasi_h

#include "m3_core.h"

d_m3BeginExternC

typedef struct m3_wasi_context_t
{
    i32                     exit_code;
    u32                     argc;
    ccstr_t *               argv;
} m3_wasi_context_t;

M3Result    m3_LinkWASI     (IM3Module io_module);

m3_wasi_context_t* m3_GetWasiContext(IM3Runtime io_runtime);

m3ApiRawFunction(m3_srv_send);
m3ApiRawFunction(m3_srv_sendv);

d_m3EndExternC

#endif // m3_api_wasi_h
