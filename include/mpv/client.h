/* Copyright (C) 2017 the mpv developers
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Note: the client API is licensed under ISC (see above) to enable
 * other wrappers outside of mpv. But keep in mind that the
 * mpv core is by default still GPLv2+ - unless built with
 * -Dgpl=false, which makes it LGPLv2+.
 */

#ifndef domi_vid_CLIENT_API_H_
#define domi_vid_CLIENT_API_H_

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#define domi_vid_EXPORT __declspec(dllexport)
#define domi_vid_SELECTANY __declspec(selectany)
#elif defined(__GNUC__) || defined(__clang__)
#define domi_vid_EXPORT __attribute__((visibility("default")))
#define domi_vid_SELECTANY
#else
#define domi_vid_EXPORT
#define domi_vid_SELECTANY
#endif

#ifdef __cpp_decltype
#define domi_vid_DECLTYPE decltype
#else
#define domi_vid_DECLTYPE __typeof__
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Mechanisms provided by this API
 * -------------------------------
 *
 * This API provides general control over mpv playback. It does not give you
 * direct access to individual components of the player, only the whole thing.
 * It's somewhat equivalent to MPlayer's slave mode. You can send commands,
 * retrieve or set playback status or settings with properties, and receive
 * events.
 *
 * The API can be used in two ways:
 * 1) Internally in mpv, to provide additional features to the command line
 *    player. Lua scripting uses this. (Currently there is no plugin API to
 *    get a client API handle in external user code. It has to be a fixed
 *    part of the player at compilation time.)
 * 2) Using mpv as a library with domi_vid_create(). This basically allows embedding
 *    mpv in other applications.
 *
 * Documentation
 * -------------
 *
 * The libmpv C API is documented directly in this header. Note that most
 * actual interaction with this player is done through
 * options/commands/properties, which can be accessed through this API.
 * Essentially everything is done with them, including loading a file,
 * retrieving playback progress, and so on.
 *
 * These are documented elsewhere:
 *      * http://mpv.io/manual/master/#options
 *      * http://mpv.io/manual/master/#list-of-input-commands
 *      * http://mpv.io/manual/master/#properties
 *
 * You can also look at the examples here:
 *      * https://github.com/mpv-player/mpv-examples/tree/master/libmpv
 *
 * Event loop
 * ----------
 *
 * In general, the API user should run an event loop in order to receive events.
 * This event loop should call domi_vid_wait_event(), which will return once a new
 * mpv client API is available. It is also possible to integrate client API
 * usage in other event loops (e.g. GUI toolkits) with the
 * domi_vid_set_wakeup_callback() function, and then polling for events by calling
 * domi_vid_wait_event() with a 0 timeout.
 *
 * Note that the event loop is detached from the actual player. Not calling
 * domi_vid_wait_event() will not stop playback. It will eventually congest the
 * event queue of your API handle, though.
 *
 * Synchronous vs. asynchronous calls
 * ----------------------------------
 *
 * The API allows both synchronous and asynchronous calls. Synchronous calls
 * have to wait until the playback core is ready, which currently can take
 * an unbounded time (e.g. if network is slow or unresponsive). Asynchronous
 * calls just queue operations as requests, and return the result of the
 * operation as events.
 *
 * Asynchronous calls
 * ------------------
 *
 * The client API includes asynchronous functions. These allow you to send
 * requests instantly, and get replies as events at a later point. The
 * requests are made with functions carrying the _async suffix, and replies
 * are returned by domi_vid_wait_event() (interleaved with the normal event stream).
 *
 * A 64 bit userdata value is used to allow the user to associate requests
 * with replies. The value is passed as reply_userdata parameter to the request
 * function. The reply to the request will have the reply
 * domi_vid_event->reply_userdata field set to the same value as the
 * reply_userdata parameter of the corresponding request.
 *
 * This userdata value is arbitrary and is never interpreted by the API. Note
 * that the userdata value 0 is also allowed, but then the client must be
 * careful not accidentally interpret the domi_vid_event->reply_userdata if an
 * event is not a reply. (For non-replies, this field is set to 0.)
 *
 * Asynchronous calls may be reordered in arbitrarily with other synchronous
 * and asynchronous calls. If you want a guaranteed order, you need to wait
 * until asynchronous calls report completion before doing the next call.
 *
 * See also the section "Asynchronous command details" in the manpage.
 *
 * Multithreading
 * --------------
 *
 * The client API is generally fully thread-safe, unless otherwise noted.
 * Currently, there is no real advantage in using more than 1 thread to access
 * the client API, since everything is serialized through a single lock in the
 * playback core.
 *
 * Basic environment requirements
 * ------------------------------
 *
 * This documents basic requirements on the C environment. This is especially
 * important if mpv is used as library with domi_vid_create().
 *
 * - The LC_NUMERIC locale category must be set to "C". If your program calls
 *   setlocale(), be sure not to use LC_ALL, or if you do, reset LC_NUMERIC
 *   to its sane default: setlocale(LC_NUMERIC, "C").
 * - If a X11 based VO is used, mpv will set the xlib error handler. This error
 *   handler is process-wide, and there's no proper way to share it with other
 *   xlib users within the same process. This might confuse GUI toolkits.
 * - mpv uses some other libraries that are not library-safe, such as Fribidi
 *   (used through libass), ALSA, FFmpeg, and possibly more.
 * - The FPU precision must be set at least to double precision.
 * - On Windows, mpv will call timeBeginPeriod(1).
 * - On memory exhaustion, mpv will kill the process.
 * - In certain cases, mpv may start sub processes (such as with the ytdl
 *   wrapper script).
 * - Using UNIX IPC (off by default) will override the SIGPIPE signal handler,
 *   and set it to SIG_IGN. Some invocations of the "subprocess" command will
 *   also do that.
 * - mpv may start sub processes, so overriding SIGCHLD, or waiting on all PIDs
 *   (such as calling wait()) by the parent process or any other library within
 *   the process must be avoided. libmpv itself only waits for its own PIDs.
 * - If anything in the process registers signal handlers, they must set the
 *   SA_RESTART flag. Otherwise you WILL get random failures on signals.
 *
 * Encoding of filenames
 * ---------------------
 *
 * mpv uses UTF-8 everywhere.
 *
 * On some platforms (like Linux), filenames actually do not have to be UTF-8;
 * for this reason libmpv supports non-UTF-8 strings. libmpv uses what the
 * kernel uses and does not recode filenames. At least on Linux, passing a
 * string to libmpv is like passing a string to the fopen() function.
 *
 * On Windows, filenames are always UTF-8, libmpv converts between UTF-8 and
 * UTF-16 when using win32 API functions. libmpv never uses or accepts
 * filenames in the local 8 bit encoding. It does not use fopen() either;
 * it uses _wfopen().
 *
 * On macOS, filenames and other strings taken/returned by libmpv can have
 * inconsistent unicode normalization. This can sometimes lead to problems.
 * You have to hope for the best.
 *
 * Also see the remarks for domi_vid_FORMAT_STRING.
 *
 * Embedding the video window
 * --------------------------
 *
 * Using the render API (in render.h) is recommended. This API requires
 * you to create and maintain an OpenGL context, to which you can render
 * video using a specific API call. This API does not include keyboard or mouse
 * input directly.
 *
 * There is an older way to embed the native mpv window into your own. You have
 * to get the raw window handle, and set it as "wid" option. This works on X11,
 * win32, and macOS only. It's much easier to use than the render API, but
 * also has various problems.
 *
 * Also see client API examples and the mpv manpage. There is an extensive
 * discussion here:
 * https://github.com/mpv-player/mpv-examples/tree/master/libmpv#methods-of-embedding-the-video-window
 *
 * Compatibility
 * -------------
 *
 * mpv development doesn't stand still, and changes to mpv internals as well as
 * to its interface can cause compatibility issues to client API users.
 *
 * The API is versioned (see domi_vid_CLIENT_API_VERSION), and changes to it are
 * documented in DOCS/client-api-changes.rst. The C API itself will probably
 * remain compatible for a long time, but the functionality exposed by it
 * could change more rapidly. For example, it's possible that options are
 * renamed, or change the set of allowed values.
 *
 * Defensive programming should be used to potentially deal with the fact that
 * options, commands, and properties could disappear, change their value range,
 * or change the underlying datatypes. It might be a good idea to prefer
 * domi_vid_FORMAT_STRING over other types to decouple your code from potential
 * mpv changes.
 *
 * Also see: DOCS/compatibility.rst
 *
 * Future changes
 * --------------
 *
 * This are the planned changes that will most likely be done on the next major
 * bump of the library:
 *
 *  - remove all symbols that are marked as deprecated
 *  - reassign enum numerical values to remove gaps
 *  - disabling all events by default
 */

/**
 * The version is incremented on each API change. The 16 lower bits form the
 * minor version number, and the 16 higher bits the major version number. If
 * the API becomes incompatible to previous versions, the major version
 * number is incremented. This affects only C part, and not properties and
 * options.
 *
 * Every API bump is described in DOCS/client-api-changes.rst
 *
 * You can use domi_vid_MAKE_VERSION() and compare the result with integer
 * relational operators (<, >, <=, >=).
 */
#define domi_vid_MAKE_VERSION(major, minor) (((major) << 16) | (minor) | 0UL)
#define domi_vid_CLIENT_API_VERSION domi_vid_MAKE_VERSION(2, 5)

/**
 * The API user is allowed to "#define domi_vid_ENABLE_DEPRECATED 0" before
 * including any libmpv headers. Then deprecated symbols will be excluded
 * from the headers. (Of course, deprecated properties and commands and
 * other functionality will still work.)
 */
#ifndef domi_vid_ENABLE_DEPRECATED
#define domi_vid_ENABLE_DEPRECATED 1
#endif

/**
 * Return the domi_vid_CLIENT_API_VERSION the mpv source has been compiled with.
 */
domi_vid_EXPORT unsigned long domi_vid_client_api_version(void);

/**
 * Client context used by the client API. Every client has its own private
 * handle.
 */
typedef struct domi_vid_handle domi_vid_handle;

/**
 * List of error codes than can be returned by API functions. 0 and positive
 * return values always mean success, negative values are always errors.
 */
