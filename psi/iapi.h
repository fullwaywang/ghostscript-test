/* Copyright (C) 2001-2019 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied,
   modified or distributed except as expressly authorized under the terms
   of the license contained in the file LICENSE in this distribution.

   Refer to licensing information at http://www.artifex.com or contact
   Artifex Software, Inc.,  1305 Grant Avenue - Suite 200, Novato,
   CA 94945, U.S.A., +1(415)492-9861, for further information.
*/



/*
 * Public API for Ghostscript interpreter
 * for use both as DLL and for static linking.
 *
 * Should work for Windows, OS/2, Linux, Mac.
 *
 * DLL exported functions should be as similar as possible to imain.c
 * You will need to include "ierrors.h".
 *
 * Current problems:
 * 1. Ghostscript does not support multiple instances.
 * 2. Global variables in gs_main_instance_default()
 *    and gsapi_instance_counter
 */

/* Exported functions may need different prefix
 *  GSDLLEXPORT marks functions as exported
 *  GSDLLAPI is the calling convention used on functions exported
 *   by Ghostscript
 *  GSDLLCALL is used on callback functions called by Ghostscript
 * When you include this header file in the caller, you may
 * need to change the definitions by defining these
 * before including this header file.
 * Make sure you get the calling convention correct, otherwise your
 * program will crash either during callbacks or soon after returning
 * due to stack corruption.
 */

#ifndef iapi_INCLUDED
#  define iapi_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WINDOWS_) || defined(__WINDOWS__)
# ifndef _Windows
#  define _Windows
# endif
#endif

#ifdef _Windows
# ifndef GSDLLEXPORT
/* We don't need both the "__declspec(dllexport)" declaration
 * and the listing in the .def file - having both results in
 * a linker warning on x64 builds (but is incorrect on x86, too)
 */
#  if 0
#    define GSDLLEXPORT __declspec(dllexport)
#  else
#    define GSDLLEXPORT
#  endif
# endif
# ifndef GSDLLAPI
#  define GSDLLAPI __stdcall
# endif
# ifndef GSDLLCALL
#  define GSDLLCALL __stdcall
# endif
#endif  /* _Windows */

#if defined(OS2) && defined(__IBMC__)
# ifndef GSDLLAPI
#  define GSDLLAPI _System
# endif
# ifndef GSDLLCALL
#  define GSDLLCALL _System
# endif
#endif	/* OS2 && __IBMC */

#ifdef __MACOS__
# pragma export on
#endif

#ifndef GSDLLEXPORT
# define GSDLLEXPORT
#endif
#ifndef GSDLLAPI
# define GSDLLAPI
#endif
#ifndef GSDLLCALL
# define GSDLLCALL
#endif

#if defined(__IBMC__)
# define GSDLLAPIPTR * GSDLLAPI
# define GSDLLCALLPTR * GSDLLCALL
#else
# define GSDLLAPIPTR GSDLLAPI *
# define GSDLLCALLPTR GSDLLCALL *
#endif

#ifndef display_callback_DEFINED
# define display_callback_DEFINED
typedef struct display_callback_s display_callback;
#endif

#ifndef gs_memory_DEFINED
#  define gs_memory_DEFINED
typedef struct gs_memory_s gs_memory_t;
#endif

#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;
#endif

typedef struct gsapi_revision_s {
    const char *product;
    const char *copyright;
    long revision;
    long revisiondate;
} gsapi_revision_t;

/* Get version numbers and strings.
 * This is safe to call at any time.
 * You should call this first to make sure that the correct version
 * of the Ghostscript is being used.
 * pr is a pointer to a revision structure.
 * len is the size of this structure in bytes.
 * Returns 0 if OK, or if len too small (additional parameters
 * have been added to the structure) it will return the required
 * size of the structure.
 */
GSDLLEXPORT int GSDLLAPI
gsapi_revision(gsapi_revision_t *pr, int len);

/*
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 *  Ghostscript supports only one instance.
 *  The current implementation uses a global static instance
 *  counter to make sure that only a single instance is used.
 *  If you try to create two instances, the second attempt
 *  will return < 0 and set pinstance to NULL.
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 */
/* Create a new instance of Ghostscript.
 * This instance is passed to most other API functions.
 * The caller_handle will be provided to callback functions.
 */

