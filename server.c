/*
 * SkyeBank - Loan Eligibility Backend
 * server.c — Minimal HTTP/1.1 server (single-threaded, for demo/dev use)
 *
 * Handles:
 *   - GET  requests (no body)
 *   - POST requests (reads Content-Length body)
 *   - Automatic CORS headers so the frontend JS can call freely
 *   - OPTIONS preflight (CORS pre-flight)
 */

#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Platform socket includes ---- */
#ifdef _WIN32
  #include <winsock2.h>
  #pragma comment(lib,"ws2_32.lib")
  typedef int socklen_t;
  #define CLOSE_SOCKET(s) closesocket(s)
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
  #include <arpa/inet.h>
  #define SOCKET int
  #define INVALID_SOCKET (-1)
  #define CLOSE_SOCKET(s) close(s)
#endif

#define BUFFER_SIZE   (1024 * 64)   /* 64 KB read buffer              */
#define MAX_BODY_SIZE (1024 * 32)   /* 32 KB max POST body            */

/* ------------------------------------------------------------------ */
/* Internal helpers                                                     */
/* ------------------------------------------------------------------ */

/* Case-insensitive substring search */
static const char *istr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    size_t nl = strlen(needle);
    for (; *haystack; haystack++) {
        if (strncasecmp(haystack, needle, nl) == 0)
            return haystack;
    }
    return NULL;
}

/* Parse "Content-Length: NNN\r\n" → returns NNN, or 0 if not found */
static size_t parse_content_length(const char *headers) {
    const char *p = istr(headers, "content-length:");
    if (!p) return 0;
    p += strlen("content-length:");
    while (*p == ' ') p++;
    return (size_t)atol(p);
}

/* Build HTTP response (caller must free) */
char *http_response(int status, const char *status_text,
                    const char *content_type, const char *body) {
    /* CORS headers allow the landing page (any origin) to call us */
    const char *cors =
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n";

    size_t body_len = body ? strlen(body) : 0;
    /* 512 bytes overhead is plenty for headers */
    size_t total    = body_len + 512;
    char  *resp     = malloc(total);
    if (!resp) return NULL;

    snprintf(resp, total,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "%s"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        status, status_text,
        content_type ? content_type : "text/plain",
        body_len,
        cors,
        body ? body : ""
    );
    return resp;
}

/* ------------------------------------------------------------------ */
/* Server loop                                                          */
/* ------------------------------------------------------------------ */

void server_start(int port, RequestHandler handler) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif

    /* --- Create listening socket --- */
    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        perror("socket");
        exit(1);
    }

    /* Allow rapid restart without "Address already in use" */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((uint16_t)port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(1);
    }

    /* --- Accept loop --- */
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        SOCKET client_fd = accept(server_fd,
                                  (struct sockaddr *)&client_addr,
                                  &addr_len);
        if (client_fd == INVALID_SOCKET) { continue; }

        printf("[+] Connection from %s\n", inet_ntoa(client_addr.sin_addr));

        /* Read raw request into buffer */
        char *buf = calloc(BUFFER_SIZE, 1);
        if (!buf) { CLOSE_SOCKET(client_fd); continue; }

        ssize_t bytes = recv(client_fd, buf, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            free(buf);
            CLOSE_SOCKET(client_fd);
            continue;
        }
        buf[bytes] = '\0';

        /* --- Parse request line: "METHOD /path HTTP/1.x\r\n" --- */
        char method[16] = {0}, path[256] = {0};
        sscanf(buf, "%15s %255s", method, path);
        printf("    %s %s\n", method, path);

        /* --- Handle CORS preflight --- */
        if (strcmp(method, "OPTIONS") == 0) {
            char *resp = http_response(204, "No Content", "text/plain", "");
            send(client_fd, resp, strlen(resp), 0);
            free(resp);
            free(buf);
            CLOSE_SOCKET(client_fd);
            continue;
        }

        /* --- Extract body (POST) --- */
        char *body = "";
        char *body_alloc = NULL;
        const char *header_end = strstr(buf, "\r\n\r\n");
        if (header_end) {
            header_end += 4; /* skip blank line */
            size_t cl = parse_content_length(buf);
            if (cl > 0 && cl <= MAX_BODY_SIZE) {
                body_alloc = calloc(cl + 1, 1);
                if (body_alloc) {
                    memcpy(body_alloc, header_end, cl);
                    body = body_alloc;
                }
            }
        }

        /* --- Dispatch to router --- */
        char *response_str = handler(method, path, body);

        /* Send response */
        if (response_str) {
            send(client_fd, response_str, strlen(response_str), 0);
            free(response_str);
        } else {
            char *err = http_response(500, "Internal Server Error",
                                      "application/json",
                                      "{\"error\":\"Internal server error\"}");
            send(client_fd, err, strlen(err), 0);
            free(err);
        }

        free(body_alloc);
        free(buf);
        CLOSE_SOCKET(client_fd);
    }

    CLOSE_SOCKET(server_fd);
}