typedef enum domi_vid_error {
    /**
     * No error happened (used to signal successful operation).
     * Keep in mind that many API functions returning error codes can also
     * return positive values, which also indicate success. API users can
     * hardcode the fact that ">= 0" means success.
     */
    domi_vid_ERROR_SUCCESS           = 0,
    /**
     * The event ringbuffer is full. This means the client is choked, and can't
     * receive any events. This can happen when too many asynchronous requests
     * have been made, but not answered. Probably never happens in practice,
     * unless the mpv core is frozen for some reason, and the client keeps
     * making asynchronous requests. (Bugs in the client API implementation
     * could also trigger this, e.g. if events become "lost".)
     */
    domi_vid_ERROR_EVENT_QUEUE_FULL  = -1,
    /**
     * Memory allocation failed.
     */
    domi_vid_ERROR_NOMEM             = -2,
    /**
     * The mpv core wasn't configured and initialized yet. See the notes in
     * domi_vid_create().
     */
    domi_vid_ERROR_UNINITIALIZED     = -3,
    /**
     * Generic catch-all error if a parameter is set to an invalid or
     * unsupported value. This is used if there is no better error code.
     */
    domi_vid_ERROR_INVALID_PARAMETER = -4,
    /**
     * Trying to set an option that doesn't exist.
     */
    domi_vid_ERROR_OPTION_NOT_FOUND  = -5,
    /**
     * Trying to set an option using an unsupported domi_vid_FORMAT.
     */
    domi_vid_ERROR_OPTION_FORMAT     = -6,
    /**
     * Setting the option failed. Typically this happens if the provided option
     * value could not be parsed.
     */
    domi_vid_ERROR_OPTION_ERROR      = -7,
    /**
     * The accessed property doesn't exist.
     */
    domi_vid_ERROR_PROPERTY_NOT_FOUND = -8,
    /**
     * Trying to set or get a property using an unsupported domi_vid_FORMAT.
     */
    domi_vid_ERROR_PROPERTY_FORMAT   = -9,
    /**
     * The property exists, but is not available. This usually happens when the
     * associated subsystem is not active, e.g. querying audio parameters while
     * audio is disabled.
     */
    domi_vid_ERROR_PROPERTY_UNAVAILABLE = -10,
    /**
     * Error setting or getting a property.
     */
    domi_vid_ERROR_PROPERTY_ERROR    = -11,
    /**
     * General error when running a command with domi_vid_command and similar.
     */
    domi_vid_ERROR_COMMAND           = -12,
    /**
     * Generic error on loading (usually used with domi_vid_event_end_file.error).
     */
    domi_vid_ERROR_LOADING_FAILED    = -13,
    /**
     * Initializing the audio output failed.
     */
    domi_vid_ERROR_AO_INIT_FAILED    = -14,
    /**
     * Initializing the video output failed.
     */
    domi_vid_ERROR_VO_INIT_FAILED    = -15,
    /**
     * There was no audio or video data to play. This also happens if the
     * file was recognized, but did not contain any audio or video streams,
     * or no streams were selected.
     */
    domi_vid_ERROR_NOTHING_TO_PLAY   = -16,
    /**
     * When trying to load the file, the file format could not be determined,
     * or the file was too broken to open it.
     */
    domi_vid_ERROR_UNKNOWN_FORMAT    = -17,
    /**
     * Generic error for signaling that certain system requirements are not
     * fulfilled.
     */
    domi_vid_ERROR_UNSUPPORTED       = -18,
    /**
     * The API function which was called is a stub only.
     */
    domi_vid_ERROR_NOT_IMPLEMENTED   = -19,
    /**
     * Unspecified error.
     */
    domi_vid_ERROR_GENERIC           = -20
} domi_vid_error;

/**
 * Return a string describing the error. For unknown errors, the string
 * "unknown error" is returned.
 *
 * @param error error number, see enum domi_vid_error
 * @return A static string describing the error. The string is completely
 *         static, i.e. doesn't need to be deallocated, and is valid forever.
 */
domi_vid_EXPORT const char *domi_vid_error_string(int error);

/**
 * General function to deallocate memory returned by some of the API functions.
 * Call this only if it's explicitly documented as allowed. Calling this on
 * mpv memory not owned by the caller will lead to undefined behavior.
 *
 * @param data A valid pointer returned by the API, or NULL.
 */
domi_vid_EXPORT void domi_vid_free(void *data);

/**
 * Return the name of this client handle. Every client has its own unique
 * name, which is mostly used for user interface purposes.
 *
 * @return The client name. The string is read-only and is valid until the
 *         domi_vid_handle is destroyed.
 */
domi_vid_EXPORT const char *domi_vid_client_name(domi_vid_handle *ctx);

/**
 * Return the ID of this client handle. Every client has its own unique ID. This
 * ID is never reused by the core, even if the domi_vid_handle at hand gets destroyed
 * and new handles get allocated.
 *
 * IDs are never 0 or negative.
 *
 * Some mpv APIs (not necessarily all) accept a name in the form "@<id>" in
 * addition of the proper domi_vid_client_name(), where "<id>" is the ID in decimal
 * form (e.g. "@123"). For example, the "script-message-to" command takes the
 * client name as first argument, but also accepts the client ID formatted in
 * this manner.
 *
 * @return The client ID.
 */
domi_vid_EXPORT int64_t domi_vid_client_id(domi_vid_handle *ctx);

/**
 * Create a new mpv instance and an associated client API handle to control
 * the mpv instance. This instance is in a pre-initialized state,
 * and needs to be initialized to be actually used with most other API
 * functions.
 *
 * Some API functions will return domi_vid_ERROR_UNINITIALIZED in the uninitialized
 * state. You can call domi_vid_set_property() (or domi_vid_set_property_string() and
 * other variants, and before mpv 0.21.0 domi_vid_set_option() etc.) to set initial
 * options. After this, call domi_vid_initialize() to start the player, and then use
 * e.g. domi_vid_command() to start playback of a file.
 *
 * The point of separating handle creation and actual initialization is that
 * you can configure things which can't be changed during runtime.
 *
 * Unlike the command line player, this will have initial settings suitable
 * for embedding in applications. The following settings are different:
 * - stdin/stdout/stderr and the terminal will never be accessed. This is
 *   equivalent to setting the --no-terminal option.
 *   (Technically, this also suppresses C signal handling.)
 * - No config files will be loaded. This is roughly equivalent to using
 *   --config=no. Since libmpv 1.15, you can actually re-enable this option,
 *   which will make libmpv load config files during domi_vid_initialize(). If you
 *   do this, you are strongly encouraged to set the "config-dir" option too.
 *   (Otherwise it will load the mpv command line player's config.)
 *   For example:
 *      domi_vid_set_option_string(mpv, "config-dir", "/my/path"); // set config root
 *      domi_vid_set_option_string(mpv, "config", "yes"); // enable config loading
 *      (call domi_vid_initialize() _after_ this)
 * - Idle mode is enabled, which means the playback core will enter idle mode
 *   if there are no more files to play on the internal playlist, instead of
 *   exiting. This is equivalent to the --idle option.
 * - Disable parts of input handling.
 * - Most of the different settings can be viewed with the command line player
 *   by running "mpv --show-profile=libmpv".
 *
 * All this assumes that API users want a mpv instance that is strictly
 * isolated from the command line player's configuration, user settings, and
 * so on. You can re-enable disabled features by setting the appropriate
 * options.
 *
 * The mpv command line parser is not available through this API, but you can
 * set individual options with domi_vid_set_property(). Files for playback must be
 * loaded with domi_vid_command() or others.
 *
 * Note that you should avoid doing concurrent accesses on the uninitialized
 * client handle. (Whether concurrent access is definitely allowed or not has
 * yet to be decided.)
 *
 * @return a new mpv client API handle. Returns NULL on error. Currently, this
 *         can happen in the following situations:
 *         - out of memory
 *         - LC_NUMERIC is not set to "C" (see general remarks)
 */
domi_vid_EXPORT domi_vid_handle *domi_vid_create(void);

/**
 * Initialize an uninitialized mpv instance. If the mpv instance is already
 * running, an error is returned.
 *
 * This function needs to be called to make full use of the client API if the
 * client API handle was created with domi_vid_create().
 *
 * Only the following options are required to be set _before_ domi_vid_initialize():
 *      - options which are only read at initialization time:
 *        - config
 *        - config-dir
 *        - input-conf
 *        - load-scripts
 *        - script
 *        - player-operation-mode
 *        - input-app-events (macOS)
 *      - all encoding mode options
 *
 * @return error code
 */
domi_vid_EXPORT int domi_vid_initialize(domi_vid_handle *ctx);

/**
 * Disconnect and destroy the domi_vid_handle. ctx will be deallocated with this
 * API call.
 *
 * If the last domi_vid_handle is detached, the core player is destroyed. In
 * addition, if there are only weak domi_vid_handles (such as created by
 * domi_vid_create_weak_client() or internal scripts), these domi_vid_handles will
 * be sent domi_vid_EVENT_SHUTDOWN. This function may block until these clients
 * have responded to the shutdown event, and the core is finally destroyed.
 */
domi_vid_EXPORT void domi_vid_destroy(domi_vid_handle *ctx);

/**
 * Similar to domi_vid_destroy(), but brings the player and all clients down
 * as well, and waits until all of them are destroyed. This function blocks. The
 * advantage over domi_vid_destroy() is that while domi_vid_destroy() merely
 * detaches the client handle from the player, this function quits the player,
 * waits until all other clients are destroyed (i.e. all domi_vid_handles are
 * detached), and also waits for the final termination of the player.
 *
 * Since domi_vid_destroy() is called somewhere on the way, it's not safe to
 * call other functions concurrently on the same context.
 *
 * Since mpv client API version 1.29:
 *  The first call on any domi_vid_handle will block until the core is destroyed.
 *  This means it will wait until other domi_vid_handle have been destroyed. If you
 *  want asynchronous destruction, just run the "quit" command, and then react
 *  to the domi_vid_EVENT_SHUTDOWN event.
 *  If another domi_vid_handle already called domi_vid_terminate_destroy(), this call will
 *  not actually block. It will destroy the domi_vid_handle, and exit immediately,
 *  while other domi_vid_handles might still be uninitializing.
 *
 * Before mpv client API version 1.29:
 *  If this is called on a domi_vid_handle that was not created with domi_vid_create(),
 *  this function will merely send a quit command and then call
 *  domi_vid_destroy(), without waiting for the actual shutdown.
 */
domi_vid_EXPORT void domi_vid_terminate_destroy(domi_vid_handle *ctx);

/**
 * Create a new client handle connected to the same player core as ctx. This
 * context has its own event queue, its own domi_vid_request_event() state, its own
 * domi_vid_request_log_messages() state, its own set of observed properties, and
 * its own state for asynchronous operations. Otherwise, everything is shared.
 *
 * This handle should be destroyed with domi_vid_destroy() if no longer
 * needed. The core will live as long as there is at least 1 handle referencing
 * it. Any handle can make the core quit, which will result in every handle
 * receiving domi_vid_EVENT_SHUTDOWN.
 *
 * This function can not be called before the main handle was initialized with
 * domi_vid_initialize(). The new handle is always initialized, unless ctx=NULL was
 * passed.
 *
 * @param ctx Used to get the reference to the mpv core; handle-specific
 *            settings and parameters are not used.
 *            If NULL, this function behaves like domi_vid_create() (ignores name).
 * @param name The client name. This will be returned by domi_vid_client_name(). If
 *             the name is already in use, or contains non-alphanumeric
 *             characters (other than '_'), the name is modified to fit.
 *             If NULL, an arbitrary name is automatically chosen.
 * @return a new handle, or NULL on error
 */