GSDLLEXPORT int GSDLLAPI
gsapi_new_instance(void **pinstance, void *caller_handle);

/*
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 *  Ghostscript supports only one instance.
 *  The current implementation uses a global static instance
 *  counter to make sure that only a single instance is used.
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 */
/* Destroy an instance of Ghostscript
 * Before you call this, Ghostscript must have finished.
 * If Ghostscript has been initialised, you must call gsapi_exit()
 * before gsapi_delete_instance.
 */
GSDLLEXPORT void GSDLLAPI
gsapi_delete_instance(void *instance);

/* Set the callback functions for stdio
 * The stdin callback function should return the number of
 * characters read, 0 for EOF, or -1 for error.
 * The stdout and stderr callback functions should return
 * the number of characters written.
 * If a callback address is NULL, the real stdio will be used.
 */
GSDLLEXPORT int GSDLLAPI
gsapi_set_stdio(void *instance,
    int (GSDLLCALLPTR stdin_fn)(void *caller_handle, char *buf, int len),
    int (GSDLLCALLPTR stdout_fn)(void *caller_handle, const char *str, int len),
    int (GSDLLCALLPTR stderr_fn)(void *caller_handle, const char *str, int len));

/* Set the callback function for polling.
 * This is used for handling window events or cooperative
 * multitasking.  This function will only be called if
 * Ghostscript was compiled with CHECK_INTERRUPTS
 * as described in gpcheck.h.
 * The polling function should return 0 if all is well,
 * and negative if it wants ghostscript to abort.
 * The polling function must be fast.
 */
GSDLLEXPORT int GSDLLAPI gsapi_set_poll(void *instance,
    int (GSDLLCALLPTR poll_fn)(void *caller_handle));

/* Set the display device callback structure.
 * If the display device is used, this must be called
 * after gsapi_new_instance() and before gsapi_init_with_args().
 * See gdevdisp.h for more details.
 */
GSDLLEXPORT int GSDLLAPI gsapi_set_display_callback(
   void *instance, display_callback *callback);

/* Set the string containing the list of default device names
 * for example "display x11alpha x11 bbox". Allows the calling
 * application to influence which device(s) gs will try in order
 * to select the default device
 *
 * *Must* be called after gsapi_new_instance() and before
 * gsapi_init_with_args().
 */
GSDLLEXPORT int GSDLLAPI
gsapi_set_default_device_list(void *instance, char *list, int listlen);

/* Returns a pointer to the current default device string
 * *Must* be called after gsapi_new_instance().
 */
GSDLLEXPORT int GSDLLAPI
gsapi_get_default_device_list(void *instance, char **list, int *listlen);

/* Set the encoding used for the args. By default we assume
 * 'local' encoding. For windows this equates to whatever the current
 * codepage is. For linux this is utf8.
 *
 * Use of this API (gsapi) with 'local' encodings (and hence without calling
 * this function) is now deprecated!
 */
GSDLLEXPORT int GSDLLAPI gsapi_set_arg_encoding(void *instance,
                                                int encoding);

enum {
    GS_ARG_ENCODING_LOCAL = 0,
    GS_ARG_ENCODING_UTF8 = 1,
    GS_ARG_ENCODING_UTF16LE = 2
};

/* Initialise the interpreter.
 * This calls gs_main_init_with_args() in imainarg.c
 * 1. If quit or EOF occur during gsapi_init_with_args(),
 *    the return value will be gs_error_Quit.  This is not an error.
 *    You must call gsapi_exit() and must not call any other
 *    gsapi_XXX functions.
 * 2. If usage info should be displayed, the return value will be gs_error_Info
 *    which is not an error.  Do not call gsapi_exit().
 * 3. Under normal conditions this returns 0.  You would then
 *    call one or more gsapi_run_*() functions and then finish
 *    with gsapi_exit().
 */
GSDLLEXPORT int GSDLLAPI gsapi_init_with_args(void *instance,
    int argc, char **argv);

