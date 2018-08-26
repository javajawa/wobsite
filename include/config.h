// Connection timeouts
#define READ_TIMEOUT_SEC 3
#define CONN_TIMEOUT_SEC 1

// Request Headers
#define MAX_HEADER_LENGTH   1024
#define MAX_METHOD_LENGTH      8
#define MAX_REQUEST_LENGTH   256
#define MAX_BODY_LENGTH     ( 1 << 20 )

// Threading Settings
#define RESPONDER_THREADS 3

#include <logging.h>

#ifdef DEBUG
#define LOG_THREAD VERB
#else
#define LOG_THREAD WARN
#endif

#ifdef DEBUG
#define LOG_NET    VERB
#else
#define LOG_NET    WARN
#endif
