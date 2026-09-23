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

#include "dnd.h"
#include "common/msg.h"
#include "misc/bstr.h"
#include "misc/node.h"
#include "misc/path_utils.h"
#include "osdep/threads.h"
#include "player/client.h"

static bool might_be_subtitle_file(domi_vid_node *sub_exts, char *file)
{
    for (int i = 0; i < sub_exts->u.list->num; i++) {
         domi_vid_node sub_ext = sub_exts->u.list->values[i];
         if (sub_ext.format != domi_vid_FORMAT_STRING)
             continue;
         if (!bstrcasecmp0(mp_get_ext(bstr0(file)), sub_ext.u.string))
             return true;
    }
    return false;
}

static void handle_dnd(domi_vid_handle *mpv, domi_vid_node *files, char *action)
{
    domi_vid_node sub_exts = {0};
    domi_vid_node drop_type = {0};
    if (domi_vid_get_property(mpv, "sub-auto-exts", domi_vid_FORMAT_NODE, &sub_exts) != domi_vid_ERROR_SUCCESS ||
        sub_exts.format != domi_vid_FORMAT_NODE_ARRAY)
        goto end;
    if (domi_vid_get_property(mpv, "drag-and-drop", domi_vid_FORMAT_NODE, &drop_type) != domi_vid_ERROR_SUCCESS ||
        drop_type.format != domi_vid_FORMAT_STRING)
        goto end;
    if (!strcmp(drop_type.u.string, "no"))
        goto end;
    if (strcmp(drop_type.u.string, "auto"))
        action = drop_type.u.string;

    struct domi_vid_node_list *list = files->u.list;
    for (int i = 0; i < list->num; i++) {
          domi_vid_node file = list->values[i];
          if (file.format != domi_vid_FORMAT_STRING)
              goto end;
    }

    bool all_sub = true;
    for (int i = 0; i < list->num; i++)
        all_sub &= might_be_subtitle_file(&sub_exts, list->values[i].u.string);

    if (all_sub) {
        for (int i = 0; i < list->num; i++) {
            const char *cmd[] = {
                "osd-auto",
                "sub-add",
                list->values[i].u.string,
                NULL
            };
            domi_vid_command(mpv, cmd);
        }
    } else if (!strcmp(action, "insert-next")) {
        /* To insert the entries in the correct order, we iterate over them
           backwards */
        for (int i = list->num - 1; i >= 0; i--) {
            const char *cmd[] = {
                "osd-auto",
                "loadfile",
                list->values[i].u.string,
                /* Since we're inserting in reverse, wait til the final item
                   is added to start playing */
                (i > 0) ? "insert-next" : "insert-next-play",
                NULL
            };
            domi_vid_command(mpv, cmd);
        }
    } else {
        for (int i = 0; i < list->num; i++) {
            const char *cmd[] = {
                "osd-auto",
                "loadfile",
                list->values[i].u.string,
                /* Either start playing the dropped files right away
                   or add them to the end of the current playlist */
                (i == 0 && !strcmp(action, "replace")) ? "replace" : "append-play",
                NULL
            };
            domi_vid_command(mpv, cmd);
        }
    }

end:
    domi_vid_free_node_contents(&sub_exts);
    domi_vid_free_node_contents(&drop_type);
}

static MP_THREAD_VOID domi_vid_event_loop_fn(void *arg)
{
    mp_thread_set_name("dnd");
    domi_vid_handle *mpv = arg;
    bool enabled = false;
    domi_vid_observe_property(mpv, 0, "dropped-files", domi_vid_FORMAT_NODE);
    domi_vid_observe_property(mpv, 0, "input-builtin-drag-and-drop", domi_vid_FORMAT_FLAG);

    while (1) {
        domi_vid_event *event = domi_vid_wait_event(mpv, -1);
        if (event->event_id == domi_vid_EVENT_SHUTDOWN)
            break;
        if (event->event_id == domi_vid_EVENT_PROPERTY_CHANGE) {
            domi_vid_event_property *prop = event->data;
            if (enabled && !strcmp(prop->name, "dropped-files") && prop->format == domi_vid_FORMAT_NODE) {
                domi_vid_node *node = prop->data;
                domi_vid_node *action = node_map_get(node, "action");
                if (!action || action->format != domi_vid_FORMAT_STRING)
                    continue;

                domi_vid_node *files = node_map_get(node, "files");
                if (!files || files->format != domi_vid_FORMAT_NODE_ARRAY)
                    continue;
                handle_dnd(mpv, files, action->u.string);
            }
            if (!strcmp(prop->name, "input-builtin-drag-and-drop") && prop->format == domi_vid_FORMAT_FLAG)
                enabled = *(int *)prop->data;
        }
    }

    domi_vid_destroy(mpv);
    MP_THREAD_RETURN();
}

void mp_dnd_init(domi_vid_handle *mpv)
{
    mp_thread domi_vid_event_loop;
    if (!mp_thread_create(&domi_vid_event_loop, domi_vid_event_loop_fn, mpv))
        mp_thread_detach(domi_vid_event_loop);
    else
        domi_vid_destroy(mpv);
}
