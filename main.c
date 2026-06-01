/*
 * SkyeBank - Loan Eligibility Backend
 * main.c — Entry point, starts the HTTP server
 */
 
#include "server.h"
#include "router.h"
#include <stdio.h>
#include <stdlib.h>
 
#define PORT 8080
 
int main(void) {
    printf("=========================================\n");
    printf("  SkyeBank Loan Eligibility Backend\n");
    printf("  Listening on http://localhost:%d\n", PORT);
    printf("=========================================\n\n");
 
    server_start(PORT, route_request);
    return 0;
}
 