domi_vid_EXPORT domi_vid_handle *domi_vid_create_client(domi_vid_handle *ctx, const char *name);

/**
 * This is the same as domi_vid_create_client(), but the created domi_vid_handle is
 * treated as a weak reference. If all domi_vid_handles referencing a core are
 * weak references, the core is automatically destroyed. (This still goes
 * through normal uninit of course. Effectively, if the last non-weak domi_vid_handle
 * is destroyed, then the weak domi_vid_handles receive domi_vid_EVENT_SHUTDOWN and are
 * asked to terminate as well.)
 *
 * Note if you want to use this like refcounting: you have to be aware that
 * domi_vid_terminate_destroy() _and_ domi_vid_destroy() for the last non-weak
 * domi_vid_handle will block until all weak domi_vid_handles are destroyed.
 */
domi_vid_EXPORT domi_vid_handle *domi_vid_create_weak_client(domi_vid_handle *ctx, const char *name);

/**
 * Load a config file. This loads and parses the file, and sets every entry in
 * the config file's default section as if domi_vid_set_option_string() is called.
 *
 * The filename should be an absolute path. If it isn't, the actual path used
 * is unspecified. (Note: an absolute path starts with '/' on UNIX.) If the
 * file wasn't found, domi_vid_ERROR_INVALID_PARAMETER is returned.
 *
 * If a fatal error happens when parsing a config file, domi_vid_ERROR_OPTION_ERROR
 * is returned. Errors when setting options as well as other types or errors
 * are ignored (even if options do not exist). You can still try to capture
 * the resulting error messages with domi_vid_request_log_messages(). Note that it's
 * possible that some options were successfully set even if any of these errors
 * happen.
 *
 * @param filename absolute path to the config file on the local filesystem
 * @return error code
 */
domi_vid_EXPORT int domi_vid_load_config_file(domi_vid_handle *ctx, const char *filename);

/**
 * Return the internal time in nanoseconds. This has an arbitrary start offset,
 * but will never wrap or go backwards.
 *
 * Note that this is always the real time, and doesn't necessarily have to do
 * with playback time. For example, playback could go faster or slower due to
 * playback speed, or due to playback being paused. Use the "time-pos" property
 * instead to get the playback status.
 *
 * Unlike other libmpv APIs, this can be called at absolutely any time (even
 * within wakeup callbacks), as long as the context is valid.
 *
 * Safe to be called from mpv render API threads.
 */
domi_vid_EXPORT int64_t domi_vid_get_time_ns(domi_vid_handle *ctx);

/**
 * Same as domi_vid_get_time_ns but in microseconds.
 */
domi_vid_EXPORT int64_t domi_vid_get_time_us(domi_vid_handle *ctx);

/**
 * Data format for options and properties. The API functions to get/set
 * properties and options support multiple formats, and this enum describes
 * them.
 */
typedef enum domi_vid_format {
    /**
     * Invalid. Sometimes used for empty values. This is always defined to 0,
     * so a normal 0-init of domi_vid_format (or e.g. domi_vid_node) is guaranteed to set
     * this it to domi_vid_FORMAT_NONE (which makes some things saner as consequence).
     */
    domi_vid_FORMAT_NONE             = 0,
    /**
     * The basic type is char*. It returns the raw property string, like
     * using ${=property} in input.conf (see input.rst).
     *
     * NULL isn't an allowed value.
     *
     * Warning: although the encoding is usually UTF-8, this is not always the
     *          case. File tags often store strings in some legacy codepage,
     *          and even filenames don't necessarily have to be in UTF-8 (at
     *          least on Linux). If you pass the strings to code that requires
     *          valid UTF-8, you have to sanitize it in some way.
     *          On Windows, filenames are always UTF-8, and libmpv converts
     *          between UTF-8 and UTF-16 when using win32 API functions. See
     *          the "Encoding of filenames" section for details.
     *
     * Example for reading:
     *
     *     char *result = NULL;
     *     if (domi_vid_get_property(ctx, "property", domi_vid_FORMAT_STRING, &result) < 0)
     *         goto error;
     *     printf("%s\n", result);
     *     domi_vid_free(result);
     *
     * Or just use domi_vid_get_property_string().
     *
     * Example for writing:
     *
     *     char *value = "the new value";
     *     // yep, you pass the address to the variable
     *     // (needed for symmetry with other types and domi_vid_get_property)
     *     domi_vid_set_property(ctx, "property", domi_vid_FORMAT_STRING, &value);
     *
     * Or just use domi_vid_set_property_string().
     *
     */
    domi_vid_FORMAT_STRING           = 1,
    /**
     * The basic type is char*. It returns the OSD property string, like
     * using ${property} in input.conf (see input.rst). In many cases, this
     * is the same as the raw string, but in other cases it's formatted for
     * display on OSD. It's intended to be human readable. Do not attempt to
     * parse these strings.
     *
     * Only valid when doing read access. The rest works like domi_vid_FORMAT_STRING.
     */
    domi_vid_FORMAT_OSD_STRING       = 2,
    /**
     * The basic type is int. The only allowed values are 0 ("no")
     * and 1 ("yes").
     *
     * Example for reading:
     *
     *     int result;
     *     if (domi_vid_get_property(ctx, "property", domi_vid_FORMAT_FLAG, &result) < 0)
     *         goto error;
     *     printf("%s\n", result ? "true" : "false");
     *
     * Example for writing:
     *
     *     int flag = 1;
     *     domi_vid_set_property(ctx, "property", domi_vid_FORMAT_FLAG, &flag);
     */
    domi_vid_FORMAT_FLAG             = 3,
    /**
     * The basic type is int64_t.
     */
    domi_vid_FORMAT_INT64            = 4,
    /**
     * The basic type is double.
     */
    domi_vid_FORMAT_DOUBLE           = 5,
    /**
     * The type is domi_vid_node.
     *
     * For reading, you usually would pass a pointer to a stack-allocated
     * domi_vid_node value to mpv, and when you're done you call
     * domi_vid_free_node_contents(&node).
     * You're expected not to write to the data - if you have to, copy it
     * first (which you have to do manually).
     *
     * For writing, you construct your own domi_vid_node, and pass a pointer to the
     * API. The API will never write to your data (and copy it if needed), so
     * you're free to use any form of allocation or memory management you like.
     *
     * Warning: when reading, always check the domi_vid_node.format member. For
     *          example, properties might change their type in future versions
     *          of mpv, or sometimes even during runtime.
     *
     * Example for reading:
     *
     *     domi_vid_node result;
     *     if (domi_vid_get_property(ctx, "property", domi_vid_FORMAT_NODE, &result) < 0)
     *         goto error;
     *     printf("format=%d\n", (int)result.format);
     *     domi_vid_free_node_contents(&result).
     *
     * Example for writing:
     *
     *     domi_vid_node value;
     *     value.format = domi_vid_FORMAT_STRING;
     *     value.u.string = "hello";
     *     domi_vid_set_property(ctx, "property", domi_vid_FORMAT_NODE, &value);
     */
    domi_vid_FORMAT_NODE             = 6,
    /**
     * Used with domi_vid_node only. Can usually not be used directly.
     */
    domi_vid_FORMAT_NODE_ARRAY       = 7,
    /**
     * See domi_vid_FORMAT_NODE_ARRAY.
     */
    domi_vid_FORMAT_NODE_MAP         = 8,
    /**
     * A raw, untyped byte array. Only used only with domi_vid_node, and only in
     * some very specific situations. (Some commands use it.)
     */
    domi_vid_FORMAT_BYTE_ARRAY       = 9
} domi_vid_format;

/**
 * Generic data storage.
 *
 * If mpv writes this struct (e.g. via domi_vid_get_property()), you must not change
 * the data. In some cases (domi_vid_get_property()), you have to free it with
 * domi_vid_free_node_contents(). If you fill this struct yourself, you're also
 * responsible for freeing it, and you must not call domi_vid_free_node_contents().
 */
typedef struct domi_vid_node {
    union {
        char *string;   /** valid if format==domi_vid_FORMAT_STRING */
        int flag;       /** valid if format==domi_vid_FORMAT_FLAG   */
        int64_t int64;  /** valid if format==domi_vid_FORMAT_INT64  */
        double double_; /** valid if format==domi_vid_FORMAT_DOUBLE */
        /**
         * valid if format==domi_vid_FORMAT_NODE_ARRAY
         *    or if format==domi_vid_FORMAT_NODE_MAP
         */
        struct domi_vid_node_list *list;
        /**
         * valid if format==domi_vid_FORMAT_BYTE_ARRAY
         */
        struct domi_vid_byte_array *ba;
    } u;
    /**
     * Type of the data stored in this struct. This value rules what members in
     * the given union can be accessed. The following formats are currently
     * defined to be allowed in domi_vid_node:
     *
     *  domi_vid_FORMAT_STRING       (u.string)
     *  domi_vid_FORMAT_FLAG         (u.flag)
     *  domi_vid_FORMAT_INT64        (u.int64)
     *  domi_vid_FORMAT_DOUBLE       (u.double_)
     *  domi_vid_FORMAT_NODE_ARRAY   (u.list)
     *  domi_vid_FORMAT_NODE_MAP     (u.list)
     *  domi_vid_FORMAT_BYTE_ARRAY   (u.ba)
     *  domi_vid_FORMAT_NONE         (no member)
     *
     * If you encounter a value you don't know, you must not make any
     * assumptions about the contents of union u.
     */
    domi_vid_format format;
} domi_vid_node;

/**
 * (see domi_vid_node)
 */
typedef struct domi_vid_node_list {
    /**
     * Number of entries. Negative values are not allowed.
     */
    int num;
    /**
     * domi_vid_FORMAT_NODE_ARRAY:
     *  values[N] refers to value of the Nth item
     *
     * domi_vid_FORMAT_NODE_MAP:
     *  values[N] refers to value of the Nth key/value pair
     *
     * If num > 0, values[0] to values[num-1] (inclusive) are valid.
     * Otherwise, this can be NULL.
     */
    domi_vid_node *values;
    /**
     * domi_vid_FORMAT_NODE_ARRAY:
     *  unused (typically NULL), access is not allowed
     *
     * domi_vid_FORMAT_NODE_MAP:
     *  keys[N] refers to key of the Nth key/value pair. If num > 0, keys[0] to
     *  keys[num-1] (inclusive) are valid. Otherwise, this can be NULL.
     *  The keys are in random order. The only guarantee is that keys[N] belongs
     *  to the value values[N]. NULL keys are not allowed.
     */
    char **keys;
} domi_vid_node_list;

