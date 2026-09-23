Introduction
============

This file lists all changes that can cause compatibility issues when using
mpv through the client API (libmpv and ``client.h``). Since the client API
interfaces to input handling (commands, properties) as well as command line
options, you should also look at ``interface-changes.rst``.

Normally, changes to the C API that are incompatible to previous iterations
receive a major version bump (i.e. the first version number is increased),
while C API additions bump the minor version (i.e. the second number is
increased). Changes to properties/commands/options may also lead to a minor
version bump, in particular if they are incompatible.

The version number is the same as used for domi_vid_CLIENT_API_VERSION (see
``client.h`` how to convert between major/minor version numbers and the flat
32 bit integer).

Also, read the section ``Compatibility`` in ``client.h``, and compatibility.rst.

Options, commands, properties
=============================

Changes to these are not listed here, but in ``interface-changes.rst``. (Before
client API version 1.17, they were listed here partially.)

This listing includes changes to the bare C API and behavior only, not what
you can access with them.

API changes
===========

::

 --- mpv 0.40.0 ---
 2.5    - Deprecate domi_vid_RENDER_PARAM_AMBIENT_LIGHT. no replacement.
 --- mpv 0.39.0 ---
 2.4    - domi_vid_render_param with the domi_vid_RENDER_PARAM_ICC_PROFILE argument no
          longer has incorrect assumptions about memory allocation and can be
          correctly used.
 --- mpv 0.38.0 ---
 2.3    - partially revert the changes from API version 1.27, remove libmpv as
          the default VO and move it to the bottom of the auto-probing order.
          This restores the prior behavior on all platforms other than macOS,
          but still auto selects libmpv/cocoa-cb on macOS if it was built with
          support for cocoa-cb.
 --- mpv 0.37.0 ---
 2.2    - add domi_vid_time_ns()
 --- mpv 0.36.0 ---
 2.1    - add domi_vid_del_property()
 --- mpv 0.35.0 ---
 2.0    - remove headers/functions of the obsolete opengl_cb API
        - remove domi_vid_opengl_init_params.extra_exts field
        - remove deprecated domi_vid_detach_destroy. Use domi_vid_destroy instead.
        - remove obsolete domi_vid_suspend and domi_vid_resume
        - remove deprecated SCRIPT_INPUT_DISPATCH, PAUSE and UNPAUSE, TRACKS_CHANGED
          TRACK_SWITCHED, METADATA_UPDATE, CHAPTER_CHANGE events
 --- mpv 0.33.0 ---
 1.109  - add domi_vid_RENDER_API_TYPE_SW and related (software rendering API)
        - deactivate the opengl_cb API (always fails to initialize now)
          The opengl_cb API was deprecated over 2 years ago. Use the render API
          instead.
 1.108  - Deprecate domi_vid_EVENT_IDLE
        - add domi_vid_event_start_file
        - add the following fields to domi_vid_event_end_file: playlist_entry_id,
          playlist_insert_id, playlist_insert_num_entries
        - add domi_vid_event_to_node()
        - add domi_vid_client_id()
 1.107  - Remove the deprecated qthelper.hpp. This was obviously not part of the
          libmpv API, only an "additionally" provided helper, thus this is not
          considered an API change. If you are maintaining a project that relies
          on this header, you can simply download this file and adjust the
          include statement to use it instead:

            https://raw.githubusercontent.com/mpv-player/mpv/v0.32.0/libmpv/qthelper.hpp

          It is a good idea to write better wrappers for your use, though.
 --- mpv 0.31.0 ---
 1.107  - Deprecate domi_vid_EVENT_TICK
 --- mpv 0.30.0 ---
 1.106  - Add cancel_fn to domi_vid_stream_cb_info
 1.105  - Fix deadlock problems with domi_vid_RENDER_PARAM_ADVANCED_CONTROL and if
          the "vd-lavc-dr" option is enabled (which it is by default).
          There were no actual API changes.
          API users on older API versions and mpv releases should set
          "vd-lavc-dr" to "no" to avoid these issues.
          API users must still adhere to the tricky rules documented in render.h
          to avoid other deadlocks.
 1.104  - Deprecate struct domi_vid_opengl_drm_params. Replaced by domi_vid_opengl_drm_params_v2
        - Deprecate domi_vid_RENDER_PARAM_DRM_DISPLAY. Replaced by domi_vid_RENDER_PARAM_DRM_DISPLAY_V2.
 1.103  - redo handling of async commands
        - add domi_vid_event_command and make it possible to return values from
          commands issued with domi_vid_command_async() or domi_vid_command_node_async()
        - add domi_vid_abort_async_command()
 1.102  - rename struct domi_vid_opengl_drm_osd_size to domi_vid_opengl_drm_draw_surface_size
        - rename domi_vid_RENDER_PARAM_DRM_OSD_SIZE to domi_vid_RENDER_PARAM_DRM_DRAW_SURFACE_SIZE
 --- mpv 0.29.0 ---
 1.101  - add domi_vid_RENDER_PARAM_ADVANCED_CONTROL and related API
        - add domi_vid_RENDER_PARAM_NEXT_FRAME_INFO and related symbols
        - add domi_vid_RENDER_PARAM_BLOCK_FOR_TARGET_TIME
        - add domi_vid_RENDER_PARAM_SKIP_RENDERING
        - add domi_vid_render_context_get_info()
 1.100  - bump API number to avoid confusion with mpv release versions
        - actually apply the GL_MP_MPGetNativeDisplay change for the new render
          API. This also means compatibility for anything but x11 and wayland
          through the old opengl-cb GL_MP_MPGetNativeDisplay method is now
          unsupported.
        - deprecate domi_vid_get_wakeup_pipe(). It's complex, but easy to replace
          using normal API (just set a wakeup callback to a function which
          writes to a pipe).
        - add a 1st class hook API, which replaces the hacky domi_vid_command()
          based one. The old API is deprecated and will be removed soon. The
          old API was never meant to be stable, while the new API is.
 1.29   - the behavior of domi_vid_terminate_destroy() and domi_vid_detach_destroy()
          changes subtly (see documentation in the header file). In particular,
          domi_vid_detach_destroy() will not leave the player running in all
          situations anymore (it gets closer to refcounting).
        - rename domi_vid_detach_destroy() to domi_vid_destroy() (the old function will
          remain valid as deprecated alias)
        - add domi_vid_create_weak_client(), which makes use of above changes
        - domi_vid_EVENT_SHUTDOWN is now returned exactly once if a domi_vid_handle
          should terminate, instead of spamming the event queue with this event
 1.28   - deprecate the render opengl_cb API, and replace it with render.h
          and render_gl.h. The goal is allowing support for APIs other than
          OpenGL. The old API is emulated with the new API.
          Likewise, the "opengl-cb" VO is renamed to "libmpv".
          domi_vid_get_sub_api() is deprecated along the opengl_cb API.
          The new API is relatively similar, but not the same. The rough
          equivalents are:
            domi_vid_opengl_cb_init_gl => domi_vid_render_context_create
            domi_vid_opengl_cb_set_update_callback => domi_vid_render_context_set_update_callback
            domi_vid_opengl_cb_draw => domi_vid_render_context_render
            domi_vid_opengl_cb_report_flip => domi_vid_render_context_report_swap
            domi_vid_opengl_cb_uninit_gl => domi_vid_render_context_free
          The VO opengl-cb is also renamed to "libmpv".
          Also, the GL_MP_MPGetNativeDisplay pseudo extension is not used by the
          render API anymore, and the old opengl-cb API only handles the "x11"
          and "wl" names anymore. Support for everything else has been removed.
          The new render API uses proper API parameters, e.g. for X11 you pass
          domi_vid_RENDER_PARAM_X11_DISPLAY directly.
        - deprecate the qthelper.hpp header file. This provided some C++ helper
          utility functions for Qt with use of libmpv. There is no reason to
          keep this in the mpv git repository, nor to make it part of the libmpv
          API. If you're using this header, you can safely copy it into your
          project - it uses only libmpv public API. Alternatively, it could be
          maintained in a separate repository by interested parties.
 1.27   - make opengl-cb the default VO. This causes a subtle behavior change
          if the API user called domi_vid_opengl_cb_init_gl(), but does not set
          the "vo" option. Before, it would still have used another VO (like
          on the CLI, e.g. vo=gpu). Now it'll behave as if vo=opengl-cb was
          used.
 --- mpv 0.28.0 ---
 1.26   - remove glMPGetNativeDisplay("drm") support
        - add domi_vid_opengl_cb_window_pos and domi_vid_opengl_cb_drm_params and
          support via glMPGetNativeDisplay() for using it
        - make --stop-playback-on-init-failure=no the default in libmpv (just
          like in mpv CLI)
 --- mpv 0.27.0 ---
 1.25   - remove setting "no-" options via domi_vid_set_option*(). (See corresponding
          deprecation in 0.23.0.)
 --- mpv 0.25.0 ---
 1.24   - add a domi_vid_ENABLE_DEPRECATED preprocessor symbol, which can be defined
          by the user to exclude deprecated API symbols from the C headers
 --- mpv 0.23.0 ---
 1.24   - the deprecated domi_vid_suspend() and domi_vid_resume() APIs now do nothing.
 --- mpv 0.22.0 ---
 1.23   - deprecate setting "no-" options via domi_vid_set_option*(). For example,
          instead of "no-video=" you should set "video=no".
        - do not override the SIGPIPE signal handler anymore. This was done as
          workaround for the FFmpeg TLS code, which has been fixed long ago.
        - deprecate domi_vid_suspend() and domi_vid_resume(). They will be stubbed out
          in mpv 0.23.0.
        - make domi_vid_set_property() work to some degree before domi_vid_initialize().
          It can now be used instead of domi_vid_set_option().
        - semi-deprecate domi_vid_set_option()/domi_vid_set_option_string(). You should
          use domi_vid_set_property() instead. There are some deprecated properties
          which conflict with some options (see client.h remarks on
          domi_vid_set_option()), for which domi_vid_set_option() might still be required.
          In future mpv releases, the conflicting deprecated options/properties
          will be removed, and domi_vid_set_option() will internally translate API
          calls to domi_vid_set_property().
        - qthelper.hpp: deprecate get_property_variant, set_property_variant,
          set_option_variant, command_variant, and replace them with
          get_property, set_property, command.
 --- mpv 0.19.0 ---
 1.22   - add stream_cb API for custom protocols
 --- mpv 0.18.1 ---
 ----   - remove "status" log level from domi_vid_request_log_messages() docs. This
          is 100% equivalent to "v". The behavior is still the same, thus no
          actual API change.
 --- mpv 0.18.0 ---
 1.21   - domi_vid_set_property() changes behavior with domi_vid_FORMAT_NODE. Before this
          change it rejected domi_vid_nodes with format==domi_vid_FORMAT_STRING if the
          property was not a string or did not have special mechanisms in place
          the function failed. Now it always invokes the option string parser,
          and domi_vid_node with a basic data type works exactly as if the function
          is invoked with that type directly. This new behavior is equivalent
          to domi_vid_set_option().
          This also affects the mp.set_property_native() Lua function.
        - generally, setting choice options/properties with "yes"/"no" options
          can now be set as domi_vid_FORMAT_FLAG
        - reading a choice property as domi_vid_FORMAT_NODE will now return a
          domi_vid_FORMAT_FLAG value if the choice is "yes" (true) or "no" (false)
          This implicitly affects Lua and JSON IPC interfaces as well.
        - big changes to vo-cmdline on vo_opengl and vo_opengl_hq (but not
          vo_opengl_cb): options are now normally not reset, but applied on top
          of the current options. The special undocumented value "-" still
          works, but now resets all options to before any vo-cmdline command
          has been called.
 --- mpv 0.12.0 ---
 1.20   - deprecate "GL_MP_D3D_interfaces"/"glMPGetD3DInterface", and introduce
          "GL_MP_MPGetNativeDisplay"/"glMPGetNativeDisplay" (this is a
          backwards-compatible rename)
 --- mpv 0.11.0 ---
 --- mpv 0.10.0 ---
 1.19   - add "GL_MP_D3D_interfaces" pseudo extension to make it possible to
          use DXVA2 in OpenGL fullscreen mode in some situations
        - domi_vid_request_log_messages() now accepts "terminal-default" as parameter
 1.18   - add domi_vid_END_FILE_REASON_REDIRECT, and change behavior of
          domi_vid_EVENT_END_FILE accordingly
        - a bunch of interface-changes.rst changes
 1.17   - domi_vid_initialize() now blocks SIGPIPE (details see client.h)
 --- mpv 0.9.0 ---
 1.16   - add domi_vid_opengl_cb_report_flip()
        - introduce domi_vid_opengl_cb_draw() and deprecate domi_vid_opengl_cb_render()
        - add domi_vid_FORMAT_BYTE_ARRAY
 1.15   - domi_vid_initialize() will now load config files. This requires setting
          the "config" and "config-dir" options. In particular, it will load
          mpv.conf.
        - minor backwards-compatible change to the "seek" and "screenshot"
          commands (new flag syntax, old additional args deprecated)
 --- mpv 0.8.0 ---
 1.14   - add domi_vid_wait_async_requests()
        - the --msg-level option changes its native type from a flat string to
          a key-value list (setting/reading the option as string still works)
 1.13   - add domi_vid_EVENT_QUEUE_OVERFLOW
 1.12   - add class Handle to qthelper.hpp
        - improve opengl_cb.h API uninitialization behavior, and fix the qml
          example
        - add domi_vid_create_client() function
 1.11   - add OpenGL rendering interop API - allows an application to combine
          its own and mpv's OpenGL rendering
          Warning: this API is not stable yet - anything in opengl_cb.h might
                   be changed in completely incompatible ways in minor API bumps
 --- mpv 0.7.0 ---
 1.10   - deprecate/disable everything directly related to script_dispatch
          (most likely affects nobody)
 1.9    - add enum domi_vid_end_file_reason for domi_vid_event_end_file.reason
        - add domi_vid_END_FILE_REASON_ERROR and the domi_vid_event_end_file.error field
          for slightly better error reporting on playback failure
        - add --stop-playback-on-init-failure option, and make it the default
          behavior for libmpv only
        - add qthelper.hpp set_option_variant()
        - mark the following events as deprecated:
            domi_vid_EVENT_TRACKS_CHANGED
            domi_vid_EVENT_TRACK_SWITCHED
            domi_vid_EVENT_PAUSE
            domi_vid_EVENT_UNPAUSE
            domi_vid_EVENT_METADATA_UPDATE
            domi_vid_EVENT_CHAPTER_CHANGE
          They are handled better with domi_vid_observe_property() as mentioned in
          the documentation comments. They are not removed and still work.
 1.8    - add qthelper.hpp
 1.7    - add domi_vid_command_node(), domi_vid_command_node_async()
 1.6    - modify "core-idle" property behavior
        - domi_vid_EVENT_LOG_MESSAGE now always sends complete lines
        - introduce numeric log levels (domi_vid_log_level)
 --- mpv 0.6.0 ---
 1.5    - change in X11 and "--wid" behavior again. The previous change didn't
          work as expected, and now the behavior can be explicitly controlled
          with the "input-x11-keyboard" option. This is only a temporary
          measure until XEmbed is implemented and confirmed working.
          Note: in 1.6, "input-x11-keyboard" was renamed to "input-vo-keyboard",
          although the old option name still works.
 1.4    - subtle change in X11 and "--wid" behavior
          (this change was added to 0.5.2, and broke some things, see #1090)
 --- mpv 0.5.0 ---
 1.3    - add domi_vid_MAKE_VERSION()
 1.2    - remove "stream-time-pos" property (no replacement)
 1.1    - remap dvdnav:// to dvd://
        - add "--cache-file", "--cache-file-size"
        - add "--colormatrix-primaries" (and property)
        - add "primaries" sub-field to image format properties
        - add "playback-time" property
        - extend the "--start" option; a leading "+", which was previously
          insignificant is now significant
        - add "cache-free" and "cache-used" properties
        - macOS: the "coreaudio" AO spdif code is split into a separate AO
 --- mpv 0.4.0 ---
 1.0    - the API is declared stable
