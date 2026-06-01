// Global State Management Trackers
let currentStep = 1;
const totalSteps = 4;

// Target and Cache Essential DOM Nodes
const stepTrackerText = document.getElementById('step-tracker-text');
const progressBarFill = document.getElementById('progress-bar-fill');
const progressPercentText = document.getElementById('progress-percent-text');
const prevBtn = document.getElementById('prev-btn');
const nextBtn = document.getElementById('next-btn');
const navControls = document.getElementById('wizard-navigation-controls');

// Inputs
const incomeInput = document.getElementById('income-input');
const creditInput = document.getElementById('credit-input');
const employmentInput = document.getElementById('employment-input');
const ageInput = document.getElementById('age-input');

// Results
const resultCard = document.getElementById('step-result');
const resultTitle = document.getElementById('result-title');
const resultMessage = document.getElementById('result-message');

// Object to store client assessment data in local memory temporarily
const clientData = {
    income: 0,
    creditScore: 0,
    employmentStatus: '', // Changed from months to track dropdown strings
    age: 0
};

// Main Navigation Event Handlers
nextBtn.addEventListener('click', () => {
    if (validateStepInputs()) {
        if (currentStep < totalSteps) {
            currentStep++;
            renderStepTransitions();
        } else {
            runEligibilityMath();
        }
    }
});

prevBtn.addEventListener('click', () => {
    if (currentStep > 1) {
        currentStep--;
        renderStepTransitions();
    } else {
        window.location.href = 'apply.html';
    }
});

// Input Validation Routine to guard form steps
function validateStepInputs() {
    if (currentStep === 1) {
        const value = parseFloat(incomeInput.value);
        if (isNaN(value) || value <= 0) {
            alert("Please enter a valid monthly income before proceeding.");
            incomeInput.focus();
            return false;
        }
        clientData.income = value;
    } 
    
    else if (currentStep === 2) {
        const value = parseInt(creditInput.value, 10);
        if (isNaN(value) || value < 300 || value > 850) {
            alert("Please enter a realistic credit score parameter between 300 and 850.");
            creditInput.focus();
            return false;
        }
        clientData.creditScore = value;
    } 
    
    else if (currentStep === 3) {
        const value = employmentInput.value; // Reads 'unemployed', 'full-time', etc.
        if (!value) {
            alert("Please select your current employment status setup from the choices.");
            employmentInput.focus();
            return false;
        }
        clientData.employmentStatus = value;
    } 
    
    else if (currentStep === 4) {
        const value = parseInt(ageInput.value, 10);
        if (isNaN(value) || value <= 0) {
            alert("Please input a standard age coordinate integer.");
            ageInput.focus();
            return false;
        }
        clientData.age = value;
    }

    return true;
}

// Layout Render Engine updates visual progress tracks and swaps step cards
function renderStepTransitions() {
    // Hide all operational step container components
    document.querySelectorAll('.step-card').forEach(card => card.classList.add('hidden'));
    
    // Unveil the specific active single card container targets
    document.getElementById(`step-${currentStep}`).classList.remove('hidden');

    // UI Tracker progress fill bar elements
    const progressPercentage = (currentStep / totalSteps) * 100;
    progressBarFill.style.width = `${progressPercentage}%`;
    progressPercentText.innerText = `${Math.round(progressPercentage)}% Complete`;
    stepTrackerText.innerText = `Step ${currentStep} of ${totalSteps}`;

    // Update button display states dynamically
    if (currentStep === totalSteps) {
        nextBtn.innerText = "Submit Assessment";
    } else {
        nextBtn.innerText = "Next";
    }
}

// CORE PROCESSING RULE ENGINE
function runEligibilityMath() {
    // Hide operational control elements to terminate workflow transitions cleanly
    document.querySelectorAll('.step-card').forEach(card => card.classList.add('hidden'));
    if (navControls) navControls.style.display = 'none';
    
    // Display dynamic placeholder state output panel
    resultCard.classList.remove('hidden');

    // Updated institutional loan requirements parameters
    const targetIncomeMinimum = 25000; // Fixed to ₱25,000
    const targetCreditMinimum = 650;
    const targetAgeFloor = 21;
    const targetAgeCeiling = 65;

    // Evaluate parameters systematically against baseline requirements
    let passesIncome = clientData.income >= targetIncomeMinimum;
    let passesCredit = clientData.creditScore >= targetCreditMinimum;
    let passesEmployment = clientData.employmentStatus !== 'unemployed'; // Fails instantly if Unemployed
    let passesAge = clientData.age >= targetAgeFloor && clientData.age <= targetAgeCeiling;

    // Check if user flags passed all logical parameters smoothly
    if (passesIncome && passesCredit && passesEmployment && passesAge) {
        resultTitle.innerText = "🎉 Congratulations! You are Eligible";
        resultTitle.style.color = "#10b981"; // Clean green indicator
        resultMessage.innerHTML = `Based on your profile inputs, your configuration status meets SkyeBank's institutional loan criteria perfectly.<br><br><strong>Income Tracked:</strong> ₱${clientData.income.toLocaleString()}<br><strong>Credit Score:</strong> ${clientData.creditScore}`;
    } else {
        resultTitle.innerText = "Review Status: Ineligible At This Time";
        resultTitle.style.color = "#ef4444"; // Soft red indicator
        
        // Pinpoint precise blocking failures to optimize user feedback quality
        let reasonString = "Your application does not fully align with our minimal thresholds due to:<br><ul style='text-align: left; display: inline-block; margin-top: 10px; padding-left: 20px;'>";
        if (!passesIncome) reasonString += `<li>Monthly income falls below the minimum requirement of ₱${targetIncomeMinimum.toLocaleString()}.</li>`;
        if (!passesCredit) reasonString += `<li>Credit evaluation is lower than the required minimum of ${targetCreditMinimum}.</li>`;
        if (!passesEmployment) reasonString += `<li>Active professional status required (Must not be unemployed).</li>`;
        if (!passesAge) reasonString += `<li>Your age profile does not align with our regulatory bracket constraint window of ${targetAgeFloor}-${targetAgeCeiling} years old.</li>`;
        reasonString += "</ul>";
        
        resultMessage.innerHTML = reasonString;
    }
}

// Start tracking on layout load
document.addEventListener('DOMContentLoaded', () => {
    renderStepTransitions();
});