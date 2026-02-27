#include "globals.h"

#define HISTORY_MAX 20
#define CMD_LEN 200
char history[HISTORY_MAX][CMD_LEN];
int history_count = 0;
int history_pos = 0;

// Forward declarations
char* tab_complete(char *partial);
void show_welcome_banner();

void add_to_history(char *cmd) {
    if (strlen(cmd) == 0) return;
    if (history_count > 0 && strcmp(history[history_count-1], cmd) == 0) return; 
    
    if (history_count < HISTORY_MAX) {
        strcpy(history[history_count++], cmd);
    } else {
        for (int i = 1; i < HISTORY_MAX; i++) {
            strcpy(history[i-1], history[i]);
        }
        strcpy(history[HISTORY_MAX-1], cmd);
    }
    history_pos = history_count;
}

int satir_oku(char *tampon, int max_boyut) {
    struct termios orig_termios, raw;
    if(tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        return 0; 
    }
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON); 
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) perror("tcsetattr");

    int pos = 0;
    int len = 0;
    tampon[0] = '\0';
    char c;
    
    while(1) {
        if(read(STDIN_FILENO, &c, 1) == 1) {
            if (c == '\n' || c == '\r') {
                printf("\n");
                break;
            } else if (c == 127 || c == 8) { // Backspace
                if (pos > 0) {
                    if (pos == len) {
                        printf("\b \b");
                        pos--; len--;
                        tampon[pos] = '\0';
                    }
                }
            } else if (c == '\033') { // Escape Sequence
                char seq[2];
                if (read(STDIN_FILENO, &seq[0], 1) == 1 && read(STDIN_FILENO, &seq[1], 1) == 1) {
                    if (seq[0] == '[') {
                        if (seq[1] == 'D') { // Left
                           if (pos > 0) {
                               pos--;
                               printf("\033[D");
                           }
                        } else if (seq[1] == 'C') { // Right
                           if (pos < len) {
                               pos++;
                               printf("\033[C");
                           }
                        } else if (seq[1] == 'A') { // UP (History Prev)
                            if (history_pos > 0) {
                                history_pos--;
                                while(pos > 0) { printf("\b \b"); pos--; }
                                strcpy(tampon, history[history_pos]);
                                len = strlen(tampon);
                                pos = len;
                                printf("%s", tampon);
                            }
                        } else if (seq[1] == 'B') { // DOWN (History Next)
                            if (history_pos < history_count) {
                                history_pos++;
                                while(pos > 0) { printf("\b \b"); pos--; }
                                
                                if (history_pos < history_count) {
                                    strcpy(tampon, history[history_pos]);
                                } else {
                                    tampon[0] = '\0'; 
                                }
                                len = strlen(tampon);
                                pos = len;
                                printf("%s", tampon);
                            }
                        }
                    }
                }
            } else if (c == '\t') { // Tab - Auto-complete
                if(len > 0) {
                    char *completion = tab_complete(tampon);
                    if(completion) {
                        // Clear current input
                        while(pos > 0) { printf("\b \b"); pos--; }
                        // Copy completion
                        strcpy(tampon, completion);
                        len = strlen(tampon);
                        pos = len;
                        printf("%s", tampon);
                    } else {
                        printf("\a");  // Beep - no match
                    }
                }
            } else if (c == 12) { // Ctrl+L - Clear screen
                printf("\033[H\033[J");
                show_welcome_banner();
                printf(RENK_YESIL "root" RENK_SIFIRLA "@" RENK_CYAN "nexos" RENK_SIFIRLA ":" RENK_MAVI "~" RENK_SIFIRLA "$ ");
                printf("%s", tampon);
            } else if (!iscntrl(c) && len < max_boyut - 1) {
                if (pos == len) {
                    printf("%c", c);
                    tampon[pos++] = c;
                    len++;
                    tampon[len] = '\0';
                } else {
                    for (int i = len; i > pos; i--) {
                        tampon[i] = tampon[i-1];
                    }
                    tampon[pos] = c;
                    len++;
                    tampon[len] = '\0';
                    printf("%s", &tampon[pos]);
                    int move_back = len - pos - 1;
                    for(int k=0; k<move_back; k++) printf("\033[D");
                    pos++;
                }
            }
            fflush(stdout); 
        }
    }
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); 
    return len;
}

