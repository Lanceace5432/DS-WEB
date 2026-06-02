// ==========================================================================
// Smooth Fade Animation Logic
// ==========================================================================

// Target all elements you want to animate on scroll
const cards = document.querySelectorAll(".loan-card");

// Create the Intersection Observer
const observer = new IntersectionObserver(entries => {
    entries.forEach(entry => {
        if(entry.isIntersecting) {
            // When the card enters the screen, fade it in and slide it up
            entry.target.style.opacity = "1";
            entry.target.style.transform = "translateY(0)";
            
            // Optional: Stop observing once it has animated so it doesn't repeat
            // observer.unobserve(entry.target); 
        }
    });
}, {
    // Triggers when 10% of the element is visible
    threshold: 0.1 
});

// Set initial invisible state for all cards and start observing
cards.forEach(card => {
    // Initial state before scrolling into view
    card.style.opacity = "0";
    card.style.transform = "translateY(30px)"; // Start 30px lower
    card.style.transition = "opacity 0.7s ease, transform 0.7s ease";
    
    // Tell the observer to watch this card
    observer.observe(card);
});