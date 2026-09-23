#pragma once

#include "video/out/libmpv.h"

struct ra_tex;

struct libdomi_vid_gpu_context {
    struct domi_vid_global *global;
    struct mp_log *log;
    const struct libdomi_vid_gpu_context_fns *fns;

    struct ra_ctx *ra_ctx;
    void *priv;
};

// Manage backend specific interaction between libmpv and ra backend, that can't
// be managed by ra itself (initialization and passing FBOs).
struct libdomi_vid_gpu_context_fns {
    // The libmpv API type name, see domi_vid_RENDER_PARAM_API_TYPE.
    const char *api_name;
    // Pretty much works like render_backend_fns.init, except that the
    // API type is already checked by the caller.
    // Successful init must set ctx->ra.
    int (*init)(struct libdomi_vid_gpu_context *ctx, domi_vid_render_param *params);
    // Wrap the surface passed to domi_vid_render_context_render() (via the params
    // array) into a ra_tex and return it. Returns a libmpv error code, and sets
    // *out to a temporary object on success. The returned object is valid until
    // another wrap_fbo() or done_frame() is called.
    // This does not need to care about generic attributes, like flipping.
    int (*wrap_fbo)(struct libdomi_vid_gpu_context *ctx, domi_vid_render_param *params,
                    struct ra_tex **out);
    // Signal that the ra_tex object obtained with wrap_fbo is no longer used.
    // For certain backends, this might also be used to signal the end of
    // rendering (like OpenGL doing weird crap).
    void (*done_frame)(struct libdomi_vid_gpu_context *ctx, bool ds);
    // Free all data in ctx->priv.
    void (*destroy)(struct libdomi_vid_gpu_context *ctx);
};

extern const struct libdomi_vid_gpu_context_fns libdomi_vid_gpu_context_gl;
