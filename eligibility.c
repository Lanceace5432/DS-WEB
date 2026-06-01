/*
 * SkyeBank - Loan Eligibility Backend
 * eligibility.c — Decision engine for Car, Housing, and Business loans
 *
 * Scoring model:
 *   Each criterion contributes points when passed.
 *   Final score 0-100; >= 60 = Eligible (all mandatory criteria must also pass).
 */

#include "eligibility.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

static void add_criterion(EligibilityResult *r,
                           const char *name,
                           const char *description,
                           int passed,
                           const char *actual,
                           const char *required) {
    if (r->criteria_count >= MAX_CRITERIA) return;
    Criterion *c = &r->criteria[r->criteria_count++];
    strncpy(c->name,           name,        sizeof(c->name) - 1);
    strncpy(c->description,    description, sizeof(c->description) - 1);
    strncpy(c->actual_value,   actual,      sizeof(c->actual_value) - 1);
    strncpy(c->required_value, required,    sizeof(c->required_value) - 1);
    c->passed = passed;
}

LoanType loan_type_from_string(const char *s) {
    if (!s) return LOAN_UNKNOWN;
    if (strcasecmp(s, "car")      == 0) return LOAN_CAR;
    if (strcasecmp(s, "housing")  == 0) return LOAN_HOUSING;
    if (strcasecmp(s, "business") == 0) return LOAN_BUSINESS;
    return LOAN_UNKNOWN;
}

/* Clamp a debt-to-income ratio: monthly payment ≈ loan / 60 (5-yr term) */
static double estimated_monthly_payment(double loan_amount) {
    return loan_amount / 60.0;
}

/* ------------------------------------------------------------------ */
/* Shared basic criteria (all loan types)                               */
/* ------------------------------------------------------------------ */

/*
 * Returns number of passed mandatory criteria (age, employment basic checks).
 * score_pts is incremented for each passing criterion.
 */
static int check_common_criteria(const Applicant *a,
                                  EligibilityResult *r,
                                  int *score_pts) {
    int mandatory_failures = 0;
    char actual[64], required[64];

    /* 1. Age ≥ 21 (mandatory) */
    snprintf(actual,   sizeof(actual),   "%d years", a->age);
    snprintf(required, sizeof(required), "At least 21 years");
    int age_ok = (a->age >= 21 && a->age <= 70);
    add_criterion(r, "age", "Applicant must be 21–70 years old",
                  age_ok, actual, required);
    if (!age_ok) mandatory_failures++;
    else         *score_pts += 10;

    /* 2. Credit score basic (non-zero, in valid range) */
    snprintf(actual,   sizeof(actual),   "%d", a->credit_score);
    snprintf(required, sizeof(required), "300–850");
    int cs_valid = (a->credit_score >= 300 && a->credit_score <= 850);
    add_criterion(r, "credit_score_range", "Credit score must be in valid range",
                  cs_valid, actual, required);
    if (!cs_valid) mandatory_failures++;
    else           *score_pts += 5;

    /* 3. Positive income */
    snprintf(actual,   sizeof(actual),   "PHP %.2f", a->monthly_income);
    snprintf(required, sizeof(required), "Greater than 0");
    int income_pos = (a->monthly_income > 0.0);
    add_criterion(r, "income_positive", "Monthly income must be positive",
                  income_pos, actual, required);
    if (!income_pos) mandatory_failures++;
    else             *score_pts += 5;

    /* 4. Existing loans ≤ 3 */
    snprintf(actual,   sizeof(actual),   "%d existing loan(s)", a->existing_loans);
    snprintf(required, sizeof(required), "≤ 3 existing loans");
    int loans_ok = (a->existing_loans <= 3);
    add_criterion(r, "existing_loans", "Must have 3 or fewer existing loans",
                  loans_ok, actual, required);
    if (!loans_ok) *score_pts -= 10;   /* penalty but not mandatory */

    return mandatory_failures;
}

