#include "globals.h"

// Login screen - returns true if successful
bool show_login_screen() {
    char username[32] = "";
    char password[32] = "";
    int attempts = 0;
    
    struct termios orig, raw;
    tcgetattr(STDIN_FILENO, &orig);
    
    while(attempts < 3) {
        printf("\033[H\033[J"); // Clear screen
        
        printf(RENK_CYAN);
        printf("\n\n");
        printf("       +------------------------------------------+\n");
        printf("       |                                          |\n");
        printf("       |" RENK_SIFIRLA "            N E X O S                    " RENK_CYAN "|\n");
        printf("       |" RENK_SIFIRLA "       Next Generation OS                " RENK_CYAN "|\n");
        printf("       |                                          |\n");
        printf("       +------------------------------------------+\n");
        printf("       |" RENK_SIFIRLA "              LOGIN                      " RENK_CYAN "|\n");
        printf("       +------------------------------------------+\n");
        printf(RENK_SIFIRLA);
        
        if(attempts > 0) {
            printf(RENK_KIRMIZI "\n       Invalid credentials! Try again.\n" RENK_SIFIRLA);
        }
        
        printf("\n       Username: ");
        fflush(stdout);
        
        // Read username (normal mode)
        if(fgets(username, sizeof(username), stdin) == NULL) return false;
        username[strcspn(username, "\n")] = '\0';
        
        printf("       Password: ");
        fflush(stdout);
        
        // Hide password input
        raw = orig;
        raw.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        
        if(fgets(password, sizeof(password), stdin) == NULL) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
            return false;
        }
        password[strcspn(password, "\n")] = '\0';
        
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
        printf("\n");
        
        // Check credentials
        if(strcmp(username, "admin") == 0 && strcmp(password, "123") == 0) {
            printf(RENK_YESIL "\n       Login successful! Welcome, admin.\n" RENK_SIFIRLA);
            sleep(1);
            return true;
        }
        
        attempts++;
    }
    
    printf(RENK_KIRMIZI "\n       Too many failed attempts. Access denied.\n" RENK_SIFIRLA);
    sleep(2);
    return false;
}

void tui_draw_box(int selected) {
    printf("\033[H\033[J"); // Clear Screen
    
    // Get basic system info for status bar
    struct sysinfo info;
    sysinfo(&info);
    long ram_used = (info.totalram - info.freeram) / (1024 * 1024);
    long ram_total = info.totalram / (1024 * 1024);
    
    // NexOS Block-Style ASCII Art Logo
    printf(RENK_CYAN);
    printf("  ╔══════════════════════════════════════════════════════════════╗\n");
    printf("  ║                                                              ║\n");
    printf("  ║  " RENK_YESIL "███╗   ██╗███████╗██╗  ██╗ ██████╗ ███████╗" RENK_CYAN "                 ║\n");
    printf("  ║  " RENK_YESIL "████╗  ██║██╔════╝╚██╗██╔╝██╔═══██╗██╔════╝" RENK_CYAN "                 ║\n");
    printf("  ║  " RENK_YESIL "██╔██╗ ██║█████╗   ╚███╔╝ ██║   ██║███████╗" RENK_CYAN "                 ║\n");
    printf("  ║  " RENK_YESIL "██║╚██╗██║██╔══╝   ██╔██╗ ██║   ██║╚════██║" RENK_CYAN "                 ║\n");
    printf("  ║  " RENK_YESIL "██║ ╚████║███████╗██╔╝ ██╗╚██████╔╝███████║" RENK_CYAN "                 ║\n");
    printf("  ║  " RENK_YESIL "╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝" RENK_CYAN "                 ║\n");
    printf("  ║                                                              ║\n");
    printf("  ╠══════════════════════════════════════════════════════════════╣\n");
    printf(RENK_SIFIRLA);
    
    // Version and status line
    char ver_status[70];
    snprintf(ver_status, sizeof(ver_status), " v%s  RAM: %ld/%ldMB  Uptime: %ldm  Procs: %d",
             NEXOS_VERSION, ram_used, ram_total, info.uptime / 60, info.procs);
    printf(RENK_CYAN "  ║" RENK_SIFIRLA "%-62s" RENK_CYAN "║\n", ver_status);
    printf("  ╠══════════════════════════════════════════════════════════════╣\n" RENK_SIFIRLA);
    
    // Menu items
    char *menu_items[] = {
        "[1] AI OS Mode (Smart Assistant)",
        "[2] Manual Mode (Expert Shell)",
        "[3] System Information",
        "[4] Web Server Control",
        "[5] Tools & Utilities",
        "[6] Power Off"
    };
    int num_items = 6;
    
    for(int i = 0; i < num_items; i++) {
        printf(RENK_CYAN "  ║" RENK_SIFIRLA);
        if(i == selected) {
            printf(RENK_YESIL " ▶ %-59s" RENK_SIFIRLA, menu_items[i]);
        } else {
            printf("   %-59s", menu_items[i]);
        }
        printf(RENK_CYAN "║\n" RENK_SIFIRLA);
    }
    
    // Footer
    printf(RENK_CYAN "  ╠══════════════════════════════════════════════════════════════╣\n");
    printf("  ║" RENK_SIFIRLA RENK_SARI " [↑↓] Navigate        [Enter] Select        [Q] Quit      " RENK_CYAN "║\n");
    printf("  ╚══════════════════════════════════════════════════════════════╝\n" RENK_SIFIRLA);
}

