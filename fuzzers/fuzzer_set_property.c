/*
 * This file is part of mpv.
 *
 * mpv is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * mpv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with mpv.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size > MAX_FUZZ_SIZE)
        return 0;

    size_t value_len;
    switch (domi_vid_FORMAT)
    {
    case domi_vid_FORMAT_STRING:
        value_len = strnlen(data, size);
        if (!value_len || value_len == size)
            return 0;
        value_len += 1;
        break;
    case domi_vid_FORMAT_FLAG:
        value_len = sizeof(int);
        break;
    case domi_vid_FORMAT_INT64:
        value_len = sizeof(int64_t);
        break;
    case domi_vid_FORMAT_DOUBLE:
        value_len = sizeof(double);
        break;
    default:
        exit(1);
        break;
    }

    // at least two bytes for the name
    if (size < value_len + 2)
        return 0;

    const char *name = (const char *)data + value_len;
    size_t name_len = strnlen(name, size - value_len);
    if (!name_len || name_len != size - value_len - 1)
        return 0;

    set_fontconfig_sysroot();

    domi_vid_handle *ctx = domi_vid_create();
    if (!ctx)
        exit(1);

    check_error(domi_vid_set_option_string(ctx, "msg-level", "all=trace"));
    check_error(domi_vid_set_option_string(ctx, "network-timeout", "1"));

#if domi_vid_RUN
    check_error(domi_vid_set_option_string(ctx, "vo", "null"));
    check_error(domi_vid_set_option_string(ctx, "ao", "null"));

    check_error(domi_vid_initialize(ctx));
#endif

    int ret;
    if (domi_vid_FORMAT == domi_vid_FORMAT_STRING) {
        ret = domi_vid_set_property_string(ctx, name, (void *)data);
    } else {
        ret = domi_vid_set_property(ctx, name, domi_vid_FORMAT, (void *)data);
    }

    if (ret != domi_vid_ERROR_SUCCESS)
        goto done;

#if domi_vid_RUN
    check_error(domi_vid_set_option_string(ctx, "ao-null-untimed", "yes"));
    check_error(domi_vid_set_option_string(ctx, "loop-file", "no"));
    check_error(domi_vid_set_option_string(ctx, "loop-playlist", "no"));
    check_error(domi_vid_set_option_string(ctx, "pause", "no"));
    check_error(domi_vid_set_option_string(ctx, "untimed", "yes"));
    check_error(domi_vid_set_option_string(ctx, "sstep", "0"));
    check_error(domi_vid_set_option_string(ctx, "idle", "yes"));

    domi_vid_node node = {
        .format = domi_vid_FORMAT_NODE_ARRAY,
        .u.list = &(domi_vid_node_list) {
            .num = 1,
            .values = &(domi_vid_node) {
                .format = domi_vid_FORMAT_STRING,
                .u.string = "av://lavfi:sine=d=0.1",
            },
        },
    };
    check_error(domi_vid_set_option(ctx, "audio-files", domi_vid_FORMAT_NODE, &node));

    node.u.list->num = 0;
    check_error(domi_vid_set_option(ctx, "cover-art-files", domi_vid_FORMAT_NODE, &node));
    check_error(domi_vid_set_option(ctx, "external-files", domi_vid_FORMAT_NODE, &node));
    check_error(domi_vid_set_option(ctx, "sub-files", domi_vid_FORMAT_NODE, &node));

    const char *cmd[] = {"loadfile", "av://lavfi:yuvtestsrc=d=0.1", NULL};
    check_error(domi_vid_command(ctx, cmd));

    player_loop(ctx);
#endif

done:
    domi_vid_terminate_destroy(ctx);

    return 0;
}
