#include "globals.h"

// =============================================================================
// ROUND ROBIN SCHEDULER (Döngüsel Zamanlayıcı)
// =============================================================================
// This module implements a Round-Robin CPU scheduler that:
// - Gives each process a time quantum
// - Switches between READY processes
// - Demonstrates OS scheduling concepts
// =============================================================================

int current_running_pid = -1;
int scheduler_tick_count = 0;

#define TIME_QUANTUM 3  // Each process gets 3 ticks

// Initialize the scheduler
void scheduler_init() {
    current_running_pid = -1;
    scheduler_tick_count = 0;
    printf(RENK_CYAN "[SCHEDULER] Initialized - Round Robin (Quantum: %d ticks)\n" RENK_SIFIRLA, TIME_QUANTUM);
}

// Find next ready process (Round Robin)
static int find_next_ready(int start) {
    for(int i = 0; i < MAX_PROCESS; i++) {
        int idx = (start + i) % MAX_PROCESS;
        if(process_table[idx].active && process_table[idx].state == PROC_READY) {
            return idx;
        }
    }
    return -1;
}

// Perform one scheduler tick (context switch if needed)
void scheduler_tick() {
    scheduler_tick_count++;
    
    // Find current running process
    SimProcess *current = NULL;
    int current_idx = -1;
    
    for(int i = 0; i < MAX_PROCESS; i++) {
        if(process_table[i].active && process_table[i].state == PROC_RUNNING) {
            current = &process_table[i];
            current_idx = i;
            current_running_pid = current->pid;
            break;
        }
    }
    
    // Decrease time slice of running process
    if(current) {
        current->time_slice--;
        
        if(current->time_slice <= 0) {
            // Time quantum expired - preempt!
            printf(RENK_SARI "[SCHED] Tick %d: %s (PID %d) quantum expired - PREEMPTED\n" RENK_SIFIRLA,
                   scheduler_tick_count, current->name, current->pid);
            current->state = PROC_READY;
            current->time_slice = TIME_QUANTUM;  // Reset for next turn
            
            // Find next ready process (starting after current)
            int next_idx = find_next_ready(current_idx + 1);
            if(next_idx >= 0) {
                process_table[next_idx].state = PROC_RUNNING;
                current_running_pid = process_table[next_idx].pid;
                printf(RENK_YESIL "[SCHED] Context switch → %s (PID %d)\n" RENK_SIFIRLA,
                       process_table[next_idx].name, process_table[next_idx].pid);
            } else {
                current_running_pid = -1;
            }
        } else {
            printf("[SCHED] Tick %d: %s (PID %d) running - %d ticks left\n",
                   scheduler_tick_count, current->name, current->pid, current->time_slice);
        }
    } else {
        // No process running - find one
        int next_idx = find_next_ready(0);
        if(next_idx >= 0) {
            process_table[next_idx].state = PROC_RUNNING;
            process_table[next_idx].time_slice = TIME_QUANTUM;
            current_running_pid = process_table[next_idx].pid;
            printf(RENK_YESIL "[SCHED] Tick %d: Starting %s (PID %d)\n" RENK_SIFIRLA,
                   scheduler_tick_count, process_table[next_idx].name, process_table[next_idx].pid);
        } else {
            printf("[SCHED] Tick %d: No ready processes (idle)\n", scheduler_tick_count);
        }
    }
}

// Run the scheduler for N ticks
void scheduler_run() {
    printf(RENK_CYAN "\n[SCHEDULER] Running for 15 ticks...\n" RENK_SIFIRLA);
    printf("Press Enter after each tick to continue...\n\n");
    
    for(int i = 0; i < 15; i++) {
        scheduler_tick();
        
        // Show brief status
        printf("    ");
        for(int j = 0; j < MAX_PROCESS; j++) {
            if(process_table[j].active) {
                const char *s = "";
                switch(process_table[j].state) {
                    case PROC_RUNNING: s = RENK_YESIL ">" RENK_SIFIRLA; break;
                    case PROC_READY:   s = RENK_CYAN "○" RENK_SIFIRLA; break;
                    case PROC_BLOCKED: s = RENK_SARI "■" RENK_SIFIRLA; break;
                    default: s = "×"; break;
                }
                printf("[%s %s] ", s, process_table[j].name);
            }
        }
        printf("\n");
        
        getchar();  // Wait for user
    }
}

// Demonstration of Round-Robin scheduling
void scheduler_demo() {
    printf(RENK_CYAN "\n");
    printf("+----------------------------------------------------------+\n");
    printf("|      ROUND ROBIN SCHEDULER DEMO (Döngüsel Zamanlayıcı)   |\n");
    printf("+----------------------------------------------------------+\n" RENK_SIFIRLA);
    
    printf("\n" RENK_SARI "[1] Initializing scheduler...\n" RENK_SIFIRLA);
    scheduler_init();
    
    printf("\n" RENK_SARI "[2] Creating test processes...\n" RENK_SIFIRLA);
    SimProcess *p1 = process_create("WebServer", 5);
    SimProcess *p2 = process_create("Database", 7);
    SimProcess *p3 = process_create("Logger", 3);
    
    if(!p1 || !p2 || !p3) {
        printf(RENK_KIRMIZI "Failed to create processes\n" RENK_SIFIRLA);
        return;
    }
    
    printf("\n" RENK_SARI "[3] Process states before scheduling:\n" RENK_SIFIRLA);
    process_list();
    
    printf("\n" RENK_SARI "[4] Running scheduler (15 ticks)...\n" RENK_SIFIRLA);
    printf("Legend: " RENK_YESIL ">" RENK_SIFIRLA "=Running  " 
           RENK_CYAN "○" RENK_SIFIRLA "=Ready  "
           RENK_SARI "■" RENK_SIFIRLA "=Blocked\n\n");
    
    // Run scheduler ticks
    for(int tick = 1; tick <= 15; tick++) {
        scheduler_tick();
        usleep(300000);  // 300ms delay for visualization
    }
    
    printf("\n" RENK_SARI "[5] Simulating I/O block on Database...\n" RENK_SIFIRLA);
    process_block(p2->pid);
    
    // Run a few more ticks
    for(int tick = 1; tick <= 5; tick++) {
        scheduler_tick();
        usleep(300000);
    }
    
    printf("\n" RENK_SARI "[6] I/O complete - unblocking Database...\n" RENK_SIFIRLA);
    process_unblock(p2->pid);
    
    // Final ticks
    for(int tick = 1; tick <= 5; tick++) {
        scheduler_tick();
        usleep(300000);
    }
    
    printf("\n" RENK_SARI "[7] Final process states:\n" RENK_SIFIRLA);
    process_list();
    
    // Cleanup
    process_terminate(p1->pid);
    process_terminate(p2->pid);
    process_terminate(p3->pid);
    
    printf("\n" RENK_YESIL "Scheduler demo complete.\n" RENK_SIFIRLA);
}