#ifdef __WIN32__
GSDLLEXPORT int GSDLLAPI gsapi_init_with_argsA(void *instance,
    int argc, char **argv);

GSDLLEXPORT int GSDLLAPI gsapi_init_with_argsW(void *instance,
    int argc, wchar_t **argv);
#endif

/*
 * The gsapi_run_* functions are like gs_main_run_* except
 * that the error_object is omitted.
 * If these functions return <= -100, either quit or a fatal
 * error has occured.  You then call gsapi_exit() next.
 * The only exception is gsapi_run_string_continue()
 * which will return gs_error_NeedInput if all is well.
 */

GSDLLEXPORT int GSDLLAPI
gsapi_run_string_begin(void *instance,
    int user_errors, int *pexit_code);

GSDLLEXPORT int GSDLLAPI
gsapi_run_string_continue(void *instance,
    const char *str, unsigned int length, int user_errors, int *pexit_code);

GSDLLEXPORT int GSDLLAPI
gsapi_run_string_end(void *instance,
    int user_errors, int *pexit_code);

GSDLLEXPORT int GSDLLAPI
gsapi_run_string_with_length(void *instance,
    const char *str, unsigned int length, int user_errors, int *pexit_code);

GSDLLEXPORT int GSDLLAPI
gsapi_run_string(void *instance,
    const char *str, int user_errors, int *pexit_code);

GSDLLEXPORT int GSDLLAPI
gsapi_run_file(void *instance,
    const char *file_name, int user_errors, int *pexit_code);

#ifdef __WIN32__
GSDLLEXPORT int GSDLLAPI
gsapi_run_fileA(void *instance,
    const char *file_name, int user_errors, int *pexit_code);

GSDLLEXPORT int GSDLLAPI
gsapi_run_fileW(void *instance,
    const wchar_t *file_name, int user_errors, int *pexit_code);
#endif

/* Retrieve the memory allocator for the interpreter instance */
GSDLLEXPORT gs_memory_t * GSDLLAPI
gsapi_get_device_memory(void *instance);

/* Set the device */
GSDLLEXPORT int GSDLLAPI
gsapi_set_device(void *instance, gx_device *pdev);

/* Exit the interpreter.
 * This must be called on shutdown if gsapi_init_with_args()
 * has been called, and just before gsapi_delete_instance().
 */
GSDLLEXPORT int GSDLLAPI
gsapi_exit(void *instance);

typedef enum {
    gs_spt_invalid = -1,
    gs_spt_null    = 0,   /* void * is NULL */
    gs_spt_bool    = 1,   /* void * is NULL (false) or non-NULL (true) */
    gs_spt_int     = 2,   /* void * is a pointer to an int */
    gs_spt_float   = 3,   /* void * is a float * */
    gs_spt_name    = 4,   /* void * is a char * */
    gs_spt_string  = 5    /* void * is a char * */
} gs_set_param_type;
GSDLLEXPORT int GSDLLAPI gsapi_set_param(void *instance, gs_set_param_type type, const char *param, const void *value);

enum {
    GS_PERMIT_FILE_READING = 0,
    GS_PERMIT_FILE_WRITING = 1,
    GS_PERMIT_FILE_CONTROL = 2
};

/* Add a path to one of the sets of permitted paths. */
GSDLLEXPORT int GSDLLAPI
gsapi_add_control_path(void *instance, int type, const char *path);

/* Remove a path from one of the sets of permitted paths. */
GSDLLEXPORT int GSDLLAPI
gsapi_remove_control_path(void *instance, int type, const char *path);

/* Purge all the paths from the one of the sets of permitted paths. */
GSDLLEXPORT void GSDLLAPI
gsapi_purge_control_paths(void *instance, int type);

GSDLLEXPORT void GSDLLAPI
gsapi_activate_path_control(void *instance, int enable);

GSDLLEXPORT int GSDLLAPI
gsapi_is_path_control_active(void *instance);

/* function prototypes */
typedef int (GSDLLAPIPTR PFN_gsapi_revision)(
    gsapi_revision_t *pr, int len);
typedef int (GSDLLAPIPTR PFN_gsapi_new_instance)(
    void **pinstance, void *caller_handle);