/**
 * (see domi_vid_node)
 */
typedef struct domi_vid_byte_array {
    /**
     * Pointer to the data. In what format the data is stored is up to whatever
     * uses domi_vid_FORMAT_BYTE_ARRAY.
     */
    void *data;
    /**
     * Size of the data pointed to by ptr.
     */
    size_t size;
} domi_vid_byte_array;

/**
 * Frees any data referenced by the node. It doesn't free the node itself.
 * Call this only if the mpv client API set the node. If you constructed the
 * node yourself (manually), you have to free it yourself.
 *
 * If node->format is domi_vid_FORMAT_NONE, this call does nothing. Likewise, if
 * the client API sets a node with this format, this function doesn't need to
 * be called. (This is just a clarification that there's no danger of anything
 * strange happening in these cases.)
 */
domi_vid_EXPORT void domi_vid_free_node_contents(domi_vid_node *node);

/**
 * Set an option. Note that you can't normally set options during runtime. It
 * works in uninitialized state (see domi_vid_create()), and in some cases in at
 * runtime.
 *
 * Using a format other than domi_vid_FORMAT_NODE is equivalent to constructing a
 * domi_vid_node with the given format and data, and passing the domi_vid_node to this
 * function.
 *
 * Note: this is semi-deprecated. For most purposes, this is not needed anymore.
 *       Starting with mpv version 0.21.0 (version 1.23) most options can be set
 *       with domi_vid_set_property() (and related functions), and even before
 *       domi_vid_initialize(). In some obscure corner cases, using this function
 *       to set options might still be required (see
 *       "Inconsistencies between options and properties" in the manpage). Once
 *       these are resolved, the option setting functions might be fully
 *       deprecated.
 *
 * @param name Option name. This is the same as on the mpv command line, but
 *             without the leading "--".
 * @param format see enum domi_vid_format.
 * @param[in] data Option value (according to the format).
 * @return error code
 */
domi_vid_EXPORT int domi_vid_set_option(domi_vid_handle *ctx, const char *name, domi_vid_format format,
                              void *data);

/**
 * Convenience function to set an option to a string value. This is like
 * calling domi_vid_set_option() with domi_vid_FORMAT_STRING.
 *
 * @return error code
 */
domi_vid_EXPORT int domi_vid_set_option_string(domi_vid_handle *ctx, const char *name, const char *data);

/**
 * Send a command to the player. Commands are the same as those used in
 * input.conf, except that this function takes parameters in a pre-split
 * form.
 *
 * The commands and their parameters are documented in input.rst.
 *
 * Does not use OSD and string expansion by default (unlike domi_vid_command_string()
 * and input.conf).
 *
 * @param[in] args NULL-terminated list of strings. Usually, the first item
 *                 is the command, and the following items are arguments.
 * @return error code
 */
domi_vid_EXPORT int domi_vid_command(domi_vid_handle *ctx, const char **args);

/**
 * Same as domi_vid_command(), but allows passing structured data in any format.
 * In particular, calling domi_vid_command() is exactly like calling
 * domi_vid_command_node() with the format set to domi_vid_FORMAT_NODE_ARRAY, and
 * every arg passed in order as domi_vid_FORMAT_STRING.
 *
 * Does not use OSD and string expansion by default.
 *
 * The args argument can have one of the following formats:
 *
 * domi_vid_FORMAT_NODE_ARRAY:
 *      Positional arguments. Each entry is an argument using an arbitrary
 *      format (the format must be compatible to the used command). Usually,
 *      the first item is the command name (as domi_vid_FORMAT_STRING). The order
 *      of arguments is as documented in each command description.
 *
 * domi_vid_FORMAT_NODE_MAP:
 *      Named arguments. This requires at least a special entry with the key
 *      "_name" to be present, which must be a string, and contains the command
 *      name. For compatibility, if the key "_name" does not exist, then the
 *      entry with the key "name" will be used instead.
 *      The special entry "_flags" is optional, and if present, must be an
 *      array of strings, each being a command prefix to apply. All other
 *      entries are interpreted as arguments. They must use the argument names
 *      as documented in each command description. Some commands do not
 *      support named arguments at all, and must use domi_vid_FORMAT_NODE_ARRAY.
 *      Some commands have arguments named "name", and can only be used if
 *      the command name is specified with key "_name" instead of "name".
 *
 * @param[in] args domi_vid_node with format set to one of the values documented
 *                 above (see there for details)
 * @param[out] result Optional, pass NULL if unused. If not NULL, and if the
 *                    function succeeds, this is set to command-specific return
 *                    data. You must call domi_vid_free_node_contents() to free it
 *                    (again, only if the command actually succeeds).
 *                    Not many commands actually use this at all.
 * @return error code (the result parameter is not set on error)
 */
domi_vid_EXPORT int domi_vid_command_node(domi_vid_handle *ctx, domi_vid_node *args, domi_vid_node *result);

/**
 * This is essentially identical to domi_vid_command() but it also returns a result.
 *
 * Does not use OSD and string expansion by default.
 *
 * @param[in] args NULL-terminated list of strings. Usually, the first item
 *                 is the command, and the following items are arguments.
 * @param[out] result Optional, pass NULL if unused. If not NULL, and if the
 *                    function succeeds, this is set to command-specific return
 *                    data. You must call domi_vid_free_node_contents() to free it
 *                    (again, only if the command actually succeeds).
 *                    Not many commands actually use this at all.
 * @return error code (the result parameter is not set on error)
 */
domi_vid_EXPORT int domi_vid_command_ret(domi_vid_handle *ctx, const char **args, domi_vid_node *result);

/**
 * Same as domi_vid_command, but use input.conf parsing for splitting arguments.
 * This is slightly simpler, but also more error prone, since arguments may
 * need quoting/escaping.
 *
 * This also has OSD and string expansion enabled by default.
 */
domi_vid_EXPORT int domi_vid_command_string(domi_vid_handle *ctx, const char *args);

/**
 * Same as domi_vid_command, but run the command asynchronously.
 *
 * Commands are executed asynchronously. You will receive a
 * domi_vid_EVENT_COMMAND_REPLY event. This event will also have an
 * error code set if running the command failed. For commands that
 * return data, the data is put into domi_vid_event_command.result.
 *
 * The only case when you do not receive an event is when the function call
 * itself fails. This happens only if parsing the command itself (or otherwise
 * validating it) fails, i.e. the return code of the API call is not 0 or
 * positive.
 *
 * Safe to be called from mpv render API threads.
 *
 * @param reply_userdata the value domi_vid_event.reply_userdata of the reply will
 *                       be set to (see section about asynchronous calls)
 * @param args NULL-terminated list of strings (see domi_vid_command())
 * @return error code (if parsing or queuing the command fails)
 */
domi_vid_EXPORT int domi_vid_command_async(domi_vid_handle *ctx, uint64_t reply_userdata,
                                 const char **args);

/**
 * Same as domi_vid_command_node(), but run it asynchronously. Basically, this
 * function is to domi_vid_command_node() what domi_vid_command_async() is to
 * domi_vid_command().
 *
 * See domi_vid_command_async() for details.
 *
 * Safe to be called from mpv render API threads.
 *
 * @param reply_userdata the value domi_vid_event.reply_userdata of the reply will
 *                       be set to (see section about asynchronous calls)
 * @param args as in domi_vid_command_node()
 * @return error code (if parsing or queuing the command fails)
 */
domi_vid_EXPORT int domi_vid_command_node_async(domi_vid_handle *ctx, uint64_t reply_userdata,
                                      domi_vid_node *args);

/**
 * Signal to all async requests with the matching ID to abort. This affects
 * the following API calls:
 *
 *      domi_vid_command_async
 *      domi_vid_command_node_async
 *
 * All of these functions take a reply_userdata parameter. This API function
 * tells all requests with the matching reply_userdata value to try to return
 * as soon as possible. If there are multiple requests with matching ID, it
 * aborts all of them.
 *
 * This API function is mostly asynchronous itself. It will not wait until the
 * command is aborted. Instead, the command will terminate as usual, but with
 * some work not done. How this is signaled depends on the specific command (for
 * example, the "subprocess" command will indicate it by "killed_by_us" set to
 * true in the result). How long it takes also depends on the situation. The
 * aborting process is completely asynchronous.
 *
 * Not all commands may support this functionality. In this case, this function
 * will have no effect. The same is true if the request using the passed
 * reply_userdata has already terminated, has not been started yet, or was
 * never in use at all.
 *
 * You have to be careful of race conditions: the time during which the abort
 * request will be effective is _after_ e.g. domi_vid_command_async() has returned,
 * and before the command has signaled completion with domi_vid_EVENT_COMMAND_REPLY.
 *
 * @param reply_userdata ID of the request to be aborted (see above)
 */
domi_vid_EXPORT void domi_vid_abort_async_command(domi_vid_handle *ctx, uint64_t reply_userdata);

/**
 * Set a property to a given value. Properties are essentially variables which
 * can be queried or set at runtime. For example, writing to the pause property
 * will actually pause or unpause playback.
 *
 * If the format doesn't match with the internal format of the property, access
 * usually will fail with domi_vid_ERROR_PROPERTY_FORMAT. In some cases, the data
 * is automatically converted and access succeeds. For example, domi_vid_FORMAT_INT64
 * is always converted to domi_vid_FORMAT_DOUBLE, and access using domi_vid_FORMAT_STRING
 * usually invokes a string parser. The same happens when calling this function
 * with domi_vid_FORMAT_NODE: the underlying format may be converted to another
 * type if possible.
 *
 * Using a format other than domi_vid_FORMAT_NODE is equivalent to constructing a
 * domi_vid_node with the given format and data, and passing the domi_vid_node to this
 * function. (Before API version 1.21, this was different.)
 *
 * Note: starting with mpv 0.21.0 (client API version 1.23), this can be used to
 *       set options in general. It even can be used before domi_vid_initialize()
 *       has been called. If called before domi_vid_initialize(), setting properties
 *       not backed by options will result in domi_vid_ERROR_PROPERTY_UNAVAILABLE.
 *       In some cases, properties and options still conflict. In these cases,
 *       domi_vid_set_property() accesses the options before domi_vid_initialize(), and
 *       the properties after domi_vid_initialize(). These conflicts will be removed
 *       in mpv 0.23.0. See domi_vid_set_option() for further remarks.
 *
 * @param name The property name. See input.rst for a list of properties.
 * @param format see enum domi_vid_format.
 * @param[in] data Option value.
 * @return error code
 */
domi_vid_EXPORT int domi_vid_set_property(domi_vid_handle *ctx, const char *name, domi_vid_format format,
                                void *data);

