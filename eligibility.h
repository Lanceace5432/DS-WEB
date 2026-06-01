/*
 * SkyeBank - Loan Eligibility Backend
 * eligibility.h — Core decision engine declarations
 */

#ifndef ELIGIBILITY_H
#define ELIGIBILITY_H

#include <stddef.h>

/* ------------------------------------------------------------------
 * Loan type identifiers
 * ------------------------------------------------------------------ */
typedef enum {
    LOAN_UNKNOWN  = 0,
    LOAN_CAR      = 1,
    LOAN_HOUSING  = 2,
    LOAN_BUSINESS = 3
} LoanType;

/* ------------------------------------------------------------------
 * Applicant profile — populated from the incoming JSON POST body
 * ------------------------------------------------------------------ */
typedef struct {
    char   full_name[128];
    int    age;
    double monthly_income;     /* PHP */
    char   employment_type[64]; /* "employed" | "self-employed" */
    double years_employed;
    int    credit_score;        /* 300–850 */
    int    existing_loans;
    double loan_amount;         /* PHP */
    double collateral_value;    /* PHP — housing only */
    double business_years;      /* business only */
    LoanType loan_type;
} Applicant;

/* ------------------------------------------------------------------
 * A single criterion check
 * ------------------------------------------------------------------ */
typedef struct {
    char   name[64];
    char   description[256];
    int    passed;              /* 1 = pass, 0 = fail */
    char   actual_value[64];    /* Human-readable actual  */
    char   required_value[64];  /* Human-readable required */
} Criterion;

#define MAX_CRITERIA 12

/* ------------------------------------------------------------------
 * Full eligibility result
 * ------------------------------------------------------------------ */
typedef struct {
    int       eligible;                   /* 1 = Eligible, 0 = Denied  */
    char      verdict[16];                /* "ELIGIBLE" | "DENIED"     */
    char      message[512];               /* Human-readable summary    */
    char      next_step[256];             /* What the applicant does next */
    int       score;                      /* 0–100 internal risk score */
    int       criteria_count;
    Criterion criteria[MAX_CRITERIA];
} EligibilityResult;

/* ------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------ */

/* Run the eligibility engine — fills *result */
void check_eligibility(const Applicant *applicant, EligibilityResult *result);

/* Serialise result to a heap-allocated JSON string (caller must free) */
char *eligibility_result_to_json(const EligibilityResult *result);

/* Utility: string → LoanType */
LoanType loan_type_from_string(const char *s);

#endif /* ELIGIBILITY_H */
