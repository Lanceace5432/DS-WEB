// ==========================================================================
// Global State & Configurations
// ==========================================================================

const LOAN_CONFIG = {
    Housing: { interestRate: 0.15, maxDTI: 30, maxMonths: 120, maxLoan: 10000000, docs: ['Government-Issued ID', 'Proof of Income', 'Proof of Residency', 'Collateral House Title', 'House Insurance'] },
    Car: { interestRate: 0.12, maxDTI: 40, maxMonths: 60, maxLoan: 5000000, docs: ['Government-Issued ID', 'Proof of Income', 'Proof of Residency', 'Vehicle Information', 'Proof of Insurance'] },
    Business: { interestRate: 0.18, maxDTI: 50, maxMonths: 60, maxLoan: 15000000, docs: ['Government-Issued ID', 'Proof of Income', 'Proof of Residency', 'Business Location', 'Business Plan'] }
};

// Object to store client assessment data temporarily
const appState = {
    fullName: '',
    age: 0,
    employment: '',
    creditScore: 0,
    monthlyIncome: 0,
    loanType: 'Housing', // LOCKED TO HOUSING LOAN
    loanAmount: 0,
    monthsToPay: 0,
    totalLoan: 0,
    monthlyPayment: 0,
    dti: 0
};

// Target and Cache Essential DOM Nodes for Progress
const stepIndicator = document.getElementById('step-indicator');
const progressFill = document.getElementById('progress-fill');
const progressPercent = document.getElementById('progress-percent');

// ==========================================================================
// Layout Render Engine & Navigation
// ==========================================================================

function nextStep(currentStep) {
    if (!validateStepInputs(currentStep)) return;

    if (currentStep === 3) {
        populateStep4Summary();
    }

    document.getElementById(`step${currentStep}`).classList.remove('active');
    document.getElementById(`step${currentStep + 1}`).classList.add('active');
    updateProgressTrack(currentStep + 1);
}

function prevStep(currentStep) {
    document.getElementById(`step${currentStep}`).classList.remove('active');
    document.getElementById(`step${currentStep - 1}`).classList.add('active');
    updateProgressTrack(currentStep - 1);
}

function updateProgressTrack(step) {
    const percentages = { 1: 25, 2: 50, 3: 75, 4: 100 };
    stepIndicator.innerText = `Step ${step} of 4`;
    progressPercent.innerText = `${percentages[step]}% Complete`;
    progressFill.style.width = `${percentages[step]}%`;
}

// ==========================================================================
// Input Validation Routine
// ==========================================================================

function validateStepInputs(step) {
    if (step === 1) {
        appState.fullName = document.getElementById('fullName').value.trim();
        appState.age = parseInt(document.getElementById('age').value, 10);
        appState.employment = document.getElementById('employment').value;
        appState.creditScore = parseInt(document.getElementById('creditScore').value, 10);

        if (!appState.fullName) { alert("Please enter your full name."); return false; }
        if (!appState.age || appState.age <= 0) { alert("Please input a valid age."); return false; }
        if (!appState.employment) { alert("Please select your current employment status."); return false; }
        if (!appState.creditScore || appState.creditScore < 300 || appState.creditScore > 850) { 
            alert("Please enter a realistic credit score parameter between 300 and 850."); return false; 
        }
    } 
    else if (step === 2) {
        appState.monthlyIncome = parseFloat(document.getElementById('monthlyIncome').value);
        if (isNaN(appState.monthlyIncome) || appState.monthlyIncome <= 0) { 
            alert("Please enter a valid monthly income before proceeding."); return false; 
        }
    } 
    else if (step === 3) {
        appState.loanAmount = parseFloat(document.getElementById('loanAmount').value);
        appState.monthsToPay = parseInt(document.getElementById('monthsToPay').value, 10);
        
        if (isNaN(appState.loanAmount) || appState.loanAmount <= 0) { 
            alert("Please enter a valid loan amount."); return false; 
        }
        if (isNaN(appState.monthsToPay) || appState.monthsToPay <= 0) { 
            alert("Please enter valid months to pay."); return false; 
        }
    }
    
    return true;
}