/**
 * Convenience function to set a property to a string value.
 *
 * This is like calling domi_vid_set_property() with domi_vid_FORMAT_STRING.
 */
domi_vid_EXPORT int domi_vid_set_property_string(domi_vid_handle *ctx, const char *name, const char *data);

/**
 * Convenience function to delete a property.
 *
 * This is equivalent to running the command "del [name]".
 *
 * @param name The property name. See input.rst for a list of properties.
 * @return error code
 */
domi_vid_EXPORT int domi_vid_del_property(domi_vid_handle *ctx, const char *name);

/**
 * Set a property asynchronously. You will receive the result of the operation
 * as domi_vid_EVENT_SET_PROPERTY_REPLY event. The domi_vid_event.error field will contain
 * the result status of the operation. Otherwise, this function is similar to
 * domi_vid_set_property().
 *
 * Safe to be called from mpv render API threads.
 *
 * @param reply_userdata see section about asynchronous calls
 * @param name The property name.
 * @param format see enum domi_vid_format.
 * @param[in] data Option value. The value will be copied by the function. It
 *                 will never be modified by the client API.
 * @return error code if sending the request failed
 */
domi_vid_EXPORT int domi_vid_set_property_async(domi_vid_handle *ctx, uint64_t reply_userdata,
                                      const char *name, domi_vid_format format, void *data);

/**
 * Read the value of the given property.
 *
 * If the format doesn't match with the internal format of the property, access
 * usually will fail with domi_vid_ERROR_PROPERTY_FORMAT. In some cases, the data
 * is automatically converted and access succeeds. For example, domi_vid_FORMAT_INT64
 * is always converted to domi_vid_FORMAT_DOUBLE, and access using domi_vid_FORMAT_STRING
 * usually invokes a string formatter.
 *
 * @param name The property name.
 * @param format see enum domi_vid_format.
 * @param[out] data Pointer to the variable holding the option value. On
 *                  success, the variable will be set to a copy of the option
 *                  value. For formats that require dynamic memory allocation,
 *                  you can free the value with domi_vid_free() (strings) or
 *                  domi_vid_free_node_contents() (domi_vid_FORMAT_NODE).
 * @return error code
 */
domi_vid_EXPORT int domi_vid_get_property(domi_vid_handle *ctx, const char *name, domi_vid_format format,
                                void *data);

/**
 * Return the value of the property with the given name as string. This is
 * equivalent to domi_vid_get_property() with domi_vid_FORMAT_STRING.
 *
 * See domi_vid_FORMAT_STRING for character encoding issues.
 *
 * On error, NULL is returned. Use domi_vid_get_property() if you want fine-grained
 * error reporting.
 *
 * @param name The property name.
 * @return Property value, or NULL if the property can't be retrieved. Free
 *         the string with domi_vid_free().
 */
domi_vid_EXPORT char *domi_vid_get_property_string(domi_vid_handle *ctx, const char *name);

/**
 * Return the property as "OSD" formatted string. This is the same as
 * domi_vid_get_property_string, but using domi_vid_FORMAT_OSD_STRING.
 *
 * @return Property value, or NULL if the property can't be retrieved. Free
 *         the string with domi_vid_free().
 */
domi_vid_EXPORT char *domi_vid_get_property_osd_string(domi_vid_handle *ctx, const char *name);

/**
 * Get a property asynchronously. You will receive the result of the operation
 * as well as the property data with the domi_vid_EVENT_GET_PROPERTY_REPLY event.
 * You should check the domi_vid_event.error field on the reply event.
 *
 * Safe to be called from mpv render API threads.
 *
 * @param reply_userdata see section about asynchronous calls
 * @param name The property name.
 * @param format see enum domi_vid_format.
 * @return error code if sending the request failed
 */
domi_vid_EXPORT int domi_vid_get_property_async(domi_vid_handle *ctx, uint64_t reply_userdata,
                                      const char *name, domi_vid_format format);

/**
 * Get a notification whenever the given property changes. You will receive
 * updates as domi_vid_EVENT_PROPERTY_CHANGE. Note that this is not very precise:
 * for some properties, it may not send updates even if the property changed.
 * This depends on the property, and it's a valid feature request to ask for
 * better update handling of a specific property. (For some properties, like
 * ``clock``, which shows the wall clock, this mechanism doesn't make too
 * much sense anyway.)
 *
 * Property changes are coalesced: the change events are returned only once the
 * event queue becomes empty (e.g. domi_vid_wait_event() would block or return
 * domi_vid_EVENT_NONE), and then only one event per changed property is returned.
 *
 * You always get an initial change notification. This is meant to initialize
 * the user's state to the current value of the property.
 *
 * Normally, change events are sent only if the property value changes according
 * to the requested format. domi_vid_event_property will contain the property value
 * as data member.
 *
 * Warning: if a property is unavailable or retrieving it caused an error,
 *          domi_vid_FORMAT_NONE will be set in domi_vid_event_property, even if the
 *          format parameter was set to a different value. In this case, the
 *          domi_vid_event_property.data field is invalid.
 *
 * If the property is observed with the format parameter set to domi_vid_FORMAT_NONE,
 * you get low-level notifications whether the property _may_ have changed, and
 * the data member in domi_vid_event_property will be unset. With this mode, you
 * will have to determine yourself whether the property really changed. On the
 * other hand, this mechanism can be faster and uses less resources.
 *
 * Observing a property that doesn't exist is allowed. (Although it may still
 * cause some sporadic change events.)
 *
 * Keep in mind that you will get change notifications even if you change a
 * property yourself. Try to avoid endless feedback loops, which could happen
 * if you react to the change notifications triggered by your own change.
 *
 * Only the domi_vid_handle on which this was called will receive the property
 * change events, or can unobserve them.
 *
 * Safe to be called from mpv render API threads.
 *
 * @param reply_userdata This will be used for the domi_vid_event.reply_userdata
 *                       field for the received domi_vid_EVENT_PROPERTY_CHANGE
 *                       events. (Also see section about asynchronous calls,
 *                       although this function is somewhat different from
 *                       actual asynchronous calls.)
 *                       If you have no use for this, pass 0.
 *                       Also see domi_vid_unobserve_property().
 * @param name The property name.
 * @param format see enum domi_vid_format. Can be domi_vid_FORMAT_NONE to omit values
 *               from the change events.
 * @return error code (usually fails only on OOM or unsupported format)
 */
domi_vid_EXPORT int domi_vid_observe_property(domi_vid_handle *mpv, uint64_t reply_userdata,
                                    const char *name, domi_vid_format format);

/**
 * Undo domi_vid_observe_property(). This will remove all observed properties for
 * which the given number was passed as reply_userdata to domi_vid_observe_property.
 *
 * Safe to be called from mpv render API threads.
 *
 * @param registered_reply_userdata ID that was passed to domi_vid_observe_property
 * @return negative value is an error code, >=0 is number of removed properties
 *         on success (includes the case when 0 were removed)
 */
domi_vid_EXPORT int domi_vid_unobserve_property(domi_vid_handle *mpv, uint64_t registered_reply_userdata);

typedef enum domi_vid_event_id {
    /**
     * Nothing happened. Happens on timeouts or sporadic wakeups.
     */
    domi_vid_EVENT_NONE              = 0,
    /**
     * Happens when the player quits. The player enters a state where it tries
     * to disconnect all clients. Most requests to the player will fail, and
     * the client should react to this and quit with domi_vid_destroy() as soon as
     * possible.
     */
    domi_vid_EVENT_SHUTDOWN          = 1,
    /**
     * See domi_vid_request_log_messages().
     */
    domi_vid_EVENT_LOG_MESSAGE       = 2,
    /**
     * Reply to a domi_vid_get_property_async() request.
     * See also domi_vid_event and domi_vid_event_property.
     */
    domi_vid_EVENT_GET_PROPERTY_REPLY = 3,
    /**
     * Reply to a domi_vid_set_property_async() request.
     * (Unlike domi_vid_EVENT_GET_PROPERTY, domi_vid_event_property is not used.)
     */
    domi_vid_EVENT_SET_PROPERTY_REPLY = 4,
    /**
     * Reply to a domi_vid_command_async() or domi_vid_command_node_async() request.
     * See also domi_vid_event and domi_vid_event_command.
     */
    domi_vid_EVENT_COMMAND_REPLY     = 5,
    /**
     * Notification before playback start of a file (before the file is loaded).
     * See also domi_vid_event and domi_vid_event_start_file.
     */
    domi_vid_EVENT_START_FILE        = 6,
    /**
     * Notification after playback end (after the file was unloaded).
     * See also domi_vid_event and domi_vid_event_end_file.
     */
    domi_vid_EVENT_END_FILE          = 7,
    /**
     * Notification when the file has been loaded (headers were read etc.), and
     * decoding starts.
     */
    domi_vid_EVENT_FILE_LOADED       = 8,
#if domi_vid_ENABLE_DEPRECATED
    /**
     * Idle mode was entered. In this mode, no file is played, and the playback
     * core waits for new commands. (The command line player normally quits
     * instead of entering idle mode, unless --idle was specified. If mpv
     * was started with domi_vid_create(), idle mode is enabled by default.)
     *
     * @deprecated This is equivalent to using domi_vid_observe_property() on the
     *             "idle-active" property. The event is redundant, and might be
     *             removed in the far future. As a further warning, this event
     *             is not necessarily sent at the right point anymore (at the
     *             start of the program), while the property behaves correctly.
     */
    domi_vid_EVENT_IDLE              = 11,
    /**
     * Sent every time after a video frame is displayed. Note that currently,
     * this will be sent in lower frequency if there is no video, or playback
     * is paused - but that will be removed in the future, and it will be
     * restricted to video frames only.
     *
     * @deprecated Use domi_vid_observe_property() with relevant properties instead
     *             (such as "playback-time").
     */
    domi_vid_EVENT_TICK              = 14,
#endif
    /**
     * Triggered by the script-message input command. The command uses the
     * first argument of the command as client name (see domi_vid_client_name()) to
     * dispatch the message, and passes along all arguments starting from the
     * second argument as strings.
     * See also domi_vid_event and domi_vid_event_client_message.
     */
    domi_vid_EVENT_CLIENT_MESSAGE    = 16,
    /**
     * Happens after video changed in some way. This can happen on resolution
     * changes, pixel format changes, or video filter changes. The event is
     * sent after the video filters and the VO are reconfigured. Applications
     * embedding a mpv window should listen to this event in order to resize
     * the window if needed.
     * Note that this event can happen sporadically, and you should check
     * yourself whether the video parameters really changed before doing
     * something expensive.
     */
    domi_vid_EVENT_VIDEO_RECONFIG    = 17,
    /**
     * Similar to domi_vid_EVENT_VIDEO_RECONFIG. This is relatively uninteresting,
     * because there is no such thing as audio output embedding.
     */
    domi_vid_EVENT_AUDIO_RECONFIG    = 18,
    /**
     * Happens when a seek was initiated. Playback stops. Usually it will
     * resume with domi_vid_EVENT_PLAYBACK_RESTART as soon as the seek is finished.
     */
    domi_vid_EVENT_SEEK              = 20,
    /**
     * There was a discontinuity of some sort (like a seek), and playback
     * was reinitialized. Usually happens on start of playback and after
     * seeking. The main purpose is allowing the client to detect when a seek
     * request is finished.
     */
    domi_vid_EVENT_PLAYBACK_RESTART  = 21,
    /**
     * Event sent due to domi_vid_observe_property().
     * See also domi_vid_event and domi_vid_event_property.
     */
    domi_vid_EVENT_PROPERTY_CHANGE   = 22,
    /**
     * Happens if the internal per-domi_vid_handle ringbuffer overflows, and at
     * least 1 event had to be dropped. This can happen if the client doesn't
     * read the event queue quickly enough with domi_vid_wait_event(), or if the
     * client makes a very large number of asynchronous calls at once.
     *
     * Event delivery will continue normally once this event was returned
     * (this forces the client to empty the queue completely).
     */
    domi_vid_EVENT_QUEUE_OVERFLOW    = 24,
    /**
     * Triggered if a hook handler was registered with domi_vid_hook_add(), and the
     * hook is invoked. If you receive this, you must handle it, and continue
     * the hook with domi_vid_hook_continue().
     * See also domi_vid_event and domi_vid_event_hook.
     */
    domi_vid_EVENT_HOOK              = 25,
    // Internal note: adjust INTERNAL_EVENT_BASE when adding new events.
} domi_vid_event_id;

