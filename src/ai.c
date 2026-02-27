#include "globals.h"

// =========================================================
// MODULE 4: ARTIFICIAL INTELLIGENCE (AI)
// =========================================================

// Ensure the AI model exists and is registered
static bool ai_model_ready = false;

void ensure_ai_model_exists() {
    if (ai_model_ready) return;  // Already checked
    
    // Check if Ollama binary exists
    if (access("/bin/ollama", F_OK) != 0) {
        printf(RENK_KIRMIZI "[AI] Ollama not installed!\n" RENK_SIFIRLA);
        return;
    }
    
    // Check if Modelfile exists, create if missing
    if (access("/root/Modelfile", F_OK) != 0) {
        printf(RENK_SARI "[AI] Modelfile not found, creating...\n" RENK_SIFIRLA);
        
        FILE *f = fopen("/root/Modelfile", "w");
        if (f) {
            fprintf(f, "FROM /root/nexos-expert.gguf\n\n");
            fprintf(f, "PARAMETER temperature 0.1\n");
            fprintf(f, "PARAMETER top_p 0.9\n\n");
            fprintf(f, "SYSTEM \"You are NexOS Assistant, a specialized AI for a custom Linux distro. ");
            fprintf(f, "You ONLY output BusyBox commands in the format: COMMAND: <cmd>\\nEXPLANATION: <text>. ");
            fprintf(f, "You are helpful, safe, and concise.\"\n");
            fclose(f);
            printf(RENK_YESIL "[AI] Modelfile created at /root/Modelfile\n" RENK_SIFIRLA);
        }
    }
    
    // Check if GGUF model file exists
    if (access("/root/nexos-expert.gguf", F_OK) != 0) {
        printf(RENK_KIRMIZI "[AI] Model file /root/nexos-expert.gguf not found!\n" RENK_SIFIRLA);
        printf("     AI features will not work.\n");
        return;
    }
    
    // Check if model is registered with Ollama (try to list it)
    FILE *check = popen("/bin/ollama list 2>&1 | grep -q nexos-expert && echo OK", "r");
    if (check) {
        char buf[16] = "";
        if (fgets(buf, sizeof(buf), check) == NULL || strstr(buf, "OK") == NULL) {
            // Model not registered, create it
            printf(RENK_SARI "[AI] Registering model with Ollama...\n" RENK_SIFIRLA);
            int ret = system("/bin/ollama create nexos-expert -f /root/Modelfile 2>&1");
            if (ret == 0) {
                printf(RENK_YESIL "[AI] Model 'nexos-expert' registered successfully\n" RENK_SIFIRLA);
            } else {
                printf(RENK_KIRMIZI "[AI] Failed to register model (code: %d)\n" RENK_SIFIRLA, ret);
            }
        }
        pclose(check);
    }
    
    ai_model_ready = true;
}

void ai_sor(char *soru) {
    char komut[512];
    char tampon[256];
    
    ensure_ai_model_exists();
    
    if (access("/bin/ollama", F_OK) != 0) {
        printf(RENK_KIRMIZI "\n[ERROR] AI Module Not Found!\n" RENK_SIFIRLA);
        printf("'/bin/ollama' was not found on the system.\n");
        return;
    }

    printf(RENK_SARI "[AI] NexOS Expert is Thinking...\n" RENK_SIFIRLA);
    
    sprintf(komut, "/bin/ollama run nexos-expert \"%s\" 2>&1", soru);
    FILE *pipe = popen(komut, "r");
    if (!pipe) {
        perror("popen");
        return;
    }
    
    printf(RENK_MAVI "--- AI Response ---\n" RENK_SIFIRLA);
    bool cevap_alindi = false;
    while (fgets(tampon, sizeof(tampon), pipe) != NULL) {
        printf("%s", tampon);
        cevap_alindi = true;
    }
    
    if(!cevap_alindi) {
        printf("(Empty response. Model may still be loading - try again in 30 seconds)\n");
    }

    printf("\n-------------------\n");
    pclose(pipe);
}

