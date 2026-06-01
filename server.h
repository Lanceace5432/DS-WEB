/*
 * SkyeBank - Loan Eligibility Backend
 * server.h — HTTP server declarations
 */
 
#ifndef SERVER_H
#define SERVER_H
 
#include <stddef.h>
 
/* Callback type: receives raw request body, returns heap-allocated JSON string */
typedef char *(*RequestHandler)(const char *method,
                                const char *path,
                                const char *body);
 
/* Starts a blocking TCP server on the given port */
void server_start(int port, RequestHandler handler);
 
/* Helper: build a complete HTTP response string (caller must free) */
char *http_response(int status, const char *status_text,
                    const char *content_type, const char *body);
 
#endif /* SERVER_H */
 