/**
 * Return a string describing the event. For unknown events, NULL is returned.
 *
 * Note that all events actually returned by the API will also yield a non-NULL
 * string with this function.
 *
 * @param event event ID, see see enum domi_vid_event_id
 * @return A static string giving a short symbolic name of the event. It
 *         consists of lower-case alphanumeric characters and can include "-"
 *         characters. This string is suitable for use in e.g. scripting
 *         interfaces.
 *         The string is completely static, i.e. doesn't need to be deallocated,
 *         and is valid forever.
 */
domi_vid_EXPORT const char *domi_vid_event_name(domi_vid_event_id event);

typedef struct domi_vid_event_property {
    /**
     * Name of the property.
     */
    const char *name;
    /**
     * Format of the data field in the same struct. See enum domi_vid_format.
     * This is always the same format as the requested format, except when
     * the property could not be retrieved (unavailable, or an error happened),
     * in which case the format is domi_vid_FORMAT_NONE.
     */
    domi_vid_format format;
    /**
     * Received property value. Depends on the format. This is like the
     * pointer argument passed to domi_vid_get_property().
     *
     * For example, for domi_vid_FORMAT_STRING you get the string with:
     *
     *    char *value = *(char **)(event_property->data);
     *
     * Note that this is set to NULL if retrieving the property failed (the
     * format will be domi_vid_FORMAT_NONE).
     */
    void *data;
} domi_vid_event_property;

/**
 * Numeric log levels. The lower the number, the more important the message is.
 * domi_vid_LOG_LEVEL_NONE is never used when receiving messages. The string in
 * the comment after the value is the name of the log level as used for the
 * domi_vid_request_log_messages() function.
 * Unused numeric values are unused, but reserved for future use.
 */
typedef enum domi_vid_log_level {
    domi_vid_LOG_LEVEL_NONE  = 0,    /// "no"    - disable absolutely all messages
    domi_vid_LOG_LEVEL_FATAL = 10,   /// "fatal" - critical/aborting errors
    domi_vid_LOG_LEVEL_ERROR = 20,   /// "error" - simple errors
    domi_vid_LOG_LEVEL_WARN  = 30,   /// "warn"  - possible problems
    domi_vid_LOG_LEVEL_INFO  = 40,   /// "info"  - informational message
    domi_vid_LOG_LEVEL_V     = 50,   /// "v"     - noisy informational message
    domi_vid_LOG_LEVEL_DEBUG = 60,   /// "debug" - very noisy technical information
    domi_vid_LOG_LEVEL_TRACE = 70,   /// "trace" - extremely noisy
} domi_vid_log_level;

typedef struct domi_vid_event_log_message {
    /**
     * The module prefix, identifies the sender of the message. As a special
     * case, if the message buffer overflows, this will be set to the string
     * "overflow" (which doesn't appear as prefix otherwise), and the text
     * field will contain an informative message.
     */
    const char *prefix;
    /**
     * The log level as string. See domi_vid_request_log_messages() for possible
     * values. The level "no" is never used here.
     */
    const char *level;
    /**
     * The log message. It consists of 1 line of text, and is terminated with
     * a newline character. (Before API version 1.6, it could contain multiple
     * or partial lines.)
     */
    const char *text;
    /**
     * The same contents as the level field, but as a numeric ID.
     * Since API version 1.6.
     */
    domi_vid_log_level log_level;
} domi_vid_event_log_message;

/// Since API version 1.9.
typedef enum domi_vid_end_file_reason {
    /**
     * The end of file was reached. Sometimes this may also happen on
     * incomplete or corrupted files, or if the network connection was
     * interrupted when playing a remote file. It also happens if the
     * playback range was restricted with --end or --frames or similar.
     */
    domi_vid_END_FILE_REASON_EOF = 0,
    /**
     * Playback was stopped by an external action (e.g. playlist controls).
     */
    domi_vid_END_FILE_REASON_STOP = 2,
    /**
     * Playback was stopped by the quit command or player shutdown.
     */
    domi_vid_END_FILE_REASON_QUIT = 3,
    /**
     * Some kind of error happened that lead to playback abort. Does not
     * necessarily happen on incomplete or broken files (in these cases, both
     * domi_vid_END_FILE_REASON_ERROR or domi_vid_END_FILE_REASON_EOF are possible).
     *
     * domi_vid_event_end_file.error will be set.
     */
    domi_vid_END_FILE_REASON_ERROR = 4,
    /**
     * The file was a playlist or similar. When the playlist is read, its
     * entries will be appended to the playlist after the entry of the current
     * file, the entry of the current file is removed, and a domi_vid_EVENT_END_FILE
     * event is sent with reason set to domi_vid_END_FILE_REASON_REDIRECT. Then
     * playback continues with the playlist contents.
     * Since API version 1.18.
     */
    domi_vid_END_FILE_REASON_REDIRECT = 5,
} domi_vid_end_file_reason;

/// Since API version 1.108.
typedef struct domi_vid_event_start_file {
    /**
     * Playlist entry ID of the file being loaded now.
     */
    int64_t playlist_entry_id;
} domi_vid_event_start_file;

typedef struct domi_vid_event_end_file {
    /**
     * Corresponds to the values in enum domi_vid_end_file_reason.
     *
     * Unknown values should be treated as unknown.
     */
    domi_vid_end_file_reason reason;
    /**
     * If reason==domi_vid_END_FILE_REASON_ERROR, this contains a mpv error code
     * (one of domi_vid_ERROR_...) giving an approximate reason why playback
     * failed. In other cases, this field is 0 (no error).
     * Since API version 1.9.
     */
    int error;
    /**
     * Playlist entry ID of the file that was being played or attempted to be
     * played. This has the same value as the playlist_entry_id field in the
     * corresponding domi_vid_event_start_file event.
     * Since API version 1.108.
     */
    int64_t playlist_entry_id;
    /**
     * If loading ended, because the playlist entry to be played was for example
     * a playlist, and the current playlist entry is replaced with a number of
     * other entries. This may happen at least with domi_vid_END_FILE_REASON_REDIRECT
     * (other event types may use this for similar but different purposes in the
     * future). In this case, playlist_insert_id will be set to the playlist
     * entry ID of the first inserted entry, and playlist_insert_num_entries to
     * the total number of inserted playlist entries. Note this in this specific
     * case, the ID of the last inserted entry is playlist_insert_id+num-1.
     * Beware that depending on circumstances, you may observe the new playlist
     * entries before seeing the event (e.g. reading the "playlist" property or
     * getting a property change notification before receiving the event).
     * Since API version 1.108.
     */
    int64_t playlist_insert_id;
    /**
     * See playlist_insert_id. Only non-0 if playlist_insert_id is valid. Never
     * negative.
     * Since API version 1.108.
     */
    int playlist_insert_num_entries;
} domi_vid_event_end_file;

typedef struct domi_vid_event_client_message {
    /**
     * Arbitrary arguments chosen by the sender of the message. If num_args > 0,
     * you can access args[0] through args[num_args - 1] (inclusive). What
     * these arguments mean is up to the sender and receiver.
     * None of the valid items are NULL.
     */
    int num_args;
    const char **args;
} domi_vid_event_client_message;

typedef struct domi_vid_event_hook {
    /**
     * The hook name as passed to domi_vid_hook_add().
     */
    const char *name;
    /**
     * Internal ID that must be passed to domi_vid_hook_continue().
     */
    uint64_t id;
} domi_vid_event_hook;

// Since API version 1.102.
typedef struct domi_vid_event_command {
    /**
     * Result data of the command. Note that success/failure is signaled
     * separately via domi_vid_event.error. This field is only for result data
     * in case of success. Most commands leave it at domi_vid_FORMAT_NONE. Set
     * to domi_vid_FORMAT_NONE on failure.
     */
    domi_vid_node result;
} domi_vid_event_command;

typedef struct domi_vid_event {
    /**
     * One of domi_vid_event. Keep in mind that later ABI compatible releases might
     * add new event types. These should be ignored by the API user.
     */
    domi_vid_event_id event_id;
    /**
     * This is mainly used for events that are replies to (asynchronous)
     * requests. It contains a status code, which is >= 0 on success, or < 0
     * on error (a domi_vid_error value). Usually, this will be set if an
     * asynchronous request fails.
     * Used for:
     *  domi_vid_EVENT_GET_PROPERTY_REPLY
     *  domi_vid_EVENT_SET_PROPERTY_REPLY
     *  domi_vid_EVENT_COMMAND_REPLY
     */
    int error;
    /**
     * If the event is in reply to a request (made with this API and this
     * API handle), this is set to the reply_userdata parameter of the request
     * call. Otherwise, this field is 0.
     * Used for:
     *  domi_vid_EVENT_GET_PROPERTY_REPLY
     *  domi_vid_EVENT_SET_PROPERTY_REPLY
     *  domi_vid_EVENT_COMMAND_REPLY
     *  domi_vid_EVENT_PROPERTY_CHANGE
     *  domi_vid_EVENT_HOOK
     */
    uint64_t reply_userdata;
    /**
     * The meaning and contents of the data member depend on the event_id:
     *  domi_vid_EVENT_GET_PROPERTY_REPLY:     domi_vid_event_property*
     *  domi_vid_EVENT_PROPERTY_CHANGE:        domi_vid_event_property*
     *  domi_vid_EVENT_LOG_MESSAGE:            domi_vid_event_log_message*
     *  domi_vid_EVENT_CLIENT_MESSAGE:         domi_vid_event_client_message*
     *  domi_vid_EVENT_START_FILE:             domi_vid_event_start_file* (since v1.108)
     *  domi_vid_EVENT_END_FILE:               domi_vid_event_end_file*
     *  domi_vid_EVENT_HOOK:                   domi_vid_event_hook*
     *  domi_vid_EVENT_COMMAND_REPLY*          domi_vid_event_command*
     *  other: NULL
     *
     * Note: future enhancements might add new event structs for existing or new
     *       event types.
     */
    void *data;
} domi_vid_event;