// ==========================================================================
// Dynamic Math Calculations & Utilities
// ==========================================================================

function formatCurrency(amount) {
    return '₱' + amount.toLocaleString('en-PH', { minimumFractionDigits: 2, maximumFractionDigits: 2 });
}

function calculateLoan() {
    const amountInput = parseFloat(document.getElementById('loanAmount').value) || 0;
    const monthsInput = parseInt(document.getElementById('monthsToPay').value, 10) || 0;

    appState.loanAmount = amountInput;
    appState.monthsToPay = monthsInput;

    const rate = LOAN_CONFIG[appState.loanType].interestRate;
    const interestAmount = appState.loanAmount * rate;
    
    appState.totalLoan = appState.loanAmount + interestAmount;
    appState.monthlyPayment = (appState.monthsToPay > 0) ? (appState.totalLoan / appState.monthsToPay) : 0;

    document.getElementById('sumLoanAmount').innerText = formatCurrency(appState.loanAmount);
    document.getElementById('sumInterestAmount').innerText = formatCurrency(interestAmount);
    document.getElementById('sumTotalLoan').innerText = formatCurrency(appState.totalLoan);
    document.getElementById('sumMonthlyPayment').innerText = formatCurrency(appState.monthlyPayment);
}

function populateStep4Summary() {
    const docs = LOAN_CONFIG[appState.loanType].docs;
    
    const checklistHTML = docs.map((doc, index) => `
        <label class="checklist-item">
            <input type="checkbox" class="doc-checkbox" value="${doc}">
            <div class="check-box"></div>
            <span>${doc}</span>
        </label>
    `).join('');
    document.getElementById('documentChecklist').innerHTML = checklistHTML;

    document.getElementById('appFullName').innerText = appState.fullName;
    document.getElementById('appAge').innerText = appState.age;
    document.getElementById('appEmployment').innerText = appState.employment;
    document.getElementById('appCreditScore').innerText = appState.creditScore;
    document.getElementById('appIncome').innerText = formatCurrency(appState.monthlyIncome);
    document.getElementById('appLoan').innerText = formatCurrency(appState.loanAmount);
    document.getElementById('appMonthly').innerText = formatCurrency(appState.monthlyPayment);
}

// ==========================================================================
// CORE PROCESSING RULE ENGINE
// ==========================================================================

function processAssessment() {
    appState.dti = (appState.monthlyPayment / appState.monthlyIncome) * 100;
    const config = LOAN_CONFIG[appState.loanType];

    document.getElementById('progress-header').style.display = 'none';
    document.getElementById('step4').classList.remove('active');
    
    const loadingScreen = document.getElementById('loadingScreen');
    loadingScreen.classList.add('show'); 

    let reasons = [];
    
    if (appState.age < 21 || appState.age > 65) reasons.push("Your age must be between 21 and 65 years old.");
    if (appState.creditScore < 650) reasons.push(`Credit evaluation (${appState.creditScore}) is lower than the required minimum of 650.`);
    if (appState.employment === 'Unemployed') reasons.push("Active professional status required (Must not be unemployed).");
    
    if (appState.loanAmount > config.maxLoan) {
        reasons.push(`The requested loan amount (${formatCurrency(appState.loanAmount)}) exceeds the maximum allowed limit of ${formatCurrency(config.maxLoan)} for a ${appState.loanType} Loan.`);
    }
    if (appState.monthsToPay > config.maxMonths) {
        reasons.push(`The requested payment duration (${appState.monthsToPay} months) exceeds the maximum allowed limit of ${config.maxMonths} months for a ${appState.loanType} Loan.`);
    }

    if (appState.dti > config.maxDTI) reasons.push(`Debt-to-Income ratio (${appState.dti.toFixed(1)}%) exceeds the maximum allowed limit (${config.maxDTI}%).`);
    
    const allDocsChecked = Array.from(document.querySelectorAll('.doc-checkbox')).every(cb => cb.checked);
    if (!allDocsChecked) {
        reasons.push("You must have all required documents ready to be eligible for a loan.");
    }

    const steps = document.querySelectorAll('#loadingStepsList li');
    const dots = document.querySelectorAll('.loading-dots .dot');
    
    setTimeout(() => {
        steps[0].classList.remove('active');
        dots[0].classList.remove('active');
        steps[1].classList.add('active');
        dots[1].classList.add('active');
    }, 1500);

    setTimeout(() => {
        steps[1].classList.remove('active');
        dots[1].classList.remove('active');
        steps[2].classList.add('active');
        dots[2].classList.add('active');
    }, 3000);

    setTimeout(() => {
        loadingScreen.classList.remove('show'); 
        
        setTimeout(() => {
            loadingScreen.style.display = 'none'; 
            renderResults(reasons.length === 0, reasons, config);
        }, 500);
        
    }, 4500);
}

