/*
 * SkyeBank - Loan Eligibility Backend
 * router.h — URL router declarations
 */

#ifndef ROUTER_H
#define ROUTER_H

/* Main dispatch function — matches method+path and calls the right handler */
char *route_request(const char *method, const char *path, const char *body);

#endif /* ROUTER_H */