// =========================================================
// AI COMMAND AUTOMATION
// =========================================================

bool is_whitelisted(char *cmd) {
    const char *whitelist[] = {
        "ls", "cd", "cat", "less", "grep", "awk", "sed", "find", 
        "ps", "top", "df", "du", "mount", "umount", "ip", "ping", 
        "wget", "tar", "free", "date", "echo", "pwd", "whoami", "ngrok", 
        "doctor", "htop", "chmod", "chown", "mkdir", "rmdir", "cp", "mv",
        "head", "tail", "wc", "sort", "uniq", "uptime", "uname", "dmesg",
        "touch", "kill", "killall", "seq", "tr", "cut", "tee", "clear",
        "run", "meminfo", "killproc", "setmem", NULL
    };
    
    while(*cmd == ' ') cmd++;
    
    for(int i=0; whitelist[i] != NULL; i++) {
        int len = strlen(whitelist[i]);
        if(strncmp(cmd, whitelist[i], len) == 0) {
            if(cmd[len] == ' ' || cmd[len] == '\0') return true;
        }
    }
    return false;
}

bool has_dangerous_chars(char *cmd) {
    const char *blocked[] = { "rm", "mkfs", "dd", ">", "|", "&", ";", "$", "`", NULL };
    for(int i=0; blocked[i] != NULL; i++) {
        if(strstr(cmd, blocked[i])) return true;
    }
    return false;
}

