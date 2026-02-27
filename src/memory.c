#include "globals.h"

// =============================================================================
// ISOLATED MEMORY MODULE (İzole Bellek Yönetimi)
// =============================================================================
// This module provides memory isolation between processes.
// Each process can ONLY access its own memory space.
// Attempts to access outside bounds trigger a "Segmentation Fault".
// =============================================================================

// Write data to a process's isolated memory (with bounds checking)
int mem_write(int pid, int offset, void *data, int size) {
    SimProcess *proc = process_find(pid);
    if(!proc) {
        printf(RENK_KIRMIZI "[MEM_ERROR] Process %d not found\n" RENK_SIFIRLA, pid);
        return -1;
    }
    
    // ISOLATION CHECK: Bounds verification
    if(offset < 0 || offset + size > proc->memory_limit) {
        printf(RENK_KIRMIZI "\n+------------------------------------------+\n");
        printf("|  ⚠️  SEGMENTATION FAULT (SIGSEGV)         |\n");
        printf("+------------------------------------------+\n");
        printf("|  Process: %-30s |\n", proc->name);
        printf("|  PID: %-34d |\n", pid);
        printf("|  Attempted: offset=%d, size=%d         |\n", offset, size);
        printf("|  Limit: %d bytes                       |\n", proc->memory_limit);
        printf("+------------------------------------------+\n" RENK_SIFIRLA);
        
        // Process gets terminated on segfault
        proc->state = PROC_TERMINATED;
        return -1;
    }
    
    // Write to isolated memory
    memcpy(proc->memory_space + offset, data, size);
    
    // Update memory usage tracking
    if(offset + size > proc->memory_used) {
        proc->memory_used = offset + size;
    }
    
    return 0;
}

// Read data from a process's isolated memory (with bounds checking)
int mem_read(int pid, int offset, void *buffer, int size) {
    SimProcess *proc = process_find(pid);
    if(!proc) {
        printf(RENK_KIRMIZI "[MEM_ERROR] Process %d not found\n" RENK_SIFIRLA, pid);
        return -1;
    }
    
    // ISOLATION CHECK: Bounds verification
    if(offset < 0 || offset + size > proc->memory_limit) {
        printf(RENK_KIRMIZI "\n+------------------------------------------+\n");
        printf("|  ⚠️  SEGMENTATION FAULT (SIGSEGV)         |\n");
        printf("+------------------------------------------+\n");
        printf("|  Process: %-30s |\n", proc->name);
        printf("|  PID: %-34d |\n", pid);
        printf("|  Read attempt: offset=%d, size=%d      |\n", offset, size);
        printf("|  Limit: %d bytes                       |\n", proc->memory_limit);
        printf("+------------------------------------------+\n" RENK_SIFIRLA);
        return -1;
    }
    
    // Read from isolated memory
    memcpy(buffer, proc->memory_space + offset, size);
    return 0;
}

// Dump memory contents of a process (hex view)
void mem_dump(int pid) {
    SimProcess *proc = process_find(pid);
    if(!proc) {
        printf(RENK_KIRMIZI "[MEM_ERROR] Process %d not found\n" RENK_SIFIRLA, pid);
        return;
    }
    
    printf(RENK_MAVI "\n+--------------------------------------------------------+\n");
    printf("|  MEMORY DUMP - Process: %-28s   |\n", proc->name);
    printf("|  PID: %d | Base: 0x%04X | Used: %d/%d bytes           |\n",
           proc->pid, proc->memory_base, proc->memory_used, proc->memory_limit);
    printf("+--------------------------------------------------------+\n" RENK_SIFIRLA);
    
    // Show first 128 bytes or used amount
    int show_bytes = (proc->memory_used > 0) ? proc->memory_used : 64;
    if(show_bytes > 128) show_bytes = 128;
    
    for(int i = 0; i < show_bytes; i += 16) {
        printf("  %04X: ", i);
        
        // Hex
        for(int j = 0; j < 16 && i+j < show_bytes; j++) {
            printf("%02X ", (unsigned char)proc->memory_space[i+j]);
        }
        
        // Padding
        for(int j = show_bytes - i; j < 16; j++) {
            printf("   ");
        }
        
        printf(" | ");
        
        // ASCII
        for(int j = 0; j < 16 && i+j < show_bytes; j++) {
            char c = proc->memory_space[i+j];
            printf("%c", (c >= 32 && c < 127) ? c : '.');
        }
        
        printf("\n");
    }
    
    printf(RENK_MAVI "+--------------------------------------------------------+\n" RENK_SIFIRLA);
}

// Demonstration of memory isolation
void mem_demo() {
    printf(RENK_CYAN "\n");
    printf("+----------------------------------------------------------+\n");
    printf("|         MEMORY ISOLATION DEMO (İzole Bellek Demo)        |\n");
    printf("+----------------------------------------------------------+\n" RENK_SIFIRLA);
    
    printf("\n" RENK_SARI "[1] Creating two isolated processes...\n" RENK_SIFIRLA);
    
    SimProcess *app1 = process_create("SecureApp", 5);
    SimProcess *app2 = process_create("OtherApp", 3);
    
    if(!app1 || !app2) {
        printf(RENK_KIRMIZI "Failed to create processes for demo\n" RENK_SIFIRLA);
        return;
    }
    
    printf("\n" RENK_SARI "[2] Writing SECRET data to SecureApp's memory...\n" RENK_SIFIRLA);
    char secret[] = "SECRET_PASSWORD_123";
    mem_write(app1->pid, 0, secret, strlen(secret) + 1);
    printf("    Wrote: \"%s\" to SecureApp\n", secret);
    
    printf("\n" RENK_SARI "[3] Writing normal data to OtherApp's memory...\n" RENK_SIFIRLA);
    char normal[] = "Hello from OtherApp";
    mem_write(app2->pid, 0, normal, strlen(normal) + 1);
    printf("    Wrote: \"%s\" to OtherApp\n", normal);
    
    printf("\n" RENK_SARI "[4] Each process reads its OWN memory:\n" RENK_SIFIRLA);
    char buffer[64];
    
    mem_read(app1->pid, 0, buffer, 64);
    printf("    SecureApp sees: \"%s\"\n", buffer);
    
    mem_read(app2->pid, 0, buffer, 64);
    printf("    OtherApp sees: \"%s\"\n", buffer);
    
    printf("\n" RENK_YESIL "✓ ISOLATION SUCCESS: Each process has its own memory!\n" RENK_SIFIRLA);
    printf("  OtherApp CANNOT see SecureApp's secret password.\n");
    
    printf("\n" RENK_SARI "[5] Attempting out-of-bounds access (attack simulation)...\n" RENK_SIFIRLA);
    printf("    OtherApp tries to read beyond its memory limit:\n");
    mem_read(app2->pid, 5000, buffer, 64);  // This should FAIL
    
    printf("\n" RENK_SARI "[6] Memory dumps:\n" RENK_SIFIRLA);
    mem_dump(app1->pid);
    mem_dump(app2->pid);
    
    // Cleanup
    process_terminate(app1->pid);
    process_terminate(app2->pid);
    
    printf("\n" RENK_YESIL "Demo complete. Processes terminated.\n" RENK_SIFIRLA);
}
