#include "globals.h"

DosyaBlogu sanal_disk[MAX_DOSYA];

void sfs_yukle() {
    FILE *dosya = fopen(DISK_DOSYASI, "rb");
    if (!dosya) {
        for(int i=0; i<MAX_DOSYA; i++) sanal_disk[i].dolu = false;
        return;
    }
    fread(sanal_disk, sizeof(DosyaBlogu), MAX_DOSYA, dosya);
    fclose(dosya);
}

void sfs_kaydet() {
    FILE *dosya = fopen(DISK_DOSYASI, "wb");
    if(dosya) {
        fwrite(sanal_disk, sizeof(DosyaBlogu), MAX_DOSYA, dosya);
        fclose(dosya);
    }
}

void sfs_dosya_olustur(char *isim, char *icerik) {
    for (int i=0; i<MAX_DOSYA; i++) {
        if (!sanal_disk[i].dolu) {
            // Use strncpy with bounds checking to prevent buffer overflow
            strncpy(sanal_disk[i].ad, isim, 31);
            sanal_disk[i].ad[31] = '\0';
            strncpy(sanal_disk[i].icerik, icerik, MAX_DOSYA_BOYUTU - 1);
            sanal_disk[i].icerik[MAX_DOSYA_BOYUTU - 1] = '\0';
            sanal_disk[i].boyut = strlen(sanal_disk[i].icerik);
            sanal_disk[i].dolu = true;
            sfs_kaydet();
            printf(RENK_YESIL "Virtual file created successfully.\n" RENK_SIFIRLA);
            return;
        }
    }
    printf(RENK_KIRMIZI "Error: Virtual Disk full!\n" RENK_SIFIRLA);
}

void sfs_liste() {
    printf("\n--- VIRTUAL DISK CONTENTS ---\n");
    int sayac = 0;
    for (int i=0; i<MAX_DOSYA; i++) {
        if (sanal_disk[i].dolu) {
             printf("%s (%d byte)\n", sanal_disk[i].ad, sanal_disk[i].boyut);
             sayac++;
        }
    }
    if(sayac == 0) printf("(Disk Empty)\n");
}
