/*
 * SkyeBank - Loan Eligibility Backend
 * json_utils.h — Lightweight JSON parser for incoming request bodies
 */

#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "eligibility.h"

/*
 * Parse a JSON object string into an Applicant struct.
 * Returns 1 on success, 0 on failure (fills error_buf).
 */
int applicant_from_json(const char *json,
                        Applicant   *out,
                        char        *error_buf,
                        int          error_buf_size);

/*
 * Minimal JSON value extractors (operate on raw JSON string).
 * All return 1 on success, 0 if key not found.
 */
int json_get_string(const char *json, const char *key,
                    char *out, int out_size);

int json_get_double(const char *json, const char *key, double *out);

int json_get_int   (const char *json, const char *key, int    *out);

#endif /* JSON_UTILS_H */
