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

#include "libdomi_vid_common.h"

static void test_file_loading(char *file)
{
    const char *cmd[] = {"loadfile", file, NULL};
    command(cmd);
    bool loaded = false;
    bool finished = false;
    while (!finished) {
        domi_vid_event *event = wrap_wait_event();
        switch (event->event_id) {
        case domi_vid_EVENT_FILE_LOADED:
            // make sure it loads before exiting
            loaded = true;
            break;
        case domi_vid_EVENT_END_FILE:
            if (loaded)
                finished = true;
            break;
        }
    }
    if (!finished)
        fail("Unable to load test file!\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        return 1;

    ctx = domi_vid_create();
    if (!ctx)
        return 1;

    atexit(exit_cleanup);

    initialize();

    const char *fmt = "================ TEST: %s ================\n";
    printf(fmt, "test_file_loading");
    test_file_loading(argv[1]);
    printf("================ SHUTDOWN ================\n");

    command_string("quit");
    while (wrap_wait_event()->event_id != domi_vid_EVENT_SHUTDOWN) {}

    return 0;
}