/* ------------------------------------------------------------------ */
/* CAR LOAN eligibility                                                 */
/* ------------------------------------------------------------------ */
static void check_car_loan(const Applicant *a, EligibilityResult *r) {
    int score = 0;
    int mandatory_fails = check_common_criteria(a, r, &score);

    char actual[64], required[64];

    /* Credit score ≥ 650 */
    snprintf(actual,   sizeof(actual),   "%d", a->credit_score);
    snprintf(required, sizeof(required), "≥ 650");
    int cs_ok = (a->credit_score >= 650);
    add_criterion(r, "credit_score", "Credit score must be 650 or higher",
                  cs_ok, actual, required);
    if (!cs_ok) mandatory_fails++;
    else score += 20;

    /* Monthly income ≥ PHP 25,000 */
    snprintf(actual,   sizeof(actual),   "PHP %.2f", a->monthly_income);
    snprintf(required, sizeof(required), "PHP 25,000");
    int inc_ok = (a->monthly_income >= 25000.0);
    add_criterion(r, "minimum_income", "Monthly income must be at least PHP 25,000",
                  inc_ok, actual, required);
    if (!inc_ok) mandatory_fails++;
    else score += 20;

    /* Years employed ≥ 2 */
    snprintf(actual,   sizeof(actual),   "%.1f years", a->years_employed);
    snprintf(required, sizeof(required), "≥ 2 years");
    int emp_ok = (a->years_employed >= 2.0);
    add_criterion(r, "employment_duration", "Must be employed for at least 2 years",
                  emp_ok, actual, required);
    if (!emp_ok) mandatory_fails++;
    else score += 15;

    /* Loan amount ≤ PHP 2,000,000 */
    snprintf(actual,   sizeof(actual),   "PHP %.2f", a->loan_amount);
    snprintf(required, sizeof(required), "≤ PHP 2,000,000");
    int amt_ok = (a->loan_amount > 0 && a->loan_amount <= 2000000.0);
    add_criterion(r, "loan_amount_limit", "Car loan maximum is PHP 2,000,000",
                  amt_ok, actual, required);
    if (!amt_ok) mandatory_fails++;
    else score += 10;

    /* Debt-to-income: estimated monthly payment ≤ 40% of income */
    double est_payment = estimated_monthly_payment(a->loan_amount);
    double dti         = (a->monthly_income > 0)
                         ? (est_payment / a->monthly_income) * 100.0
                         : 999.0;
    snprintf(actual,   sizeof(actual),   "%.1f%% DTI", dti);
    snprintf(required, sizeof(required), "≤ 40%% DTI");
    int dti_ok = (dti <= 40.0);
    add_criterion(r, "debt_to_income", "Estimated monthly payment ≤ 40% of income",
                  dti_ok, actual, required);
    if (dti_ok) score += 15;
    else        score -= 10;

    /* Bonus: high credit score */
    if (a->credit_score >= 750) score += 5;

    /* Cap score */
    if (score > 100) score = 100;
    if (score < 0)   score = 0;

    r->score = score;
    r->eligible = (mandatory_fails == 0 && score >= 60);
    strncpy(r->verdict, r->eligible ? "ELIGIBLE" : "DENIED", sizeof(r->verdict) - 1);

    if (r->eligible) {
        snprintf(r->message, sizeof(r->message),
            "Congratulations, %s! You are eligible for a Car Loan of PHP %.2f. "
            "Your application scored %d/100.",
            a->full_name, a->loan_amount, score);
        strncpy(r->next_step,
            "Proceed to the nearest SkyeBank branch with a valid ID, proof of income, "
            "and vehicle details to complete your application.",
            sizeof(r->next_step) - 1);
    } else {
        snprintf(r->message, sizeof(r->message),
            "We're sorry, %s. Your Car Loan application has been denied. "
            "Application score: %d/100. Please review the criteria below.",
            a->full_name, score);
        strncpy(r->next_step,
            "You may reapply once you have improved your credit score, "
            "increased your income, or reduced existing loan obligations.",
            sizeof(r->next_step) - 1);
    }
}