typedef void (GSDLLAPIPTR PFN_gsapi_delete_instance)(
    void *instance);
typedef int (GSDLLAPIPTR PFN_gsapi_set_stdio)(void *instance,
    int (GSDLLCALLPTR stdin_fn)(void *caller_handle, char *buf, int len),
    int (GSDLLCALLPTR stdout_fn)(void *caller_handle, const char *str, int len),
    int (GSDLLCALLPTR stderr_fn)(void *caller_handle, const char *str, int len));
typedef int (GSDLLAPIPTR PFN_gsapi_set_poll)(void *instance,
    int(GSDLLCALLPTR poll_fn)(void *caller_handle));
typedef int (GSDLLAPIPTR PFN_gsapi_set_display_callback)(
    void *instance, display_callback *callback);
typedef int (GSDLLAPIPTR PFN_gsapi_set_default_device_list)(
    void *instance, char *list, int listlen);
typedef int (GSDLLAPIPTR PFN_gsapi_get_default_device_list)(
    void *instance, char **list, int *listlen);
typedef int (GSDLLAPIPTR PFN_gsapi_init_with_args)(
    void *instance, int argc, char **argv);
#ifdef __WIN32__
typedef int (GSDLLAPIPTR PFN_gsapi_init_with_argsA)(
    void *instance, int argc, char **argv);
typedef int (GSDLLAPIPTR PFN_gsapi_init_with_argsW)(
    void *instance, int argc, wchar_t **argv);
#endif
typedef int (GSDLLAPIPTR PFN_gsapi_set_arg_encoding)(
    void *instance, int encoding);
typedef int (GSDLLAPIPTR PFN_gsapi_run_string_begin)(
    void *instance, int user_errors, int *pexit_code);
typedef int (GSDLLAPIPTR PFN_gsapi_run_string_continue)(
    void *instance, const char *str, unsigned int length,
    int user_errors, int *pexit_code);
typedef int (GSDLLAPIPTR PFN_gsapi_run_string_end)(
    void *instance, int user_errors, int *pexit_code);
typedef int (GSDLLAPIPTR PFN_gsapi_run_string_with_length)(
    void *instance, const char *str, unsigned int length,
    int user_errors, int *pexit_code);
typedef int (GSDLLAPIPTR PFN_gsapi_run_string)(
    void *instance, const char *str,
    int user_errors, int *pexit_code);
typedef int (GSDLLAPIPTR PFN_gsapi_run_file)(void *instance,
    const char *file_name, int user_errors, int *pexit_code);
#ifdef __WIN32__
typedef int (GSDLLAPIPTR PFN_gsapi_run_fileA)(void *instance,
    const char *file_name, int user_errors, int *pexit_code);
typedef int (GSDLLAPIPTR PFN_gsapi_run_fileW)(void *instance,
    const wchar_t *file_name, int user_errors, int *pexit_code);
#endif
typedef gs_memory_t * (GSDLLAPIPTR PFN_gsapi_get_device_memory)(void *instance);
typedef gs_memory_t * (GSDLLAPIPTR PFN_gsapi_set_device)(void *instance, gx_device *pdev);
typedef int (GSDLLAPIPTR PFN_gsapi_exit)(void *instance);
typedef int (GSDLLAPIPTR PFN_gsapi_set_param)(void *instance, gs_set_param_type type, const char *param, const void *value);

typedef int (GSDLLAPIPTR PFN_gsapi_add_control_path)(void *instance, int type, const char *path);
typedef int (GSDLLAPIPTR PFN_gsapi_remove_control_path)(void *instance, int type, const char *path);
typedef void (GSDLLAPIPTR PFN_gsapi_purge_control_paths)(void *instance, int type);
typedef void (GSDLLAPIPTR PFN_gsapi_activate_path_control)(void *instance, int enable);
typedef int (GSDLLAPIPTR PFN_gsapi_is_path_control_active)(void *instance);


#ifdef __MACOS__
#pragma export off
#endif

#ifdef __cplusplus
} /* extern 'C' protection */
#endif

#endif /* iapi_INCLUDED */