void tui_menu() {
    int selected = 0;
    int num_items = 6;
    
    struct termios orig_termios, raw;
    if(tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr TUI");
        return; 
    }
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) perror("tcsetattr TUI");
    
    printf("\033[?25l"); // Hide Cursor
    
    while(1) {
        tui_draw_box(selected);
        
        char c;
        if(read(STDIN_FILENO, &c, 1) != 1) {
            usleep(100000); 
            continue; 
        }

        // Handle 'q' or 'Q' to quit
        if(c == 'q' || c == 'Q') {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
            printf("\033[?25h"); // Show Cursor
            printf("\n" RENK_SARI "Returning to system...\n" RENK_SIFIRLA);
            return;
        }
        
        if(c == '\033') { // Escape sequence
            char seq[2];
            if(read(STDIN_FILENO, &seq[0], 1) == 1 && read(STDIN_FILENO, &seq[1], 1) == 1) {
                if(seq[0] == '[') {
                    if(seq[1] == 'A') { // Up
                        selected--;
                        if(selected < 0) selected = num_items - 1;
                    } else if(seq[1] == 'B') { // Down
                        selected++;
                        if(selected >= num_items) selected = 0;
                    }
                }
            }
        } else if(c == '\n' || c == '\r') { // Enter
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
                printf("\033[?25h"); // Show Cursor
                printf("\n");
                
                switch(selected) {
                     case 0: // 1. AI OS Mode
                          while(1) {
                              printf("\n" RENK_CYAN "AI OS ['exit' for menu]> " RENK_SIFIRLA);
                              char task[256];
                             if(!fgets(task, sizeof(task), stdin)) break;
                             task[strcspn(task, "\n")] = 0;
                             if(strcmp(task, "exit") == 0 || strcmp(task, "back") == 0) break;
                             if(strlen(task) > 0) ai_command_helper_run(task); 
                         }
                         break;
                         
                    case 1: // 2. Manual Mode (Shell)
                        shell();
                        break;
                        
                    case 2: // 3. System Info
                        donanim_bilgisi_goster(); 
                        printf("\n(Press Enter to continue)");
                        getchar();
                        break;
                        
                    case 3: // 4. Web Server
                         printf(RENK_SARI "Starting Web Server...\n" RENK_SIFIRLA);
                         system("/bin/sh /opt/nexos/webserver.sh start");
                         printf("\n(Press Enter to continue)");
                         getchar(); 
                         break;
                         
                    case 4: // 5. Tools
                        yardim_menusu();
                        printf("\n(Press Enter to continue)");
                        getchar();
                        break;
                        
                    case 5: // 6. Power Off
                        printf(RENK_KIRMIZI "Sistem Kapatiliyor...\n" RENK_SIFIRLA);
                        system("poweroff -f");
                        exit(0);
                }
                
                // Back to raw
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
                printf("\033[?25l"); // Hide Cursor
            }
    }
    
    printf("\033[?25h"); // Show Cursor
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}
