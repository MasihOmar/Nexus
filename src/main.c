#include "globals.h"
#include <signal.h>

// Signal handler for graceful shutdown
volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        running = 0;
        printf("\n" RENK_SARI "Received shutdown signal, cleaning up...\n" RENK_SIFIRLA);
    }
}

int main() {
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    sfs_yukle();     
    memory_init(); // Initialize memory simulation
    
    printf(RENK_CYAN "NexOS v%s (Built: %s %s)\n" RENK_SIFIRLA, 
           NEXOS_VERSION, NEXOS_BUILD_DATE, NEXOS_BUILD_TIME);
    printf("System starting...\n");
    sleep(1);
    
    // Show login screen - must authenticate before menu
    if(!show_login_screen()) {
        printf(RENK_KIRMIZI "Access denied. Shutting down.\n" RENK_SIFIRLA);
        return 1;
    }
    
    tui_menu(); 
    
    sfs_kaydet(); 
    return 0;
}
