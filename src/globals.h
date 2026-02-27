#ifndef GLOBALS_H
#define GLOBALS_H

// --- VERSION INFO ---
#define NEXOS_VERSION "1.2.0"
#define NEXOS_BUILD_DATE __DATE__
#define NEXOS_BUILD_TIME __TIME__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <time.h>
#include <termios.h>
#include <ctype.h>

// --- COLORS ---
#define RENK_KIRMIZI "\033[1;31m"
#define RENK_YESIL   "\033[1;32m"
#define RENK_MAVI    "\033[1;34m"
#define RENK_SARI    "\033[1;33m"
#define RENK_MOR     "\033[1;35m"
#define RENK_CYAN    "\033[1;36m"
#define RENK_SIFIRLA "\033[0m"

// --- SETTINGS ---
#define DISK_DOSYASI "sanal_disk.bin"
#define MAX_DOSYA 50
#define MAX_DOSYA_BOYUTU 1024

// --- DATA STRUCTURES ---
typedef struct {
    char ad[32];
    int boyut;
    char icerik[MAX_DOSYA_BOYUTU];
    bool dolu;
} DosyaBlogu;

extern DosyaBlogu sanal_disk[MAX_DOSYA];

// --- MEMORY SIMULATION (ENHANCED) ---
#define MAX_PROCESS 10
#define DEFAULT_RAM_MB 64
#define PROCESS_MEMORY_SIZE 4096  // 4KB per process isolated space
#define MAX_IPC_MESSAGES 10
#define MAX_IPC_MSG_SIZE 256

// Process States (Süreç Durumları)
typedef enum {
    PROC_NEW,        // Yeni oluşturulmuş
    PROC_READY,      // Çalışmaya hazır
    PROC_RUNNING,    // Çalışıyor
    PROC_BLOCKED,    // Bloklanmış (I/O bekliyor)
    PROC_TERMINATED  // Sonlandırılmış
} ProcessState;

// IPC Message Structure (Süreçler Arası İletişim)
typedef struct {
    int sender_pid;
    int receiver_pid;
    char content[MAX_IPC_MSG_SIZE];
    bool read;
} IPCMessage;

// Enhanced Process Structure with Isolated Memory
typedef struct {
    int pid;
    char name[32];
    ProcessState state;
    int priority;           // 0-9, higher = more important
    int time_slice;         // Remaining time quantum
    
    // ISOLATED MEMORY SIMULATION (İzole Bellek)
    int memory_base;        // Virtual base address
    int memory_limit;       // Size in bytes
    char memory_space[PROCESS_MEMORY_SIZE];  // Isolated memory
    int memory_used;        // Bytes used in isolated space
    
    // IPC Mailbox
    IPCMessage mailbox[MAX_IPC_MESSAGES];
    int mailbox_count;
    
    bool active;
} SimProcess;

extern SimProcess process_table[MAX_PROCESS];
extern int SYSTEM_RAM_LIMIT_MB;
extern int current_running_pid;
extern int scheduler_tick_count;

// --- PROTOTYPES ---

// fs.c
void sfs_yukle();
void sfs_kaydet();
void sfs_dosya_olustur(char *isim, char *icerik);
void sfs_liste();
void sfs_dosya_oku(char *isim);
void sfs_dosya_sil(char *isim);

// hardware.c (Legacy)
void donanim_bilgisi_goster();
void gercek_ls();
void memory_init();
int get_used_memory();
void show_memory_status();
void allocate_memory(char *proc_name, int size_mb);
void kill_sim_process(int pid);
void set_memory_limit(int mb);

// process.c (NEW - Process Management)
SimProcess* process_create(char *name, int priority);
void process_terminate(int pid);
SimProcess* process_find(int pid);
const char* process_state_str(ProcessState state);
void process_list();
void process_block(int pid);
void process_unblock(int pid);

// memory.c (NEW - Isolated Memory)
int mem_write(int pid, int offset, void *data, int size);
int mem_read(int pid, int offset, void *buffer, int size);
void mem_dump(int pid);
void mem_demo();

// scheduler.c (NEW - Round Robin Scheduler)
void scheduler_init();
void scheduler_tick();
void scheduler_run();
void scheduler_demo();

// ipc.c (NEW - Inter-Process Communication)
int ipc_send(int sender_pid, int receiver_pid, char *message);
int ipc_receive(int pid, char *buffer);
int ipc_check_mailbox(int pid);
void ipc_demo();

// utils.c
void show_scrollable_text(char *title, char *text);
void komut_calistir_ve_yazdir(char *baslik, char *komut, FILE *rapor_dosyasi);
void remove_ansi(char *str);
void system_wrapper(char *base_cmd, char *arg);

// ai.c
void ai_sor(char *soru);
void ai_command_helper_run(char *request);
bool is_whitelisted(char *cmd);
bool has_dangerous_chars(char *cmd);

// shell.c
void shell();
int satir_oku(char *tampon, int max_boyut);
void log_analizi_yap(char *dosya_adi);
void yardim_menusu();
void add_to_history(char *cmd);

// tui.c
bool show_login_screen();
void tui_menu();
void tui_draw_box(int selected);

#endif

