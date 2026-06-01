/*
 * SkyeBank - Loan Eligibility Backend
 * router.c — URL dispatch table
 *
 * Routes:
 *   GET  /                        → 200 welcome JSON
 *   GET  /api/loan-types          → list of available loan types
 *   GET  /api/requirements/:type  → requirements checklist for a loan type
 *   POST /api/check-eligibility   → run eligibility evaluation
 *   *                             → 404
 */

#include "router.h"
#include "server.h"
#include "eligibility.h"
#include "json_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Route handlers (each returns heap-allocated HTTP response string)    */
/* ------------------------------------------------------------------ */

/* GET / */
static char *handle_root(void) {
    const char *body =
        "{"
          "\"bank\":\"SkyeBank\","
          "\"version\":\"1.0.0\","
          "\"description\":\"Loan Eligibility Decision-Support API\","
          "\"endpoints\":["
            "\"/api/loan-types\","
            "\"/api/requirements/:type\","
            "\"/api/check-eligibility\""
          "]"
        "}";
    return http_response(200, "OK", "application/json", body);
}

/* GET /api/loan-types */
static char *handle_loan_types(void) {
    const char *body =
        "{"
          "\"loan_types\":["
            "{\"id\":\"car\",     \"label\":\"Car Loan\","
             "\"description\":\"Finance a new or used vehicle\","
             "\"max_amount\":2000000},"
            "{\"id\":\"housing\", \"label\":\"Housing Loan\","
             "\"description\":\"Purchase or build your home\","
             "\"max_amount\":10000000},"
            "{\"id\":\"business\",\"label\":\"Business Loan\","
             "\"description\":\"Grow or start your business\","
             "\"max_amount\":5000000}"
          "]"
        "}";
    return http_response(200, "OK", "application/json", body);
}

/* GET /api/requirements/car  (or housing / business) */
static char *handle_requirements(const char *path) {
    /* Extract the loan type from the last path segment */
    const char *prefix = "/api/requirements/";
    if (strncmp(path, prefix, strlen(prefix)) != 0)
        return http_response(400, "Bad Request", "application/json",
                             "{\"error\":\"Missing loan type in path\"}");

    const char *loan_type = path + strlen(prefix);
    const char *body = NULL;

    if (strcmp(loan_type, "car") == 0) {
        body =
          "{"
            "\"loan_type\":\"car\","
            "\"required_fields\":["
              "{\"name\":\"full_name\",       \"type\":\"string\",  \"label\":\"Full Name\"},"
              "{\"name\":\"age\",             \"type\":\"integer\", \"label\":\"Age\"},"
              "{\"name\":\"monthly_income\",  \"type\":\"number\",  \"label\":\"Monthly Income (PHP)\"},"
              "{\"name\":\"employment_type\", \"type\":\"string\",  \"label\":\"Employment Type (employed/self-employed)\"},"
              "{\"name\":\"years_employed\",  \"type\":\"number\",  \"label\":\"Years Employed\"},"
              "{\"name\":\"credit_score\",    \"type\":\"integer\", \"label\":\"Credit Score (300-850)\"},"
              "{\"name\":\"existing_loans\",  \"type\":\"integer\", \"label\":\"Number of Existing Loans\"},"
              "{\"name\":\"loan_amount\",     \"type\":\"number\",  \"label\":\"Requested Loan Amount (PHP)\"}"
            "],"
            "\"criteria_summary\":\"Minimum age 21, credit score ≥ 650, 2+ years employed, monthly income ≥ 25000\""
          "}";

    } else if (strcmp(loan_type, "housing") == 0) {
        body =
          "{"
            "\"loan_type\":\"housing\","
            "\"required_fields\":["
              "{\"name\":\"full_name\",        \"type\":\"string\",  \"label\":\"Full Name\"},"
              "{\"name\":\"age\",              \"type\":\"integer\", \"label\":\"Age\"},"
              "{\"name\":\"monthly_income\",   \"type\":\"number\",  \"label\":\"Monthly Income (PHP)\"},"
              "{\"name\":\"employment_type\",  \"type\":\"string\",  \"label\":\"Employment Type (employed/self-employed)\"},"
              "{\"name\":\"years_employed\",   \"type\":\"number\",  \"label\":\"Years Employed\"},"
              "{\"name\":\"credit_score\",     \"type\":\"integer\", \"label\":\"Credit Score (300-850)\"},"
              "{\"name\":\"existing_loans\",   \"type\":\"integer\", \"label\":\"Number of Existing Loans\"},"
              "{\"name\":\"loan_amount\",      \"type\":\"number\",  \"label\":\"Requested Loan Amount (PHP)\"},"
              "{\"name\":\"collateral_value\", \"type\":\"number\",  \"label\":\"Property / Collateral Value (PHP)\"}"
            "],"
            "\"criteria_summary\":\"Minimum age 21, credit score ≥ 700, 3+ years employed, monthly income ≥ 50000, collateral ≥ 80% of loan\""
          "}";

    } else if (strcmp(loan_type, "business") == 0) {
        body =
          "{"
            "\"loan_type\":\"business\","
            "\"required_fields\":["
              "{\"name\":\"full_name\",          \"type\":\"string\",  \"label\":\"Full Name\"},"
              "{\"name\":\"age\",                \"type\":\"integer\", \"label\":\"Age\"},"
              "{\"name\":\"monthly_income\",     \"type\":\"number\",  \"label\":\"Monthly Business Revenue (PHP)\"},"
              "{\"name\":\"employment_type\",    \"type\":\"string\",  \"label\":\"Employment Type (employed/self-employed)\"},"
              "{\"name\":\"years_employed\",     \"type\":\"number\",  \"label\":\"Years in Business / Employment\"},"
              "{\"name\":\"credit_score\",       \"type\":\"integer\", \"label\":\"Credit Score (300-850)\"},"
              "{\"name\":\"existing_loans\",     \"type\":\"integer\", \"label\":\"Number of Existing Loans\"},"
              "{\"name\":\"loan_amount\",        \"type\":\"number\",  \"label\":\"Requested Loan Amount (PHP)\"},"
              "{\"name\":\"business_years\",     \"type\":\"number\",  \"label\":\"Years Business Has Been Operating\"}"
            "],"
            "\"criteria_summary\":\"Minimum age 21, credit score ≥ 680, 2+ years in business, monthly revenue ≥ 80000\""
          "}";

    } else {
        return http_response(404, "Not Found", "application/json",
                             "{\"error\":\"Unknown loan type. Use: car, housing, or business\"}");
    }

    return http_response(200, "OK", "application/json", body);
}

