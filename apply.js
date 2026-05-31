// Grab parameter arguments out of the address path (e.g. apply.html?type=housing)
const urlParams = new URLSearchParams(window.location.search);
const loanType = urlParams.get('type') || 'car'; // Fallback to car defaults if blank

// Element selection points
const reqTitle = document.getElementById('req-title');
const reqIcon = document.getElementById('req-icon');
const reqIncome = document.getElementById('req-income');
const reqDocs = document.getElementById('req-docs');

// Content criteria data mapping object rulesets
const loanRules = {
    car: {
        title: "Car Loan Requirements",
        iconClass: "fa-car",
        income: "₱25,000 or above",
        docs: "ID, Proof of Income, Employment Letter"
    },
    housing: {
        title: "Housing Loan Requirements",
        iconClass: "fa-house-chimney",
        income: "$5,500 or above",
        docs: "ID, Certified True Copy of Title, Tax Declaration, Proof of Income"
    },
    business: {
        title: "Business Loan Requirements",
        iconClass: "fa-briefcase",
        income: "$10,000 or above",
        docs: "DTI/SEC Registration, 3 Years Financial Statements, Mayor's Permit"
    }
};

// Inject parameters if data entry point mapping configuration matches
if (loanRules[loanType]) {
    const selectedLoan = loanRules[loanType];
    
    reqTitle.innerText = selectedLoan.title;
    reqIncome.innerText = selectedLoan.income;
    reqDocs.innerText = selectedLoan.docs;
    
    // Clear old icon descriptors and refresh class string structures
    reqIcon.className = `fa-solid ${selectedLoan.iconClass}`;
}

// Redirect action pointer step handler
document.getElementById('continueBtn').addEventListener('click', () => {
    // When they click continue, pass the loan type over to your checker page/form block view
    alert(`Proceeding to the ${loanType} eligibility assessment form...`);
    // Example: window.location.href = `checker.html?type=${loanType}`;
});