/**
 * Convert the given src event to a domi_vid_node, and set *dst to the result. *dst
 * is set to a domi_vid_FORMAT_NODE_MAP, with fields for corresponding domi_vid_event and
 * domi_vid_event.data/domi_vid_event_* fields.
 *
 * The exact details are not completely documented out of laziness. A start
 * is located in the "Events" section of the manpage.
 *
 * *dst may point to newly allocated memory, or pointers in domi_vid_event. You must
 * copy the entire domi_vid_node if you want to reference it after domi_vid_event becomes
 * invalid (such as making a new domi_vid_wait_event() call, or destroying the
 * domi_vid_handle from which it was returned). Call domi_vid_free_node_contents() to free
 * any memory allocations made by this API function.
 *
 * Safe to be called from mpv render API threads.
 *
 * @param dst Target. This is not read and fully overwritten. Must be released
 *            with domi_vid_free_node_contents(). Do not write to pointers returned
 *            by it. (On error, this may be left as an empty node.)
 * @param src The source event. Not modified (it's not const due to the author's
 *            prejudice of the C version of const).
 * @return error code (domi_vid_ERROR_NOMEM only, if at all)
 */
domi_vid_EXPORT int domi_vid_event_to_node(domi_vid_node *dst, domi_vid_event *src);

/**
 * Enable or disable the given event.
 *
 * Some events are enabled by default. Some events can't be disabled.
 *
 * (Informational note: currently, all events are enabled by default, except
 *  domi_vid_EVENT_TICK.)
 *
 * Safe to be called from mpv render API threads.
 *
 * @param event See enum domi_vid_event_id.
 * @param enable 1 to enable receiving this event, 0 to disable it.
 * @return error code
 */
domi_vid_EXPORT int domi_vid_request_event(domi_vid_handle *ctx, domi_vid_event_id event, int enable);

/**
 * Enable or disable receiving of log messages. These are the messages the
 * command line player prints to the terminal. This call sets the minimum
 * required log level for a message to be received with domi_vid_EVENT_LOG_MESSAGE.
 *
 * @param min_level Minimal log level as string. Valid log levels:
 *                      no fatal error warn info v debug trace
 *                  The value "no" disables all messages. This is the default.
 *                  An exception is the value "terminal-default", which uses the
 *                  log level as set by the "--msg-level" option. This works
 *                  even if the terminal is disabled. (Since API version 1.19.)
 *                  Also see domi_vid_log_level.
 * @return error code
 */
domi_vid_EXPORT int domi_vid_request_log_messages(domi_vid_handle *ctx, const char *min_level);

/**
 * Wait for the next event, or until the timeout expires, or if another thread
 * makes a call to domi_vid_wakeup(). Passing 0 as timeout will never wait, and
 * is suitable for polling.
 *
 * The internal event queue has a limited size (per client handle). If you
 * don't empty the event queue quickly enough with domi_vid_wait_event(), it will
 * overflow and silently discard further events. If this happens, making
 * asynchronous requests will fail as well (with domi_vid_ERROR_EVENT_QUEUE_FULL).
 *
 * Only one thread is allowed to call this on the same domi_vid_handle at a time.
 * The API won't complain if more than one thread calls this, but it will cause
 * race conditions in the client when accessing the shared domi_vid_event struct.
 * Note that most other API functions are not restricted by this, and no API
 * function internally calls domi_vid_wait_event(). Additionally, concurrent calls
 * to different domi_vid_handles are always safe.
 *
 * As long as the timeout is 0, this is safe to be called from mpv render API
 * threads.
 *
 * @param timeout Timeout in seconds, after which the function returns even if
 *                no event was received. A domi_vid_EVENT_NONE is returned on
 *                timeout. A value of 0 will disable waiting. Negative values
 *                will wait with an infinite timeout.
 * @return A struct containing the event ID and other data. The pointer (and
 *         fields in the struct) stay valid until the next domi_vid_wait_event()
 *         call, or until the domi_vid_handle is destroyed. You must not write to
 *         the struct, and all memory referenced by it will be automatically
 *         released by the API on the next domi_vid_wait_event() call, or when the
 *         context is destroyed. The return value is never NULL.
 */
domi_vid_EXPORT domi_vid_event *domi_vid_wait_event(domi_vid_handle *ctx, double timeout);

/**
 * Interrupt the current domi_vid_wait_event() call. This will wake up the thread
 * currently waiting in domi_vid_wait_event(). If no thread is waiting, the next
 * domi_vid_wait_event() call will return immediately (this is to avoid lost
 * wakeups).
 *
 * domi_vid_wait_event() will receive a domi_vid_EVENT_NONE if it's woken up due to
 * this call. But note that this dummy event might be skipped if there are
 * already other events queued. All what counts is that the waiting thread
 * is woken up at all.
 *
 * Safe to be called from mpv render API threads.
 */
domi_vid_EXPORT void domi_vid_wakeup(domi_vid_handle *ctx);

/**
 * Set a custom function that should be called when there are new events. Use
 * this if blocking in domi_vid_wait_event() to wait for new events is not feasible.
 *
 * Keep in mind that the callback will be called from foreign threads. You
 * must not make any assumptions of the environment, and you must return as
 * soon as possible (i.e. no long blocking waits). Exiting the callback through
 * any other means than a normal return is forbidden (no throwing exceptions,
 * no longjmp() calls). You must not change any local thread state (such as
 * the C floating point environment).
 *
 * You are not allowed to call any client API functions inside of the callback.
 * In particular, you should not do any processing in the callback, but wake up
 * another thread that does all the work. The callback is meant strictly for
 * notification only, and is called from arbitrary core parts of the player,
 * that make no considerations for reentrant API use or allowing the callee to
 * spend a lot of time doing other things. Keep in mind that it's also possible
 * that the callback is called from a thread while a mpv API function is called
 * (i.e. it can be reentrant).
 *
 * In general, the client API expects you to call domi_vid_wait_event() to receive
 * notifications, and the wakeup callback is merely a helper utility to make
 * this easier in certain situations. Note that it's possible that there's
 * only one wakeup callback invocation for multiple events. You should call
 * domi_vid_wait_event() with no timeout until domi_vid_EVENT_NONE is reached, at which
 * point the event queue is empty.
 *
 * If you actually want to do processing in a callback, spawn a thread that
 * does nothing but call domi_vid_wait_event() in a loop and dispatches the result
 * to a callback.
 *
 * Only one wakeup callback can be set.
 *
 * @param cb function that should be called if a wakeup is required
 * @param d arbitrary userdata passed to cb
 */
domi_vid_EXPORT void domi_vid_set_wakeup_callback(domi_vid_handle *ctx, void (*cb)(void *d), void *d);

/**
 * Block until all asynchronous requests are done. This affects functions like
 * domi_vid_command_async(), which return immediately and return their result as
 * events.
 *
 * This is a helper, and somewhat equivalent to calling domi_vid_wait_event() in a
 * loop until all known asynchronous requests have sent their reply as event,
 * except that the event queue is not emptied.
 */
domi_vid_EXPORT void domi_vid_wait_async_requests(domi_vid_handle *ctx);

/**
 * A hook is like a synchronous event that blocks the player. You register
 * a hook handler with this function. You will get an event, which you need
 * to handle, and once things are ready, you can let the player continue with
 * domi_vid_hook_continue().
 *
 * Currently, hooks can't be removed explicitly. But they will be implicitly
 * removed if the domi_vid_handle it was registered with is destroyed. This also
 * continues the hook if it was being handled by the destroyed domi_vid_handle (but
 * this should be avoided, as it might mess up order of hook execution).
 *
 * Hook handlers are ordered globally by priority and order of registration.
 * Handlers for the same hook with same priority are invoked in order of
 * registration (the handler registered first is run first). Handlers with
 * lower priority are run first (which seems backward).
 *
 * See the "Hooks" section in the manpage to see which hooks are currently
 * defined.
 *
 * Some hooks might be reentrant (so you get multiple domi_vid_EVENT_HOOK for the
 * same hook). If this can happen for a specific hook type, it will be
 * explicitly documented in the manpage.
 *
 * Only the domi_vid_handle on which this was called will receive the hook events,
 * or can "continue" them.
 *
 * @param reply_userdata This will be used for the domi_vid_event.reply_userdata
 *                       field for the received domi_vid_EVENT_HOOK events.
 *                       If you have no use for this, pass 0.
 * @param name The hook name. This should be one of the documented names. But
 *             if the name is unknown, the hook event will simply be never
 *             raised.
 * @param priority See remarks above. Use 0 as a neutral default.
 * @return error code (usually fails only on OOM)
 */
domi_vid_EXPORT int domi_vid_hook_add(domi_vid_handle *ctx, uint64_t reply_userdata,
                            const char *name, int priority);

/**
 * Respond to a domi_vid_EVENT_HOOK event. You must call this after you have handled
 * the event. There is no way to "cancel" or "stop" the hook.
 *
 * Calling this will will typically unblock the player for whatever the hook
 * is responsible for (e.g. for the "on_load" hook it lets it continue
 * playback).
 *
 * It is explicitly undefined behavior to call this more than once for each
 * domi_vid_EVENT_HOOK, to pass an incorrect ID, or to call this on a domi_vid_handle
 * different from the one that registered the handler and received the event.
 *
 * @param id This must be the value of the domi_vid_event_hook.id field for the
 *           corresponding domi_vid_EVENT_HOOK.
 * @return error code
 */
domi_vid_EXPORT int domi_vid_hook_continue(domi_vid_handle *ctx, uint64_t id);

#if domi_vid_ENABLE_DEPRECATED