/* POST /api/check-eligibility */
static char *handle_check_eligibility(const char *body) {
    if (!body || strlen(body) == 0) {
        return http_response(400, "Bad Request", "application/json",
                             "{\"error\":\"Request body is required\"}");
    }

    /* Parse applicant from JSON body */
    Applicant applicant;
    memset(&applicant, 0, sizeof(applicant));

    char parse_error[256] = {0};
    if (!applicant_from_json(body, &applicant, parse_error, sizeof(parse_error))) {
        char err_json[512];
        snprintf(err_json, sizeof(err_json),
                 "{\"error\":\"Invalid request body\",\"detail\":\"%s\"}", parse_error);
        return http_response(400, "Bad Request", "application/json", err_json);
    }

    /* Run eligibility engine */
    EligibilityResult result;
    check_eligibility(&applicant, &result);

    /* Serialise result to JSON */
    char *result_json = eligibility_result_to_json(&result);
    if (!result_json)
        return http_response(500, "Internal Server Error", "application/json",
                             "{\"error\":\"Failed to serialize result\"}");

    char *response = http_response(200, "OK", "application/json", result_json);
    free(result_json);
    return response;
}

/* ------------------------------------------------------------------ */
/* Main dispatch                                                         */
/* ------------------------------------------------------------------ */

char *route_request(const char *method, const char *path, const char *body) {
    /* Strip query string for matching */
    char clean_path[256];
    strncpy(clean_path, path, sizeof(clean_path) - 1);
    clean_path[sizeof(clean_path) - 1] = '\0';
    char *qs = strchr(clean_path, '?');
    if (qs) *qs = '\0';

    /* GET / */
    if (strcmp(method, "GET") == 0 && strcmp(clean_path, "/") == 0)
        return handle_root();

    /* GET /api/loan-types */
    if (strcmp(method, "GET") == 0 && strcmp(clean_path, "/api/loan-types") == 0)
        return handle_loan_types();

    /* GET /api/requirements/:type */
    if (strcmp(method, "GET") == 0 && strncmp(clean_path, "/api/requirements/", 18) == 0)
        return handle_requirements(clean_path);

    /* POST /api/check-eligibility */
    if (strcmp(method, "POST") == 0 && strcmp(clean_path, "/api/check-eligibility") == 0)
        return handle_check_eligibility(body);

    /* 404 catch-all */
    return http_response(404, "Not Found", "application/json",
                         "{\"error\":\"Route not found\"}");
}