// ==========================================================================
// Results Generator
// ==========================================================================

function renderResults(isApproved, reasons, config) {
    const resultsScreen = document.getElementById('resultsScreen');
    
    resultsScreen.style.display = 'block';
    resultsScreen.className = "card results-screen active";

    if (isApproved) {
        resultsScreen.innerHTML = `
            <div class="result-header">
                <div class="result-icon success">
                    <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"></polyline></svg>
                </div>
                <h1>✓ Eligible for Loan</h1>
                <p>Congratulations! You meet all requirements for a ${appState.loanType} Loan.</p>
            </div>
            
            <h2 style="font-size: 1.1rem; color: var(--navy); margin-bottom: 1rem;">Loan Details</h2>
            
            <div class="result-grid">
                <div class="result-item"><span>Loan Type</span><span>${appState.loanType} Loan</span></div>
                <div class="result-item"><span>Interest Rate</span><span>${config.interestRate * 100}%</span></div>
                <div class="result-item"><span>Monthly Income</span><span>${formatCurrency(appState.monthlyIncome)}</span></div>
                <div class="result-item"><span>Total Loan Amount</span><span class="blue-text">${formatCurrency(appState.totalLoan)}</span></div>
                <div class="result-item"><span>Loan Amount</span><span>${formatCurrency(appState.loanAmount)}</span></div>
                <div class="result-item"><span>Monthly Payment</span><span class="blue-text">${formatCurrency(appState.monthlyPayment)}</span></div>
            </div>

            <div class="dti-indicator">
                <div class="dti-header">
                    <span>Debt-to-Income Ratio</span>
                    <span class="dti-val">${appState.dti.toFixed(1)}%</span>
                </div>
                <div class="dti-bar-bg">
                    <div class="dti-bar-fill" style="width: ${appState.dti}%"></div>
                </div>
                <div class="dti-max">Maximum allowed: ${config.maxDTI}%</div>
            </div>

            <div class="button-group" style="margin-top: 2rem; justify-content: center;">
                <button class="btn btn-primary" onclick="window.location.href='services.html'" style="max-width: 300px;">Check Another Loan</button>
            </div>
        `;
    } else {
        const listHTML = reasons.map(r => `<li>${r}</li>`).join('');
        
        resultsScreen.innerHTML = `
            <div class="result-header">
                <div class="result-icon error">
                    <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="6" x2="6" y2="18"></line><line x1="6" y1="6" x2="18" y2="18"></line></svg>
                </div>
                <h1>✖ Not Eligible</h1>
                <p>Unfortunately, your application does not fully align with our minimal thresholds.</p>
            </div>
            
            <h2 style="font-size: 1.1rem; color: var(--navy); margin-bottom: 1rem;">Reason(s) for Rejection:</h2>
            <ul class="rejection-list">
                ${listHTML}
            </ul>

            <div class="button-group" style="margin-top: 2rem; justify-content: center;">
                <button class="btn btn-primary" onclick="window.location.href='services.html'" style="max-width: 300px;">Check Another Loan</button>
            </div>
        `;
    }

    setTimeout(() => {
        resultsScreen.classList.add('show');
    }, 50);
}