#include "globals.h"

// =============================================================================
// PROCESS MANAGEMENT MODULE (Süreç Yönetimi)
// =============================================================================
// This module provides process creation, termination, and state management
// with ISOLATED memory spaces for each process.
// =============================================================================

static int pid_counter = 1000;

// Convert process state to string for display
const char* process_state_str(ProcessState state) {
    switch(state) {
        case PROC_NEW:        return "NEW";
        case PROC_READY:      return "READY";
        case PROC_RUNNING:    return "RUNNING";
        case PROC_BLOCKED:    return "BLOCKED";
        case PROC_TERMINATED: return "TERMINATED";
        default:              return "UNKNOWN";
    }
}

// Find a process by PID
SimProcess* process_find(int pid) {
    for(int i = 0; i < MAX_PROCESS; i++) {
        if(process_table[i].active && process_table[i].pid == pid) {
            return &process_table[i];
        }
    }
    return NULL;
}

// Create a new process with isolated memory
SimProcess* process_create(char *name, int priority) {
    // Find empty slot
    for(int i = 0; i < MAX_PROCESS; i++) {
        if(!process_table[i].active) {
            SimProcess *proc = &process_table[i];
            
            // Initialize process
            proc->pid = pid_counter++;
            strncpy(proc->name, name, 31);
            proc->name[31] = '\0';
            proc->state = PROC_READY;
            proc->priority = (priority < 0) ? 0 : (priority > 9) ? 9 : priority;
            proc->time_slice = 3;  // 3 time units per quantum
            
            // ISOLATED MEMORY - Initialize with zeros
            proc->memory_base = i * PROCESS_MEMORY_SIZE;  // Virtual address
            proc->memory_limit = PROCESS_MEMORY_SIZE;
            memset(proc->memory_space, 0, PROCESS_MEMORY_SIZE);
            proc->memory_used = 0;
            
            // Clear IPC mailbox
            proc->mailbox_count = 0;
            for(int j = 0; j < MAX_IPC_MESSAGES; j++) {
                proc->mailbox[j].read = true;
            }
            
            proc->active = true;
            
            printf(RENK_YESIL "[PROCESS] Created: %s (PID: %d, Priority: %d)\n" RENK_SIFIRLA,
                   proc->name, proc->pid, proc->priority);
            printf("          Memory: Base=0x%04X, Limit=%d bytes\n",
                   proc->memory_base, proc->memory_limit);
            
            return proc;
        }
    }
    
    printf(RENK_KIRMIZI "[ERROR] Process table full! Cannot create: %s\n" RENK_SIFIRLA, name);
    return NULL;
}

// Terminate a process and free its resources
void process_terminate(int pid) {
    SimProcess *proc = process_find(pid);
    if(!proc) {
        printf(RENK_KIRMIZI "[ERROR] Process %d not found\n" RENK_SIFIRLA, pid);
        return;
    }
    
    proc->state = PROC_TERMINATED;
    proc->active = false;
    
    // Clear isolated memory (security)
    memset(proc->memory_space, 0, PROCESS_MEMORY_SIZE);
    
    printf(RENK_SARI "[PROCESS] Terminated: %s (PID: %d)\n" RENK_SIFIRLA,
           proc->name, pid);
}

// Block a process (e.g., waiting for I/O)
void process_block(int pid) {
    SimProcess *proc = process_find(pid);
    if(!proc) {
        printf(RENK_KIRMIZI "[ERROR] Process %d not found\n" RENK_SIFIRLA, pid);
        return;
    }
    
    if(proc->state == PROC_RUNNING || proc->state == PROC_READY) {
        proc->state = PROC_BLOCKED;
        printf(RENK_SARI "[PROCESS] Blocked: %s (PID: %d) - Waiting for I/O\n" RENK_SIFIRLA,
               proc->name, pid);
    }
}

// Unblock a process (I/O complete)
void process_unblock(int pid) {
    SimProcess *proc = process_find(pid);
    if(!proc) {
        printf(RENK_KIRMIZI "[ERROR] Process %d not found\n" RENK_SIFIRLA, pid);
        return;
    }
    
    if(proc->state == PROC_BLOCKED) {
        proc->state = PROC_READY;
        printf(RENK_YESIL "[PROCESS] Unblocked: %s (PID: %d) - Ready to run\n" RENK_SIFIRLA,
               proc->name, pid);
    }
}

// List all processes with their states
void process_list() {
    printf(RENK_MAVI "\n+------------------------------------------------------------+\n");
    printf("|              PROCESS TABLE (Süreç Tablosu)                 |\n");
    printf("+------╦----------------╦----------╦------╦-----------------+\n");
    printf("| PID  | Name           | State    | Prio | Memory          |\n");
    printf("+------╬----------------╬----------╬------╬-----------------+\n" RENK_SIFIRLA);
    
    int count = 0;
    for(int i = 0; i < MAX_PROCESS; i++) {
        if(process_table[i].active) {
            SimProcess *p = &process_table[i];
            
            // Color based on state
            const char *color = RENK_SIFIRLA;
            switch(p->state) {
                case PROC_RUNNING: color = RENK_YESIL; break;
                case PROC_READY:   color = RENK_CYAN; break;
                case PROC_BLOCKED: color = RENK_SARI; break;
                default: break;
            }
            
            printf("| %4d | %-14s | %s%-8s%s |  %d   | %4d/%4d bytes |\n",
                   p->pid, p->name, color, process_state_str(p->state), RENK_SIFIRLA,
                   p->priority, p->memory_used, p->memory_limit);
            count++;
        }
    }
    
    if(count == 0) {
        printf("|              (No active processes)                         |\n");
    }
    
    printf(RENK_MAVI "+------╩----------------╩----------╩------╩-----------------+\n" RENK_SIFIRLA);
    printf("Total: %d active processes\n", count);
}
