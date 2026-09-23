#include <stddef.h>

#include "input/input.h"

struct mp_ipc_ctx *mp_init_ipc(struct mp_client_api *client_api,
                               struct domi_vid_global *global)
{
    return NULL;
}

bool mp_ipc_start_anon_client(struct mp_ipc_ctx *ctx, struct domi_vid_handle *h,
                              int out_fd[2])
{
    return false;
}

void mp_uninit_ipc(struct mp_ipc_ctx *ctx)
{
}