/* ------------------------------------------------------------------ */
/* HOUSING LOAN eligibility                                             */
/* ------------------------------------------------------------------ */
static void check_housing_loan(const Applicant *a, EligibilityResult *r) {
    int score = 0;
    int mandatory_fails = check_common_criteria(a, r, &score);

    char actual[64], required[64];

    /* Credit score ≥ 700 */
    snprintf(actual,   sizeof(actual),   "%d", a->credit_score);
    snprintf(required, sizeof(required), "≥ 700");
    int cs_ok = (a->credit_score >= 700);
    add_criterion(r, "credit_score", "Credit score must be 700 or higher for housing loans",
                  cs_ok, actual, required);
    if (!cs_ok) mandatory_fails++;
    else score += 20;

    /* Monthly income ≥ PHP 50,000 */
    snprintf(actual,   sizeof(actual),   "PHP %.2f", a->monthly_income);
    snprintf(required, sizeof(required), "PHP 50,000");
    int inc_ok = (a->monthly_income >= 50000.0);
    add_criterion(r, "minimum_income", "Monthly income must be at least PHP 50,000",
                  inc_ok, actual, required);
    if (!inc_ok) mandatory_fails++;
    else score += 20;

    /* Years employed ≥ 3 */
    snprintf(actual,   sizeof(actual),   "%.1f years", a->years_employed);
    snprintf(required, sizeof(required), "≥ 3 years");
    int emp_ok = (a->years_employed >= 3.0);
    add_criterion(r, "employment_duration", "Must be employed for at least 3 years",
                  emp_ok, actual, required);
    if (!emp_ok) mandatory_fails++;
    else score += 15;

    /* Loan amount ≤ PHP 10,000,000 */
    snprintf(actual,   sizeof(actual),   "PHP %.2f", a->loan_amount);
    snprintf(required, sizeof(required), "≤ PHP 10,000,000");
    int amt_ok = (a->loan_amount > 0 && a->loan_amount <= 10000000.0);
    add_criterion(r, "loan_amount_limit", "Housing loan maximum is PHP 10,000,000",
                  amt_ok, actual, required);
    if (!amt_ok) mandatory_fails++;
    else score += 10;

    /* Collateral ≥ 80% of loan amount */
    double collateral_ratio = (a->loan_amount > 0)
                              ? (a->collateral_value / a->loan_amount) * 100.0
                              : 0.0;
    snprintf(actual,   sizeof(actual),   "PHP %.2f (%.1f%%)", a->collateral_value, collateral_ratio);
    snprintf(required, sizeof(required), "≥ 80%% of loan amount");
    int col_ok = (collateral_ratio >= 80.0);
    add_criterion(r, "collateral_coverage", "Property value must cover at least 80% of the loan",
                  col_ok, actual, required);
    if (!col_ok) mandatory_fails++;
    else score += 15;

    /* Debt-to-income: housing uses 20-year amortisation estimate */
    double est_payment = a->loan_amount / 240.0; /* 20yr * 12mo */
    double dti = (a->monthly_income > 0)
                 ? (est_payment / a->monthly_income) * 100.0
                 : 999.0;
    snprintf(actual,   sizeof(actual),   "%.1f%% DTI", dti);
    snprintf(required, sizeof(required), "≤ 35%% DTI");
    int dti_ok = (dti <= 35.0);
    add_criterion(r, "debt_to_income",
                  "Estimated monthly amortisation ≤ 35% of income",
                  dti_ok, actual, required);
    if (dti_ok) score += 10;
    else        score -= 15;

    /* Bonus: very high credit score */
    if (a->credit_score >= 780) score += 5;

    if (score > 100) score = 100;
    if (score < 0)   score = 0;

    r->score = score;
    r->eligible = (mandatory_fails == 0 && score >= 60);
    strncpy(r->verdict, r->eligible ? "ELIGIBLE" : "DENIED", sizeof(r->verdict) - 1);

    if (r->eligible) {
        snprintf(r->message, sizeof(r->message),
            "Congratulations, %s! You qualify for a Housing Loan of PHP %.2f. "
            "Score: %d/100.",
            a->full_name, a->loan_amount, score);
        strncpy(r->next_step,
            "Visit a SkyeBank branch with your valid ID, latest payslips (3 months), "
            "property documents, and collateral appraisal to begin formal processing.",
            sizeof(r->next_step) - 1);
    } else {
        snprintf(r->message, sizeof(r->message),
            "Sorry, %s. Your Housing Loan application was denied. "
            "Score: %d/100. Check the criteria details below.",
            a->full_name, score);
        strncpy(r->next_step,
            "Consider improving your credit score, increasing your down payment, "
            "or providing additional collateral before reapplying.",
            sizeof(r->next_step) - 1);
    }
}

