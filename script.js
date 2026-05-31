// Smooth Fade Animation logic
// Added '.loan-card' so your new services page cards animate too!
const cards = document.querySelectorAll(".feature-card, .hero-card, .loan-card");

const observer = new IntersectionObserver(entries => {
    entries.forEach(entry => {
        if(entry.isIntersecting) {
            entry.target.style.opacity = "1";
            entry.target.style.transform = "translateY(0)";
        }
    });
});

cards.forEach(card => {
    // Initial state before scrolling into view
    card.style.opacity = "0";
    card.style.transform = "translateY(30px)";
    card.style.transition = "all .7s ease";
    
    // Tell the observer to watch this card
    observer.observe(card);
});