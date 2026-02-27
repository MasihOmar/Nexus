#include "globals.h"

SimProcess process_table[MAX_PROCESS];
int SYSTEM_RAM_LIMIT_MB = DEFAULT_RAM_MB;

void donanim_bilgisi_goster() {
    struct sysinfo info;
    if (sysinfo(&info) != 0) return;

    struct statvfs disk_info;
    statvfs("/", &disk_info); // Read Root disk

    unsigned long toplam_disk = (disk_info.f_blocks * disk_info.f_frsize) / (1024 * 1024 * 1024);
    unsigned long bos_disk = (disk_info.f_bfree * disk_info.f_frsize) / (1024 * 1024 * 1024);
    long toplam_ram = info.totalram / (1024 * 1024);
    long bos_ram = info.freeram / (1024 * 1024);

    printf(RENK_MAVI "\n--- SYSTEM STATUS (HOST) ---\n" RENK_SIFIRLA);
    printf("CPU Cores         : %d\n", get_nprocs());
    printf("System Uptime     : %ld sec\n", info.uptime);
    printf("RAM Usage         : %ld MB / %ld MB (Free: %ld MB)\n", (toplam_ram - bos_ram), toplam_ram, bos_ram);
    printf("Disk Status (/)   : %lu GB Free / %lu GB Total\n", bos_disk, toplam_disk);
    printf("----------------------------\n");
}

void gercek_ls() {
    DIR *d = opendir("."); // Open current dir
    struct dirent *dir;
    if (d) {
        printf(RENK_MOR "\n--- HOST FILES (REAL) ---\n" RENK_SIFIRLA);
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] != '.') printf("- %s\n", dir->d_name);
        }
        closedir(d);
        printf("-------------------------------\n");
    }
}

// --- MEMORY SIMULATION (Legacy compatibility) ---

void memory_init() {
    for(int i=0; i<MAX_PROCESS; i++) {
        process_table[i].active = false;
        process_table[i].state = PROC_NEW;
        process_table[i].memory_used = 0;
        memset(process_table[i].memory_space, 0, PROCESS_MEMORY_SIZE);
    }
    scheduler_init();
}

int get_used_memory() {
    // Calculate total bytes used across all processes
    int used = 0;
    for(int i=0; i<MAX_PROCESS; i++) {
        if(process_table[i].active) {
            used += process_table[i].memory_used;
        }
    }
    return used / 1024;  // Return in KB
}

void show_memory_status() {
    int total_bytes = 0;
    int active_count = 0;
    
    for(int i=0; i<MAX_PROCESS; i++) {
        if(process_table[i].active) {
            total_bytes += process_table[i].memory_used;
            active_count++;
        }
    }
    
    printf(RENK_MAVI "\n+-------------------------------------------------------+\n");
    printf("|       VIRTUAL MEMORY MAP (İzole Bellek Haritası)       |\n");
    printf("+-------------------------------------------------------+\n" RENK_SIFIRLA);
    printf("| Total System RAM    : %d MB                           |\n", SYSTEM_RAM_LIMIT_MB);
    printf("| Active Processes    : %d / %d                          |\n", active_count, MAX_PROCESS);
    printf("| ISO Memory Per Proc : %d bytes                       |\n", PROCESS_MEMORY_SIZE);
    printf("| Total Allocated     : %d bytes                       |\n", total_bytes);
    printf(RENK_MAVI "+-------------------------------------------------------+\n" RENK_SIFIRLA);
    
    // Show each process's isolated memory
    printf("| PID  | Name           | State    | Memory Usage       |\n");
    printf(RENK_MAVI "+------╪----------------╪----------╪--------------------+\n" RENK_SIFIRLA);
    
    for(int i=0; i<MAX_PROCESS; i++) {
        if(process_table[i].active) {
            SimProcess *p = &process_table[i];
            int pct = (p->memory_used * 100) / PROCESS_MEMORY_SIZE;
            
            // Visual bar
            char bar[21];
            int filled = pct / 5;
            for(int j=0; j<20; j++) bar[j] = (j < filled) ? '#' : '.';
            bar[20] = '\0';
            
            const char *color = RENK_SIFIRLA;
            switch(p->state) {
                case PROC_RUNNING: color = RENK_YESIL; break;
                case PROC_READY:   color = RENK_CYAN; break;
                case PROC_BLOCKED: color = RENK_SARI; break;
                default: break;
            }
            
            printf("| %4d | %-14s | %s%-8s%s | [%s] %3d%% |\n",
                   p->pid, p->name, color, process_state_str(p->state), RENK_SIFIRLA,
                   bar, pct);
        }
    }
    
    if(active_count == 0) {
        printf("|            (No active processes)                      |\n");
    }
    
    printf(RENK_MAVI "+-------------------------------------------------------+\n" RENK_SIFIRLA);
}

// Legacy function - now uses process_create
void allocate_memory(char *proc_name, int size_mb) {
    SimProcess *proc = process_create(proc_name, 5);  // Default priority
    if(proc) {
        // Simulate writing some data to show memory usage
        char msg[64];
        snprintf(msg, sizeof(msg), "Process %s data block", proc_name);
        mem_write(proc->pid, 0, msg, strlen(msg) + 1);
    }
}

// Legacy function - now uses process_terminate
void kill_sim_process(int pid) {
    process_terminate(pid);
}

void set_memory_limit(int mb) {
    if (mb < 1) {
        printf("Error: Memory must be at least 1 MB.\n");
        return;
    }
    SYSTEM_RAM_LIMIT_MB = mb;
    printf(RENK_YESIL "System Memory Limit set to %d MB.\n" RENK_SIFIRLA, SYSTEM_RAM_LIMIT_MB);
}