/* ------------------------------------------------------------------ */
/* BUSINESS LOAN eligibility                                            */
/* ------------------------------------------------------------------ */
static void check_business_loan(const Applicant *a, EligibilityResult *r) {
    int score = 0;
    int mandatory_fails = check_common_criteria(a, r, &score);

    char actual[64], required[64];

    /* Credit score ≥ 680 */
    snprintf(actual,   sizeof(actual),   "%d", a->credit_score);
    snprintf(required, sizeof(required), "≥ 680");
    int cs_ok = (a->credit_score >= 680);
    add_criterion(r, "credit_score", "Credit score must be 680 or higher for business loans",
                  cs_ok, actual, required);
    if (!cs_ok) mandatory_fails++;
    else score += 20;

    /* Monthly revenue ≥ PHP 80,000 */
    snprintf(actual,   sizeof(actual),   "PHP %.2f", a->monthly_income);
    snprintf(required, sizeof(required), "PHP 80,000");
    int inc_ok = (a->monthly_income >= 80000.0);
    add_criterion(r, "minimum_revenue", "Monthly business revenue must be at least PHP 80,000",
                  inc_ok, actual, required);
    if (!inc_ok) mandatory_fails++;
    else score += 20;

    /* Business operating ≥ 2 years */
    snprintf(actual,   sizeof(actual),   "%.1f years", a->business_years);
    snprintf(required, sizeof(required), "≥ 2 years");
    int biz_ok = (a->business_years >= 2.0);
    add_criterion(r, "business_age", "Business must have been operating for at least 2 years",
                  biz_ok, actual, required);
    if (!biz_ok) mandatory_fails++;
    else score += 15;

    /* Loan amount ≤ PHP 5,000,000 */
    snprintf(actual,   sizeof(actual),   "PHP %.2f", a->loan_amount);
    snprintf(required, sizeof(required), "≤ PHP 5,000,000");
    int amt_ok = (a->loan_amount > 0 && a->loan_amount <= 5000000.0);
    add_criterion(r, "loan_amount_limit", "Business loan maximum is PHP 5,000,000",
                  amt_ok, actual, required);
    if (!amt_ok) mandatory_fails++;
    else score += 10;

    /* Loan ≤ 3× annual revenue */
    double annual_rev   = a->monthly_income * 12.0;
    double revenue_ratio = (annual_rev > 0) ? a->loan_amount / annual_rev : 999.0;
    snprintf(actual,   sizeof(actual),   "Loan is %.1f× annual revenue", revenue_ratio);
    snprintf(required, sizeof(required), "≤ 3× annual revenue");
    int rev_ok = (revenue_ratio <= 3.0);
    add_criterion(r, "loan_to_revenue", "Loan must not exceed 3× annual revenue",
                  rev_ok, actual, required);
    if (!rev_ok) mandatory_fails++;
    else score += 15;

    /* Employment type bonus: self-employed shows business ownership */
    int is_self = (strcasecmp(a->employment_type, "self-employed") == 0);
    add_criterion(r, "employment_type",
                  "Self-employed applicants get positive consideration",
                  is_self,
                  a->employment_type,
                  "self-employed preferred");
    if (is_self) score += 5;

    /* Bonus: long business track record */
    if (a->business_years >= 5.0) score += 5;

    if (score > 100) score = 100;
    if (score < 0)   score = 0;

    r->score = score;
    r->eligible = (mandatory_fails == 0 && score >= 60);
    strncpy(r->verdict, r->eligible ? "ELIGIBLE" : "DENIED", sizeof(r->verdict) - 1);

    if (r->eligible) {
        snprintf(r->message, sizeof(r->message),
            "Congratulations, %s! Your Business Loan of PHP %.2f is approved for review. "
            "Score: %d/100.",
            a->full_name, a->loan_amount, score);
        strncpy(r->next_step,
            "Bring your DTI certificate, 2 years of audited financial statements, "
            "business registration, and valid ID to the nearest SkyeBank branch.",
            sizeof(r->next_step) - 1);
    } else {
        snprintf(r->message, sizeof(r->message),
            "We regret to inform you, %s, that your Business Loan application "
            "was denied. Score: %d/100.",
            a->full_name, score);
        strncpy(r->next_step,
            "Work on building your credit score, demonstrating consistent revenue growth, "
            "and ensuring your business has been operating for at least 2 years.",
            sizeof(r->next_step) - 1);
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                            */
/* ------------------------------------------------------------------ */

void check_eligibility(const Applicant *a, EligibilityResult *r) {
    memset(r, 0, sizeof(*r));

    switch (a->loan_type) {
        case LOAN_CAR:
            check_car_loan(a, r);
            break;
        case LOAN_HOUSING:
            check_housing_loan(a, r);
            break;
        case LOAN_BUSINESS:
            check_business_loan(a, r);
            break;
        default:
            r->eligible = 0;
            strncpy(r->verdict,  "DENIED",            sizeof(r->verdict) - 1);
            strncpy(r->message,  "Unknown loan type.", sizeof(r->message) - 1);
            strncpy(r->next_step,"Specify car, housing, or business.", sizeof(r->next_step)-1);
            break;
    }
}

/* ------------------------------------------------------------------ */
/* JSON serialisation                                                    */
/* ------------------------------------------------------------------ */

char *eligibility_result_to_json(const EligibilityResult *r) {
    /* Build criteria array first */
    char criteria_buf[4096] = "[";
    for (int i = 0; i < r->criteria_count; i++) {
        const Criterion *c = &r->criteria[i];
        char item[512];
        snprintf(item, sizeof(item),
            "%s{"
              "\"name\":\"%s\","
              "\"description\":\"%s\","
              "\"passed\":%s,"
              "\"actual\":\"%s\","
              "\"required\":\"%s\""
            "}",
            i > 0 ? "," : "",
            c->name, c->description,
            c->passed ? "true" : "false",
            c->actual_value, c->required_value
        );
        strncat(criteria_buf, item, sizeof(criteria_buf) - strlen(criteria_buf) - 1);
    }
    strncat(criteria_buf, "]", sizeof(criteria_buf) - strlen(criteria_buf) - 1);

    /* Allocate final JSON buffer */
    size_t total = strlen(criteria_buf) + 1024;
    char  *json  = malloc(total);
    if (!json) return NULL;

    snprintf(json, total,
        "{"
          "\"eligible\":%s,"
          "\"verdict\":\"%s\","
          "\"score\":%d,"
          "\"message\":\"%s\","
          "\"next_step\":\"%s\","
          "\"criteria\":%s"
        "}",
        r->eligible ? "true" : "false",
        r->verdict,
        r->score,
        r->message,
        r->next_step,
        criteria_buf
    );
    return json;
}