/**
 * Return a UNIX file descriptor referring to the read end of a pipe. This
 * pipe can be used to wake up a poll() based processing loop. The purpose of
 * this function is very similar to domi_vid_set_wakeup_callback(), and provides
 * a primitive mechanism to handle coordinating a foreign event loop and the
 * libmpv event loop. The pipe is non-blocking. It's closed when the domi_vid_handle
 * is destroyed. This function always returns the same value (on success).
 *
 * This is in fact implemented using the same underlying code as for
 * domi_vid_set_wakeup_callback() (though they don't conflict), and it is as if each
 * callback invocation writes a single 0 byte to the pipe. When the pipe
 * becomes readable, the code calling poll() (or select()) on the pipe should
 * read all contents of the pipe and then call domi_vid_wait_event(c, 0) until
 * no new events are returned. The pipe contents do not matter and can just
 * be discarded. There is not necessarily one byte per readable event in the
 * pipe. For example, the pipes are non-blocking, and mpv won't block if the
 * pipe is full. Pipes are normally limited to 4096 bytes, so if there are
 * more than 4096 events, the number of readable bytes can not equal the number
 * of events queued. Also, it's possible that mpv does not write to the pipe
 * once it's guaranteed that the client was already signaled. See the example
 * below how to do it correctly.
 *
 * Example:
 *
 *  int pipefd = domi_vid_get_wakeup_pipe(mpv);
 *  if (pipefd < 0)
 *      error();
 *  while (1) {
 *      struct pollfd pfds[1] = {
 *          { .fd = pipefd, .events = POLLIN },
 *      };
 *      // Wait until there are possibly new mpv events.
 *      poll(pfds, 1, -1);
 *      if (pfds[0].revents & POLLIN) {
 *          // Empty the pipe. Doing this before calling domi_vid_wait_event()
 *          // ensures that no wakeups are missed. It's not so important to
 *          // make sure the pipe is really empty (it will just cause some
 *          // additional wakeups in unlikely corner cases).
 *          char unused[256];
 *          read(pipefd, unused, sizeof(unused));
 *          while (1) {
 *              domi_vid_event *ev = domi_vid_wait_event(mpv, 0);
 *              // If domi_vid_EVENT_NONE is received, the event queue is empty.
 *              if (ev->event_id == domi_vid_EVENT_NONE)
 *                  break;
 *              // Process the event.
 *              ...
 *          }
 *      }
 *  }
 *
 * @deprecated this function will be removed in the future. If you need this
 *             functionality, use domi_vid_set_wakeup_callback(), create a pipe
 *             manually, and call write() on your pipe in the callback.
 *
 * @return A UNIX FD of the read end of the wakeup pipe, or -1 on error.
 *         On MS Windows/MinGW, this will always return -1.
 */
domi_vid_EXPORT int domi_vid_get_wakeup_pipe(domi_vid_handle *ctx);

#endif

/**
 * Defining domi_vid_CPLUGIN_DYNAMIC_SYM during plugin compilation will replace domi_vid_*
 * functions with function pointers. Those pointer will be initialized when
 * loading the plugin.
 *
 * It is recommended to use this symbol table when targeting Windows. The loader
 * does not have notion of global symbols. Loading cplugin into mpv process will
 * not allow this plugin to call any of the symbols that may be available in
 * other modules. Instead cplugin has to link explicitly to specific PE binary,
 * libmpv-2.dll/mpv.exe or any other binary that may have linked mpv statically.
 * This limits portability of cplugin as it would need to be compiled separately
 * for each of target PE binary that includes mpv's symbols. Which in practice
 * is unrealistic, as we want one cplugin to be loaded without those restrictions.
 *
 * Instead of linking to any PE binary, we create function pointers for all mpv's
 * exported symbols. For convenience names of entrypoints are redefined to those
 * pointer, so no changes are required in cplugin source code, except of defining
 * domi_vid_CPLUGIN_DYNAMIC_SYM. Those function pointer are exported to make them
 * available for mpv to init with correct values during runtime, before calling
 * `domi_vid_open_cplugin`.
 *
 * Note that those pointers are decorated with `selectany` attribute, so no need
 * to worry about multiple definitions, linker will keep only single instance.
 */
#ifdef domi_vid_CPLUGIN_DYNAMIC_SYM

#define domi_vid_DEFINE_SYM_PTR(name)  \
    domi_vid_SELECTANY domi_vid_EXPORT      \
    domi_vid_DECLTYPE(name) *pfn_##name;

domi_vid_DEFINE_SYM_PTR(domi_vid_client_api_version)
#define domi_vid_client_api_version pfn_domi_vid_client_api_version
domi_vid_DEFINE_SYM_PTR(domi_vid_error_string)
#define domi_vid_error_string pfn_domi_vid_error_string
domi_vid_DEFINE_SYM_PTR(domi_vid_free)
#define domi_vid_free pfn_domi_vid_free
domi_vid_DEFINE_SYM_PTR(domi_vid_client_name)
#define domi_vid_client_name pfn_domi_vid_client_name
domi_vid_DEFINE_SYM_PTR(domi_vid_client_id)
#define domi_vid_client_id pfn_domi_vid_client_id
domi_vid_DEFINE_SYM_PTR(domi_vid_create)
#define domi_vid_create pfn_domi_vid_create
domi_vid_DEFINE_SYM_PTR(domi_vid_initialize)
#define domi_vid_initialize pfn_domi_vid_initialize
domi_vid_DEFINE_SYM_PTR(domi_vid_destroy)
#define domi_vid_destroy pfn_domi_vid_destroy
domi_vid_DEFINE_SYM_PTR(domi_vid_terminate_destroy)
#define domi_vid_terminate_destroy pfn_domi_vid_terminate_destroy
domi_vid_DEFINE_SYM_PTR(domi_vid_create_client)
#define domi_vid_create_client pfn_domi_vid_create_client
domi_vid_DEFINE_SYM_PTR(domi_vid_create_weak_client)
#define domi_vid_create_weak_client pfn_domi_vid_create_weak_client
domi_vid_DEFINE_SYM_PTR(domi_vid_load_config_file)
#define domi_vid_load_config_file pfn_domi_vid_load_config_file
domi_vid_DEFINE_SYM_PTR(domi_vid_get_time_ns)
#define domi_vid_get_time_ns pfn_domi_vid_get_time_ns
domi_vid_DEFINE_SYM_PTR(domi_vid_get_time_us)
#define domi_vid_get_time_us pfn_domi_vid_get_time_us
domi_vid_DEFINE_SYM_PTR(domi_vid_free_node_contents)
#define domi_vid_free_node_contents pfn_domi_vid_free_node_contents
domi_vid_DEFINE_SYM_PTR(domi_vid_set_option)
#define domi_vid_set_option pfn_domi_vid_set_option
domi_vid_DEFINE_SYM_PTR(domi_vid_set_option_string)
#define domi_vid_set_option_string pfn_domi_vid_set_option_string
domi_vid_DEFINE_SYM_PTR(domi_vid_command)
#define domi_vid_command pfn_domi_vid_command
domi_vid_DEFINE_SYM_PTR(domi_vid_command_node)
#define domi_vid_command_node pfn_domi_vid_command_node
domi_vid_DEFINE_SYM_PTR(domi_vid_command_ret)
#define domi_vid_command_ret pfn_domi_vid_command_ret
domi_vid_DEFINE_SYM_PTR(domi_vid_command_string)
#define domi_vid_command_string pfn_domi_vid_command_string
domi_vid_DEFINE_SYM_PTR(domi_vid_command_async)
#define domi_vid_command_async pfn_domi_vid_command_async
domi_vid_DEFINE_SYM_PTR(domi_vid_command_node_async)
#define domi_vid_command_node_async pfn_domi_vid_command_node_async
domi_vid_DEFINE_SYM_PTR(domi_vid_abort_async_command)
#define domi_vid_abort_async_command pfn_domi_vid_abort_async_command
domi_vid_DEFINE_SYM_PTR(domi_vid_set_property)
#define domi_vid_set_property pfn_domi_vid_set_property
domi_vid_DEFINE_SYM_PTR(domi_vid_set_property_string)
#define domi_vid_set_property_string pfn_domi_vid_set_property_string
domi_vid_DEFINE_SYM_PTR(domi_vid_del_property)
#define domi_vid_del_property pfn_domi_vid_del_property
domi_vid_DEFINE_SYM_PTR(domi_vid_set_property_async)
#define domi_vid_set_property_async pfn_domi_vid_set_property_async
domi_vid_DEFINE_SYM_PTR(domi_vid_get_property)
#define domi_vid_get_property pfn_domi_vid_get_property
domi_vid_DEFINE_SYM_PTR(domi_vid_get_property_string)
#define domi_vid_get_property_string pfn_domi_vid_get_property_string
domi_vid_DEFINE_SYM_PTR(domi_vid_get_property_osd_string)
#define domi_vid_get_property_osd_string pfn_domi_vid_get_property_osd_string
domi_vid_DEFINE_SYM_PTR(domi_vid_get_property_async)
#define domi_vid_get_property_async pfn_domi_vid_get_property_async
domi_vid_DEFINE_SYM_PTR(domi_vid_observe_property)
#define domi_vid_observe_property pfn_domi_vid_observe_property
domi_vid_DEFINE_SYM_PTR(domi_vid_unobserve_property)
#define domi_vid_unobserve_property pfn_domi_vid_unobserve_property
domi_vid_DEFINE_SYM_PTR(domi_vid_event_name)
#define domi_vid_event_name pfn_domi_vid_event_name
domi_vid_DEFINE_SYM_PTR(domi_vid_event_to_node)
#define domi_vid_event_to_node pfn_domi_vid_event_to_node
domi_vid_DEFINE_SYM_PTR(domi_vid_request_event)
#define domi_vid_request_event pfn_domi_vid_request_event
domi_vid_DEFINE_SYM_PTR(domi_vid_request_log_messages)
#define domi_vid_request_log_messages pfn_domi_vid_request_log_messages
domi_vid_DEFINE_SYM_PTR(domi_vid_wait_event)
#define domi_vid_wait_event pfn_domi_vid_wait_event
domi_vid_DEFINE_SYM_PTR(domi_vid_wakeup)
#define domi_vid_wakeup pfn_domi_vid_wakeup
domi_vid_DEFINE_SYM_PTR(domi_vid_set_wakeup_callback)
#define domi_vid_set_wakeup_callback pfn_domi_vid_set_wakeup_callback
domi_vid_DEFINE_SYM_PTR(domi_vid_wait_async_requests)
#define domi_vid_wait_async_requests pfn_domi_vid_wait_async_requests
domi_vid_DEFINE_SYM_PTR(domi_vid_hook_add)
#define domi_vid_hook_add pfn_domi_vid_hook_add
domi_vid_DEFINE_SYM_PTR(domi_vid_hook_continue)
#define domi_vid_hook_continue pfn_domi_vid_hook_continue
domi_vid_DEFINE_SYM_PTR(domi_vid_get_wakeup_pipe)
#define domi_vid_get_wakeup_pipe pfn_domi_vid_get_wakeup_pipe

#endif

#ifdef __cplusplus
}
#endif

#endif
