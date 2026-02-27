#include "globals.h"

// =============================================================================
// INTER-PROCESS COMMUNICATION (Süreçler Arası İletişim - IPC)
// =============================================================================
// This module provides message passing between processes.
// Each process has a mailbox where other processes can send messages.
// =============================================================================

// Send a message to another process
int ipc_send(int sender_pid, int receiver_pid, char *message) {
    SimProcess *sender = process_find(sender_pid);
    SimProcess *receiver = process_find(receiver_pid);
    
    if(!sender) {
        printf(RENK_KIRMIZI "[IPC] Error: Sender process %d not found\n" RENK_SIFIRLA, sender_pid);
        return -1;
    }
    
    if(!receiver) {
        printf(RENK_KIRMIZI "[IPC] Error: Receiver process %d not found\n" RENK_SIFIRLA, receiver_pid);
        return -1;
    }
    
    // Find empty slot in receiver's mailbox
    for(int i = 0; i < MAX_IPC_MESSAGES; i++) {
        if(receiver->mailbox[i].read) {
            receiver->mailbox[i].sender_pid = sender_pid;
            receiver->mailbox[i].receiver_pid = receiver_pid;
            strncpy(receiver->mailbox[i].content, message, MAX_IPC_MSG_SIZE - 1);
            receiver->mailbox[i].content[MAX_IPC_MSG_SIZE - 1] = '\0';
            receiver->mailbox[i].read = false;
            receiver->mailbox_count++;
            
            printf(RENK_CYAN "[IPC] Message sent: %s → %s\n" RENK_SIFIRLA,
                   sender->name, receiver->name);
            printf("      Content: \"%s\"\n", message);
            
            // If receiver is blocked waiting for message, unblock it
            if(receiver->state == PROC_BLOCKED) {
                receiver->state = PROC_READY;
                printf(RENK_YESIL "[IPC] Process %s unblocked (message received)\n" RENK_SIFIRLA,
                       receiver->name);
            }
            
            return 0;
        }
    }
    
    printf(RENK_KIRMIZI "[IPC] Error: %s's mailbox is full!\n" RENK_SIFIRLA, receiver->name);
    return -1;
}

// Receive a message (returns 0 if message received, -1 if none)
int ipc_receive(int pid, char *buffer) {
    SimProcess *proc = process_find(pid);
    if(!proc) {
        printf(RENK_KIRMIZI "[IPC] Error: Process %d not found\n" RENK_SIFIRLA, pid);
        return -1;
    }
    
    // Find first unread message
    for(int i = 0; i < MAX_IPC_MESSAGES; i++) {
        if(!proc->mailbox[i].read) {
            strcpy(buffer, proc->mailbox[i].content);
            proc->mailbox[i].read = true;
            proc->mailbox_count--;
            
            SimProcess *sender = process_find(proc->mailbox[i].sender_pid);
            printf(RENK_CYAN "[IPC] %s received message from %s\n" RENK_SIFIRLA,
                   proc->name, sender ? sender->name : "Unknown");
            printf("      Content: \"%s\"\n", buffer);
            
            return 0;
        }
    }
    
    // No messages - process might block
    printf(RENK_SARI "[IPC] No messages for %s (mailbox empty)\n" RENK_SIFIRLA, proc->name);
    return -1;
}

// Check mailbox status
int ipc_check_mailbox(int pid) {
    SimProcess *proc = process_find(pid);
    if(!proc) return -1;
    
    int unread = 0;
    for(int i = 0; i < MAX_IPC_MESSAGES; i++) {
        if(!proc->mailbox[i].read) unread++;
    }
    
    return unread;
}

// Demonstration of IPC
void ipc_demo() {
    printf(RENK_CYAN "\n");
    printf("+----------------------------------------------------------+\n");
    printf("|      IPC DEMO (Süreçler Arası İletişim Demosu)           |\n");
    printf("+----------------------------------------------------------+\n" RENK_SIFIRLA);
    
    printf("\n" RENK_SARI "[1] Creating processes for IPC demo...\n" RENK_SIFIRLA);
    SimProcess *client = process_create("Client", 5);
    SimProcess *server = process_create("Server", 8);
    
    if(!client || !server) {
        printf(RENK_KIRMIZI "Failed to create processes\n" RENK_SIFIRLA);
        return;
    }
    
    printf("\n" RENK_SARI "[2] Client sends request to Server...\n" RENK_SIFIRLA);
    ipc_send(client->pid, server->pid, "REQUEST: Get user data");
    
    printf("\n" RENK_SARI "[3] Server checks mailbox...\n" RENK_SIFIRLA);
    int pending = ipc_check_mailbox(server->pid);
    printf("    Server has %d pending message(s)\n", pending);
    
    printf("\n" RENK_SARI "[4] Server reads message...\n" RENK_SIFIRLA);
    char buffer[MAX_IPC_MSG_SIZE];
    ipc_receive(server->pid, buffer);
    
    printf("\n" RENK_SARI "[5] Server processes request and responds...\n" RENK_SIFIRLA);
    ipc_send(server->pid, client->pid, "RESPONSE: {user: 'admin', status: 'ok'}");
    
    printf("\n" RENK_SARI "[6] Client receives response...\n" RENK_SIFIRLA);
    ipc_receive(client->pid, buffer);
    
    printf("\n" RENK_SARI "[7] Simulating multiple messages...\n" RENK_SIFIRLA);
    ipc_send(client->pid, server->pid, "REQUEST: Get logs");
    ipc_send(client->pid, server->pid, "REQUEST: Get stats");
    ipc_send(client->pid, server->pid, "REQUEST: Get config");
    
    printf("\n    Server mailbox status: %d messages\n", ipc_check_mailbox(server->pid));
    
    printf("\n" RENK_SARI "[8] Server processes all messages...\n" RENK_SIFIRLA);
    while(ipc_receive(server->pid, buffer) == 0) {
        printf("    Processing: %s\n", buffer);
        usleep(200000);
    }
    
    // Cleanup
    process_terminate(client->pid);
    process_terminate(server->pid);
    
    printf("\n" RENK_YESIL "IPC demo complete.\n" RENK_SIFIRLA);
}