void ai_command_helper_run(char *request) {
    char komut[1024];
    char tampon[1024];
    char extracted_cmd[512] = "";
    
    ensure_ai_model_exists();
    
    printf(RENK_SARI "[AI] NexOS Expert Thinking...\n" RENK_SIFIRLA);
    
    // Escape quotes in request to prevent shell injection
    char safe_request[512];
    char *src = request;
    char *dst = safe_request;
    while(*src && (dst - safe_request) < 500) {
        if(*src == '"' || *src == '\\' || *src == '`' || *src == '$') {
            *dst++ = '\\';
        }
        *dst++ = *src++;
    }
    *dst = '\0';
    
    snprintf(komut, sizeof(komut), "/bin/ollama run nexos-expert \"%s\" 2>&1", safe_request);
    
    #define MAX_LINES 2048
    #define MAX_LEN 256
    static char buffer[MAX_LINES][MAX_LEN];
    int line_count = 0;
    
    FILE *pipe = popen(komut, "r");
    if (!pipe) { perror("popen"); return; }
    
    bool capturing = false;
    bool found_explanation = false;
    char explanation[1024] = "";
    
    // Parse output silently, only extract COMMAND and EXPLANATION
    while (fgets(tampon, sizeof(tampon), pipe) != NULL) {
        tampon[strcspn(tampon, "\n")] = 0;
        remove_ansi(tampon);

        if (line_count < MAX_LINES) {
             strncpy(buffer[line_count], tampon, MAX_LEN - 1);
             buffer[line_count][MAX_LEN - 1] = '\0';
             line_count++;
        }
        
        // Check for EXPLANATION:
        char *exp_ptr = strcasestr(tampon, "EXPLANATION:");
        if(exp_ptr) {
            found_explanation = true;
            exp_ptr += 12;
            while(*exp_ptr == ' ') exp_ptr++;
            strncat(explanation, exp_ptr, sizeof(explanation) - strlen(explanation) - 1);
            continue;
        }
        
        // If already found explanation, keep appending
        if(found_explanation && strlen(tampon) > 0 && !strcasestr(tampon, "COMMAND:")) {
            if(strlen(explanation) > 0) strncat(explanation, " ", sizeof(explanation) - strlen(explanation) - 1);
            strncat(explanation, tampon, sizeof(explanation) - strlen(explanation) - 1);
        }
        
        char *ptr_start = NULL;
        char *search_pos = tampon;
        
        while (*search_pos) {
            if (strncasecmp(search_pos, "COMMAND:", 8) == 0) {
                ptr_start = search_pos;
                break;
            }
            search_pos++;
        }
        
        if (ptr_start && strlen(extracted_cmd) == 0) {
            capturing = true;
            ptr_start += 8;
            while(*ptr_start && (*ptr_start == ' ' || *ptr_start == '\t' || *ptr_start == '*' || *ptr_start == '`' || *ptr_start == '\'' || *ptr_start == '"')) {
                ptr_start++;
            }
            strncat(extracted_cmd, ptr_start, sizeof(extracted_cmd) - strlen(extracted_cmd) - 1);
            continue; 
        }
        
        if (capturing) {
            bool stop_keyword = false;
            if (strncasecmp(tampon, "EXPLANATION:", 12) == 0) stop_keyword = true;
            if (strncasecmp(tampon, "NOTE:", 5) == 0) stop_keyword = true;
            
            if (stop_keyword) {
                capturing = false;
            } else {
                if (!stop_keyword && strlen(tampon) > 0) {
                    if (strlen(extracted_cmd) > 0 && extracted_cmd[strlen(extracted_cmd)-1] != ' ') {
                        strncat(extracted_cmd, " ", sizeof(extracted_cmd) - strlen(extracted_cmd) - 1);
                    }
                    strncat(extracted_cmd, tampon, sizeof(extracted_cmd) - strlen(extracted_cmd) - 1);
                }
            }
        }
    }
    
    int len = strlen(extracted_cmd);
    while(len > 0 && (extracted_cmd[len-1] == ' ' || extracted_cmd[len-1] == '*' || extracted_cmd[len-1] == '`')) {
        extracted_cmd[--len] = '\0';
    }
    
    if (len >= 2) {
        if ((extracted_cmd[0] == '"' && extracted_cmd[len-1] == '"') || 
            (extracted_cmd[0] == '\'' && extracted_cmd[len-1] == '\'')) {
            memmove(extracted_cmd, extracted_cmd + 1, len - 2);
            extracted_cmd[len - 2] = '\0';
            len = strlen(extracted_cmd);
        }
    }
    
    // FALLBACK PARSING: If no COMMAND: found, try other patterns
    if (len == 0) {
        // Try to find command in backticks: `command`
        for(int i = 0; i < line_count && len == 0; i++) {
            char *tick1 = strchr(buffer[i], '`');
            if(tick1) {
                char *tick2 = strchr(tick1 + 1, '`');
                if(tick2 && tick2 - tick1 > 1 && tick2 - tick1 < 200) {
                    int cmd_len = tick2 - tick1 - 1;
                    strncpy(extracted_cmd, tick1 + 1, cmd_len);
                    extracted_cmd[cmd_len] = '\0';
                    len = cmd_len;
                }
            }
        }
    }
    
    // FALLBACK 2: Look for known command patterns in raw output
    if (len == 0) {
        const char *known_cmds[] = {"doctor", "traffic", "htop", "web", "sysinfo", 
                                     "ls", "cat", "free", "ping", "ifconfig", 
                                     "ps", "top", "df", "uptime", "dmesg", NULL};
        for(int i = 0; i < line_count && len == 0; i++) {
            for(int j = 0; known_cmds[j] != NULL; j++) {
                char *found = strstr(buffer[i], known_cmds[j]);
                if(found) {
                    // Extract the word and any following arguments
                    char *start = found;
                    char *end = start;
                    while(*end && *end != '\n' && *end != '.' && *end != ',' && (end - start) < 100) {
                        end++;
                    }
                    int cmd_len = end - start;
                    // Trim trailing whitespace
                    while(cmd_len > 0 && (start[cmd_len-1] == ' ' || start[cmd_len-1] == '*')) cmd_len--;
                    if(cmd_len > 0) {
                        strncpy(extracted_cmd, start, cmd_len);
                        extracted_cmd[cmd_len] = '\0';
                        len = cmd_len;
                        break;
                    }
                }
            }
        }
    }
    
    pclose(pipe);
    
    if (line_count > 20) {
        int offset = 0;
        int view_height = 20; 
        char ch;
        
        while(1) {
            printf("\033[H\033[J");
            printf(RENK_MAVI "--- AI RESULT (Reader Mode) ---\n" RENK_SIFIRLA);
            printf(RENK_SARI "UP/DOWN: Scroll | Q: Quit\n" RENK_SIFIRLA);
            printf("----------------------------------------\n");
            
            for(int i = 0; i < view_height; i++) {
                int idx = offset + i;
                if(idx < line_count) {
                    printf("%s\n", buffer[idx]);
                } else {
                    printf("~\n"); 
                }
            }
            
             printf("\n" RENK_MAVI "[u:Up] [d:Down] [q:Quit] Choice: " RENK_SIFIRLA);
            char input[10];
            fgets(input, sizeof(input), stdin);
            ch = input[0];
            
            if (ch == 'q' || ch == 'Q') break;
            if (ch == 'd') {
                if(offset + view_height < line_count) offset += 5;
            }
            if (ch == 'u') {
                if(offset > 0) offset -= 5;
                if(offset < 0) offset = 0;
            }
        }
    }
    
    // Display clean output
    if(strlen(extracted_cmd) > 0) {
        printf("\n" RENK_CYAN "+---------------------------------------+\n");
        printf("|" RENK_SIFIRLA " AI Response                          " RENK_CYAN "|\n");
        printf("+---------------------------------------+\n" RENK_SIFIRLA);
        
        // Show explanation if found
        if(strlen(explanation) > 0) {
            printf(RENK_SARI "Explanation:" RENK_SIFIRLA " %s\n\n", explanation);
        }
        
        bool safe = true;
        if (!is_whitelisted(extracted_cmd) || has_dangerous_chars(extracted_cmd)) {
            printf(RENK_KIRMIZI "Command (UNSAFE):" RENK_SIFIRLA " %s\n", extracted_cmd);
            safe = false;
        } else {
            printf(RENK_YESIL "Command:" RENK_SIFIRLA " %s\n", extracted_cmd);
        }
        
        if(safe) {
             printf("Execute? [Y/n]: ");
        } else {
             printf(RENK_KIRMIZI "WARNING! Unsafe command. Execute anyway? [y/N]: " RENK_SIFIRLA);
        }
        
        char c = getchar();
        while(getchar() != '\n'); 
        
        bool run = false;
        if (safe) {
             if (c == 'e' || c == 'E' || c == '\n') run = true; 
             if (c == 'y' || c == 'Y' || c == '\n') run = true;
        } else {
             if (c == 'y' || c == 'Y') run = true;
        }
        
        if(run) {
             printf(RENK_YESIL "Executing...\n" RENK_SIFIRLA);
             
                if (strncmp(extracted_cmd, "doctor", 6) == 0) {
                    system("/bin/sh /opt/nexos/doctor.sh");
                } 
                else if (strncmp(extracted_cmd, "traffic", 7) == 0 || strncmp(extracted_cmd, "analyze", 7) == 0) {
                    system("/bin/sh /opt/nexos/analyze_traffic.sh run");
                }
                else if (strncmp(extracted_cmd, "web", 3) == 0) {
                   system("/bin/sh /opt/nexos/webserver.sh start"); 
                }
                else if (strncmp(extracted_cmd, "server", 6) == 0) {
                   if (strlen(extracted_cmd) > 7) {
                        char cmd[256];
                        snprintf(cmd, sizeof(cmd), "/bin/sh /opt/nexos/webserver.sh %s", extracted_cmd + 7);
                        system(cmd);
                   } else {
                       system("/bin/sh /opt/nexos/webserver.sh help");
                   }
                }
                else if (strncmp(extracted_cmd, "htop", 4) == 0) {
                    if(system("which htop >/dev/null 2>&1") == 0) {
                        system("htop");
                    } else {
                        printf(RENK_KIRMIZI "Hata: htop yuklu degil veya PATH'de bulunamadi.\n" RENK_SIFIRLA);
                    }
                }
                else {
                    system(extracted_cmd);
                }
            }

    } else {
        printf(RENK_KIRMIZI "AI anlamli bir komut uretemedi.\n" RENK_SIFIRLA);
    }
}
