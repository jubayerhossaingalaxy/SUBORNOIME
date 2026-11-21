#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#if defined(SUBORNO_API_EXPORTS)
#define SUBORNO_API __declspec(dllexport)
#elif !defined(SUBORNO_API_STATIC)
#define SUBORNO_API __declspec(dllimport)
#else
#define SUBORNO_API
#endif
#else
#define SUBORNO_API
#endif

typedef struct SUBORNO_Engine SUBORNO_Engine;
typedef struct SUBORNO_Session SUBORNO_Session;

typedef enum SUBORNO_Status {
    SUBORNO_STATUS_UPDATED = 0,
    SUBORNO_STATUS_PENDING = 1,
    SUBORNO_STATUS_LITERAL_PASSTHROUGH = 2,
    SUBORNO_STATUS_INVALID_UTF8 = 3,
    SUBORNO_STATUS_INPUT_LIMIT = 4,
    SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE = 5,
    SUBORNO_STATUS_EMPTY = 6,
    SUBORNO_ERROR_INVALID_ARGUMENT = -1,
    SUBORNO_ERROR_BUFFER_TOO_SMALL = -2,
    SUBORNO_ERROR_INTERNAL = -3
} SUBORNO_Status;

SUBORNO_API SUBORNO_Engine* suborno_engine_create(void);
SUBORNO_API void suborno_engine_destroy(SUBORNO_Engine* engine);
SUBORNO_API SUBORNO_Session* suborno_session_create(SUBORNO_Engine* engine);
SUBORNO_API void suborno_session_destroy(SUBORNO_Session* session);
/* Functions returning status use SUBORNO_Status values. */
SUBORNO_API int suborno_session_append_utf8(SUBORNO_Session* session, const char* input);
SUBORNO_API int suborno_session_backspace(SUBORNO_Session* session);
/* UTF-8 output buffers include the terminating NUL in buffer_size. */
SUBORNO_API int suborno_session_snapshot(SUBORNO_Session* session,
                                         char* buffer,
                                         size_t buffer_size);
SUBORNO_API int suborno_session_candidate_count(SUBORNO_Session* session, size_t* count);
SUBORNO_API int suborno_session_candidate_snapshot(SUBORNO_Session* session,
                                                   size_t index,
                                                   char* buffer,
                                                   size_t buffer_size);
SUBORNO_API int suborno_session_select_candidate(SUBORNO_Session* session, size_t index);
/* On failure, commit outputs are set to NULL and 0. Free successful output with
 * suborno_string_free(). */
SUBORNO_API int suborno_session_commit(SUBORNO_Session* session,
                                       char** committed,
                                       size_t* committed_size);
SUBORNO_API int suborno_session_commit_candidate(SUBORNO_Session* session,
                                                 size_t index,
                                                 char** committed,
                                                 size_t* committed_size);
SUBORNO_API void suborno_string_free(char* text);

#ifdef __cplusplus
}
#endif
