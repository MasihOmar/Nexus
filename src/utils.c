#include "globals.h"

// --- CUSTOM PAGER WITH VISUAL SCROLLBAR ---
void show_scrollable_text(char *title, char *text) {
    // Basic Pager Implementation
    #define MAX_PAGER_LINES 200
    char *lines[MAX_PAGER_LINES];
    int line_count = 0;
    
    char *copy = strdup(text);
    char *p = strtok(copy, "\n");
    while(p && line_count < MAX_PAGER_LINES) {
        lines[line_count++] = p;
        p = strtok(NULL, "\n");
    }
    
    int offset = 0;
    int height = 18; // Viewport height (slightly smaller for better fit)
    struct termios orig, raw;
    tcgetattr(STDIN_FILENO, &orig);
    raw = orig;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;   // Read at least 1 character
    raw.c_cc[VTIME] = 0;  // No timeout
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    
    while(1) {
        printf("\033[H\033[J"); // Clear screen
        
        // Fixed width box: 78 chars total (76 inner + 2 borders)
        printf(RENK_MAVI "+------------------------------------------------------------------------------+\n");
        
        // Title line - pad to exactly 76 chars
        printf("|" RENK_SIFIRLA "  %-74s" RENK_MAVI "|\n", title);
        
        printf("+------------------------------------------------------------------------------+\n" RENK_SIFIRLA);
        
        for(int i = 0; i < height; i++) {
            int idx = offset + i;
            char line_buf[80];
            memset(line_buf, ' ', sizeof(line_buf));
            
            if(idx < line_count) {
                // Copy line content, truncate if needed
                int len = strlen(lines[idx]);
                if(len > 73) len = 73;
                memcpy(line_buf, lines[idx], len);
            }
            line_buf[73] = '\0';
            
            printf(RENK_MAVI "|" RENK_SIFIRLA " %-73s", line_buf);
            
            // Scrollbar position
            int bar_pos = (line_count > height) ? (offset * height) / line_count : -1;
            if (i == bar_pos) {
                printf(RENK_YESIL "#" RENK_SIFIRLA);
            } else {
                printf(RENK_MAVI "|" RENK_SIFIRLA);
            }
            printf(RENK_MAVI "|\n" RENK_SIFIRLA);
        }
        
        // Footer with proper padding
        printf(RENK_MAVI "+------------------------------------------------------------------------------+\n");
        
        char footer[80];
        snprintf(footer, sizeof(footer), " [Up/Down] Scroll  [PgUp/Dn] Page  [Q] Quit     Lines %d-%d of %d",
               offset + 1, 
               (offset + height < line_count ? offset + height : line_count), 
               line_count);
        printf("|" RENK_SIFIRLA "%-76s" RENK_MAVI "|\n", footer);
        
        printf("+------------------------------------------------------------------------------+\n" RENK_SIFIRLA);
        
        fflush(stdout);  // IMPORTANT: Flush output before reading input
        
        char c;
        if(read(STDIN_FILENO, &c, 1) == 1) {
            if(c == 'q' || c == 'Q') break;
            
            // Handle escape sequences for arrow keys
            if(c == '\033') {
                char seq[3];
                if(read(STDIN_FILENO, &seq[0], 1) == 1) {
                    if(seq[0] == '[') {
                        if(read(STDIN_FILENO, &seq[1], 1) == 1) {
                            if(seq[1] == 'A' && offset > 0) offset--;  // Up arrow
                            if(seq[1] == 'B' && offset + height < line_count) offset++;  // Down arrow
                            if(seq[1] == '5') { // Page Up
                                read(STDIN_FILENO, &seq[2], 1); // Read trailing ~
                                offset -= height;
                                if(offset < 0) offset = 0;
                            }
                            if(seq[1] == '6') { // Page Down
                                read(STDIN_FILENO, &seq[2], 1); // Read trailing ~
                                offset += height;
                                if(offset + height > line_count) offset = line_count - height;
                                if(offset < 0) offset = 0;
                            }
                        }
                    }
                }
            }
            
            // Also handle j/k for vim-style navigation
            if(c == 'j' && offset + height < line_count) offset++;
            if(c == 'k' && offset > 0) offset--;
        }
    }
    
    free(copy);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
    printf("\033[H\033[J");  // Clear screen on exit
}

void komut_calistir_ve_yazdir(char *baslik, char *komut, FILE *rapor_dosyasi) {
    char tampon[256];
    FILE *pipe = popen(komut, "r"); // Execute Linux command
    
    if (!pipe) return;

    printf(RENK_CYAN "\n--- %s ---\n" RENK_SIFIRLA, baslik);
    if(rapor_dosyasi) fprintf(rapor_dosyasi, "\n--- %s ---\n", baslik);

    while (fgets(tampon, sizeof(tampon), pipe) != NULL) {
        printf("%s", tampon); // Print to screen
        if(rapor_dosyasi) fprintf(rapor_dosyasi, "%s", tampon); // Write to file
    }
    pclose(pipe);
}

void remove_ansi(char *str) {
    char *read = str;
    char *write = str;
    bool in_esc = false;
    
    while (*read) {
        if (*read == '\033') {
            in_esc = true;
            read++;
            if (*read == '[') {
                read++;
                while (*read && !isalpha(*read)) read++;
                if (*read) read++; 
            }
            in_esc = false;
        } else {
            *write++ = *read++;
        }
    }
    *write = '\0';
}

void system_wrapper(char *base_cmd, char *arg) {
    char full_cmd[512];
    if(arg) {
        // Sanitize arg - escape shell metacharacters to prevent command injection
        char safe_arg[256];
        char *src = arg;
        char *dst = safe_arg;
        while(*src && (dst - safe_arg) < 250) {
            // Escape dangerous shell metacharacters
            if(strchr(";&|`$(){}[]<>\"'\\!#", *src)) {
                *dst++ = '\\';
            }
            *dst++ = *src++;
        }
        *dst = '\0';
        snprintf(full_cmd, sizeof(full_cmd), "%s %s", base_cmd, safe_arg);
    } else {
        snprintf(full_cmd, sizeof(full_cmd), "%s", base_cmd);
    }
    system(full_cmd);
}
