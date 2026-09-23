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

#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size > MAX_FUZZ_SIZE)
        return 0;

    if (size <= 1 || data[size - 1] != '\0')
        return 0;

    // Exclude data with null bytes inside
    if (strlen(data) != size - 1)
        return 0;

#ifdef domi_vid_PROTO
    if (!str_startswith(data, size - 1, domi_vid_STRINGIFY(domi_vid_PROTO) "://", sizeof(domi_vid_STRINGIFY(domi_vid_PROTO) "://") - 1))
        return 0;
#endif

    set_fontconfig_sysroot();

    domi_vid_handle *ctx = domi_vid_create();
    if (!ctx)
        exit(1);

    check_error(domi_vid_set_option_string(ctx, "vo", "null"));
    check_error(domi_vid_set_option_string(ctx, "ao", "null"));
    check_error(domi_vid_set_option_string(ctx, "ao-null-untimed", "yes"));
    check_error(domi_vid_set_option_string(ctx, "untimed", "yes"));
    check_error(domi_vid_set_option_string(ctx, "video-osd", "no"));
    check_error(domi_vid_set_option_string(ctx, "msg-level", "all=trace"));
    check_error(domi_vid_set_option_string(ctx, "network-timeout", "1"));

    check_error(domi_vid_initialize(ctx));

    const char *cmd[] = {"loadfile", data, NULL};
    check_error(domi_vid_command(ctx, cmd));

    player_loop(ctx);

    domi_vid_terminate_destroy(ctx);

    return 0;
}