void log_analizi_yap(char *dosya_adi) {
    if(access(dosya_adi, F_OK) != 0) {
        printf(RENK_KIRMIZI "Error: File %s not found! Create log first.\n" RENK_SIFIRLA, dosya_adi);
        return;
    }

    printf(RENK_SARI "Starting Log Analysis: %s\n" RENK_SIFIRLA, dosya_adi);
    printf("Writing to report file...\n");

    char rapor_adi[50];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(rapor_adi, "report_%02d-%02d-%d.txt", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
    
    FILE *rapor = fopen(rapor_adi, "w");
    if(rapor) {
        fprintf(rapor, "=== AUTOMATED LOG ANALYSIS REPORT ===\n");
        fprintf(rapor, "Date: %s\n", rapor_adi);
        fprintf(rapor, "Analyzed File: %s\n", dosya_adi);
    }

    char komut_ip[256];
    sprintf(komut_ip, "awk '{print $1}' %s | sort | uniq -c | sort -nr | head -5", dosya_adi);
    komut_calistir_ve_yazdir("TOP 5 VISITOR IPS", komut_ip, rapor);

    char komut_sayfa[256];
    sprintf(komut_sayfa, "awk '{print $7}' %s | sort | uniq -c | sort -nr | head -5", dosya_adi);
    komut_calistir_ve_yazdir("TOP ACCESSED PAGES", komut_sayfa, rapor);

    char komut_hata[256];
    sprintf(komut_hata, "grep -E \" 404 | 500 \" %s | awk '{print $9}' | sort | uniq -c", dosya_adi);
    komut_calistir_ve_yazdir("ERROR CODE ANALYSIS (404/500)", komut_hata, rapor);

    if(rapor) {
        fclose(rapor);
        printf(RENK_YESIL "\n>> Report saved successfully: %s\n" RENK_SIFIRLA, rapor_adi);
    }
}

void yardim_menusu() {
    char buffer[4096] = "";  
    strcat(buffer, "--- COMMAND LIST ---\n");
    strcat(buffer, "analyze <file> : Log file analysis\n");
    strcat(buffer, "sysinfo        : Real hardware report\n");
    strcat(buffer, "ls             : View virtual files (SFS)\n");
    strcat(buffer, "write <n> <t>  : Create virtual file\n");
    strcat(buffer, "ai             : Ask questions to AI\n");
    strcat(buffer, "lss            : List system files (ls -l)\n");
    strcat(buffer, "cat <file>     : Read file content\n");
    strcat(buffer, "less <file>    : Read file (Scrollable)\n");
    strcat(buffer, "ps             : List processes\n");
    strcat(buffer, "kill <PID>     : Kill process\n");
    strcat(buffer, "ping <addr>    : Internet test\n");
    strcat(buffer, "free           : Memory status\n");
    strcat(buffer, "clean          : Clear cache (Free RAM)\n");
    strcat(buffer, "web            : Start Web Server (Port 80)\n");
    strcat(buffer, "vi <file>      : Text Editor\n");
    strcat(buffer, "top            : Process Monitor (Exit: q)\n");
    strcat(buffer, "wget <url>     : Download File\n");
    strcat(buffer, "analyze        : Advanced Log Analysis (HTML)\n");
    strcat(buffer, "traffic        : Analyze web traffic (AI)\n");
    strcat(buffer, "doctor         : System health scan (AI)\n");
    strcat(buffer, "htop           : Advanced task manager\n");
    strcat(buffer, "web            : Manage web server\n");
    strcat(buffer, "run <app> <mb> : Start app with Memory Limit\n");
    strcat(buffer, "meminfo        : Show Virtual Memory\n");
    strcat(buffer, "killproc <pid> : Kill Simulated Process\n");
    strcat(buffer, "setmem <mb>    : Set Total RAM (Def: 64MB)\n");
    strcat(buffer, "\n--- PROCESS ISOLATION (HOMEWORK) ---\n");
    strcat(buffer, "proclist       : Show process table\n");
    strcat(buffer, "memdemo        : Memory isolation demo\n");
    strcat(buffer, "scheddemo      : Scheduler demo\n");
    strcat(buffer, "ipcdemo        : IPC demo\n");
    strcat(buffer, "newproc <name> : Create new process\n");
    strcat(buffer, "exit           : Shutdown system\n");
    
    show_scrollable_text("HELP MENU", buffer);
}

// Tab completion for commands
static const char *completions[] = {
    "analyze", "sysinfo", "ls", "lss", "cat", "less", "kill", "killproc",
    "ping", "free", "clean", "web", "top", "vi", "wget", "traffic", "doctor",
    "htop", "run", "meminfo", "setmem", "exit", "help", "clear", "cd",
    "memdemo", "scheddemo", "ipcdemo", "proclist", "newproc", "server",
    "monitor", "write", "ps", "ifconfig", NULL
};

char* tab_complete(char *partial) {
    int len = strlen(partial);
    if(len == 0) return NULL;
    
    for(int i = 0; completions[i] != NULL; i++) {
        if(strncmp(completions[i], partial, len) == 0) {
            return (char*)completions[i];
        }
    }
    return NULL;
}

void show_welcome_banner() {
    // Get system info for status bar
    struct sysinfo info;
    sysinfo(&info);
    long ram_mb = info.totalram / (1024 * 1024);
    
    printf("\033[H\033[J");  // Clear screen
    
    printf(RENK_CYAN);
    printf("  +------------------------------------------------------+\n");
    printf("  |" RENK_SIFIRLA " NexOS Expert Shell v%-32s" RENK_CYAN "|\n", NEXOS_VERSION);
    printf("  +------------------------------------------------------+\n");
    printf(RENK_SIFIRLA);
    
    // System status line
    char status_line[60];
    snprintf(status_line, sizeof(status_line), " RAM: %ldMB | Uptime: %ldm | Procs: %d",
             ram_mb, info.uptime / 60, info.procs);
    printf(RENK_CYAN "  |" RENK_SIFIRLA "%-54s" RENK_CYAN "|\n" RENK_SIFIRLA, status_line);
    
    printf(RENK_CYAN "  +------------------------------------------------------+\n");
    printf("  |" RENK_SIFIRLA " Quick Tips:                                            " RENK_CYAN "|\n");
    printf("  |" RENK_SIFIRLA "   " RENK_SARI "help" RENK_SIFIRLA "     - Show all commands                      " RENK_CYAN "|\n");
    printf("  |" RENK_SIFIRLA "   " RENK_SARI "Tab" RENK_SIFIRLA "      - Auto-complete commands                 " RENK_CYAN "|\n");
    printf("  |" RENK_SIFIRLA "   " RENK_SARI "Up/Down" RENK_SIFIRLA "  - Browse command history                 " RENK_CYAN "|\n");
    printf("  |" RENK_SIFIRLA "   " RENK_SARI "exit" RENK_SIFIRLA "     - Return to main menu                    " RENK_CYAN "|\n");
    printf("  +------------------------------------------------------+\n" RENK_SIFIRLA);
    printf("\n");
}

void shell() {
    char komut[50], arg1[50], arg2[100];
    
    show_welcome_banner();
    
    while(1) {
        // Get current directory for prompt
        char cwd[64];
        if(getcwd(cwd, sizeof(cwd)) == NULL) strcpy(cwd, "~");
        
        // Shorten path if too long
        char *display_path = cwd;
        if(strcmp(cwd, "/root") == 0) display_path = "~";
        else if(strlen(cwd) > 20) display_path = strrchr(cwd, '/');
        
        printf(RENK_YESIL "root" RENK_SIFIRLA "@" RENK_CYAN "nexos" RENK_SIFIRLA ":" 
               RENK_MAVI "%s" RENK_SIFIRLA "$ ", display_path ? display_path : "~");
        
        char line[200];
        history_pos = history_count;
        if (satir_oku(line, 200) == 0) continue; 
        
        add_to_history(line); 
        
        arg1[0] = '\0'; arg2[0] = '\0';
        int n = sscanf(line, "%49s %49s %99s", komut, arg1, arg2);
        
        if(n < 1) continue;

        if(strcmp(komut, "exit") == 0 || strcmp(komut, "cikis") == 0) break;
        else if(strcmp(komut, "help") == 0 || strcmp(komut, "yardim") == 0) yardim_menusu();
        else if(strcmp(komut, "cd") == 0) {
            if(n > 1) {
                if(chdir(arg1) != 0) {
                    perror("cd");
                }
            } else {
                chdir("/root"); 
            }
        }
        else if(strcmp(komut, "sysinfo") == 0) donanim_bilgisi_goster();
        else if(strcmp(komut, "lsreal") == 0 || strcmp(komut, "lss") == 0) system_wrapper("/bin/busybox ls -l", NULL);
        else if(strcmp(komut, "ls") == 0) sfs_liste();
        else if(strcmp(komut, "ps") == 0) system_wrapper("/bin/busybox ps aux", NULL);
        else if(strcmp(komut, "cat") == 0) {
            if(n>1) system_wrapper("/bin/busybox cat", arg1);
            else printf("Usage: cat <file>\n");
        }
        else if(strcmp(komut, "less") == 0) {
            if(n>1) system_wrapper("/bin/busybox less", arg1);
            else printf("Usage: less <file>\n");
        }
        else if(strcmp(komut, "kill") == 0) {
            if(n>1) system_wrapper("/bin/busybox kill -9", arg1);
            else printf("Usage: kill <PID>\n");
        }
        else if(strcmp(komut, "ifconfig") == 0) {
            system("/bin/busybox ifconfig");
            printf("\nUse 'ping 8.8.8.8' to test connection.\n");
        }
        else if(strcmp(komut, "ping") == 0) {
            if(n>1) system_wrapper("/bin/busybox ping -c 4", arg1);
            else printf("Usage: ping <addr>\n");
        }
        else if(strcmp(komut, "free") == 0) system_wrapper("/bin/busybox free -h", NULL);
        else if(strcmp(komut, "clean") == 0) {
            printf(RENK_SARI "Clearing cache...\n" RENK_SIFIRLA);
            system("echo 3 > /proc/sys/vm/drop_caches");
            printf(RENK_YESIL "Completed.\n" RENK_SIFIRLA);
        }
        else if(strcmp(komut, "analyze") == 0 || strcmp(komut, "analiz") == 0) {
            if(n>1) log_analizi_yap(arg1); 
            else {
                 char cmd[256];
                 snprintf(cmd, sizeof(cmd), "/bin/sh /opt/nexos/analyze_traffic.sh run");
                 system(cmd);
            }
        }
        else if(strcmp(komut, "write") == 0 || strcmp(komut, "yaz") == 0) {
            if(n>2) sfs_dosya_olustur(arg1, arg2);
            else printf("Usage: write <name> <txt>\n");
        }
        /*
        else if(strcmp(komut, "ai") == 0) {
             if(n > 1) {
                char *SoruBaslangic = strstr(line, "ai ");
                if(SoruBaslangic) ai_sor(SoruBaslangic + 3);
            } else {
                printf("AI Question: ");
                char soru[200];
                satir_oku(soru, 200);
                ai_sor(soru);
            }
        }
        */
        else if(strcmp(komut, "web") == 0) {
            printf(RENK_SARI "Starting Web Server...\n" RENK_SIFIRLA);
            system("/bin/sh /opt/nexos/webserver.sh start");
        }
        else if(strcmp(komut, "top") == 0) system_wrapper("/bin/busybox top", NULL);
        else if(strcmp(komut, "vi") == 0) {
            if(n>1) system_wrapper("/bin/busybox vi", arg1);
            else printf("Usage: vi <file>\n");
        }
        else if(strcmp(komut, "wget") == 0) {
            if(n>1) system_wrapper("/bin/busybox wget", arg1);
            else printf("Usage: wget <url>\n");
        }
        else if(strcmp(komut, "server") == 0) {
            if(n>1) {
                char cmd[256];
                snprintf(cmd, sizeof(cmd), "/bin/sh /opt/nexos/webserver.sh %s", arg1);
                system(cmd);
            } else {
                system("/bin/sh /opt/nexos/webserver.sh start");
            }
        }
        else if (strcmp(komut, "traffic") == 0) {
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "/bin/sh /opt/nexos/analyze_traffic.sh run");
            system(cmd);
        }
        else if (strcmp(komut, "monitor") == 0) {
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "/bin/sh /opt/nexos/analyze_traffic.sh monitor");
            system(cmd);
        }
        // MEMORY COMMANDS
        else if(strcmp(komut, "meminfo") == 0) show_memory_status();
        else if(strcmp(komut, "run") == 0) {
            if(n > 2) {
                allocate_memory(arg1, atoi(arg2));
            } else printf("Usage: run <app_name> <ram_mb>\n");
        }
        else if(strcmp(komut, "killproc") == 0) {
            if(n > 1) kill_sim_process(atoi(arg1));
            else printf("Usage: killproc <pid>\n");
        }
        else if(strcmp(komut, "setmem") == 0) {
            if(n > 1) set_memory_limit(atoi(arg1));
            else printf("Usage: setmem <mb>\n");
        }
        else if (strcmp(komut, "doctor") == 0) {
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "/bin/sh /opt/nexos/doctor.sh");
            system(cmd);
        } else if (strcmp(komut, "htop") == 0) {
            if(system("which htop >/dev/null 2>&1") == 0) {
                system("htop");
            } else {
                printf(RENK_KIRMIZI "Error: htop not installed.\n" RENK_SIFIRLA);
            }
        }
        else if (strcmp(komut, "clear") == 0) {
            printf("\033[H\033[J");
        }
        // NEW: Process Isolation Demo Commands (Homework)
        else if (strcmp(komut, "memdemo") == 0) {
            mem_demo();
        }
        else if (strcmp(komut, "scheddemo") == 0) {
            scheduler_demo();
        }
        else if (strcmp(komut, "ipcdemo") == 0) {
            ipc_demo();
        }
        else if (strcmp(komut, "proclist") == 0) {
            process_list();
        }
        else if (strcmp(komut, "newproc") == 0) {
            if(n > 1) {
                int prio = (n > 2) ? atoi(arg2) : 5;
                process_create(arg1, prio);
            } else {
                printf("Usage: newproc <name> [priority]\n");
            }
        }
        else {
             char busybox_cmd[512];
             
             char *args_ptr = line;
             while(*args_ptr && *args_ptr != ' ') args_ptr++;
             while(*args_ptr && *args_ptr == ' ') args_ptr++;
             
             if (*args_ptr) {
                 snprintf(busybox_cmd, sizeof(busybox_cmd), "/bin/busybox %s %s", komut, args_ptr);
             } else {
                 snprintf(busybox_cmd, sizeof(busybox_cmd), "/bin/busybox %s", komut);
             }
             
             int ret = system(busybox_cmd);
             if (ret != 0 && ret != 127) {
             }
        }
    }
}
