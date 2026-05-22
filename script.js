const button = document.getElementById("checkBtn");

button.addEventListener("click", () => {

    alert(
        "Welcome to SkyeBank!\n\nProceed to Loan Eligibility Checker."
    );

});

/* Smooth Fade Animation */

const cards = document.querySelectorAll(
".feature-card, .hero-card"
);

const observer = new IntersectionObserver(entries => {

    entries.forEach(entry => {

        if(entry.isIntersecting){

            entry.target.style.opacity = "1";
            entry.target.style.transform = "translateY(0)";

        }

    });

});

cards.forEach(card => {

    card.style.opacity = "0";
    card.style.transform = "translateY(30px)";
    card.style.transition = "all .7s ease";

    observer.observe(card);

});