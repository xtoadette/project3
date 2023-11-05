#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Constants for memory sizes and parameters
#define VIRTUAL_ADDRESS_SIZE 65536
#define PAGE_SIZE 256
#define PAGE_TABLE_SIZE 256
#define TLB_SIZE 16
#define PHYSICAL_MEMORY_SIZE (PAGE_SIZE * PAGE_TABLE_SIZE)
#define BACKING_STORE_FILE "BACKING_STORE.bin"
#define MAX_ADDRESS_COUNT 1000

// Define structures for page table, TLB, and physical memory
typedef struct {
    int page;
    int frame;
} PageTableEntry;

typedef struct {
    int page;
    int frame;
} TLBEntry;

typedef struct {
    signed char data[PAGE_SIZE];
} Frame;

// Function to read a page from BACKING_STORE into physical memory
void loadPageToMemory(int page, Frame* memory) {
    FILE *backingStore = fopen(BACKING_STORE_FILE, "rb");
    if (backingStore == NULL) {
        fprintf(stderr, "Error opening BACKING_STORE.bin\n");
        exit(1);
    }

    fseek(backingStore, page * PAGE_SIZE, SEEK_SET);
    fread(memory->data, sizeof(signed char), PAGE_SIZE, backingStore);

    fclose(backingStore);
}

// Function to translate a logical address to a physical address
int translateAddress(int logicalAddress, PageTableEntry* pageTable, TLBEntry* tlb, Frame* memory) {
    int page = (logicalAddress >> 8) & 0xFF; // Extract page number
    int offset = logicalAddress & 0xFF; // Extract offset

    int frame = -1;

    // Check the TLB for a hit
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].page == page) {
            frame = tlb[i].frame;
            break;
        }
    }

    if (frame == -1) {
        // TLB miss, check the page table
        if (pageTable[page].page == page) {
            frame = pageTable[page].frame;
        } else {
            // Page fault, load the page from BACKING_STORE
            loadPageToMemory(page, &memory[PAGE_TABLE_SIZE]);
            frame = PAGE_TABLE_SIZE;

            // Update the page table
            pageTable[page].page = page;
            pageTable[page].frame = frame;
        }

        // Update the TLB (FIFO replacement strategy)
        for (int i = TLB_SIZE - 1; i > 0; i--) {
            tlb[i] = tlb[i - 1];
        }
        tlb[0].page = page;
        tlb[0].frame = frame;
    }

    return frame * PAGE_SIZE + offset;
}

int main(int argc, char *argv[]) {
    FILE *addressesFile = fopen("addresses.txt", "r");
    FILE *output1 = fopen("out1.txt", "w");
    FILE *output2 = fopen("out2.txt", "w");
    FILE *output3 = fopen("out3.txt", "w");

    PageTableEntry pageTable[PAGE_TABLE_SIZE];
    TLBEntry tlb[TLB_SIZE];
    Frame physicalMemory[PAGE_TABLE_SIZE];
    int pageTableSize = 0; // Keep track of the page table size

    // Initialize data structures
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        pageTable[i].page = -1; // Initialize page table entries to invalid
    }

    int pageFaultCount = 0;
    int tlbHitCount = 0;

    int logicalAddress;
    while (fscanf(addressesFile, "%d", &logicalAddress) != EOF) {
        int physicalAddress = translateAddress(logicalAddress, pageTable, tlb, physicalMemory);

        fprintf(output1, "%d\n", logicalAddress);
        fprintf(output2, "%d\n", physicalAddress);
        fprintf(output3, "%d\n", physicalMemory[physicalAddress / PAGE_SIZE].data[physicalAddress % PAGE_SIZE]);
    }

    // Calculate and print page-fault rate and TLB-hit rate
    double pageFaultRate = (double)pageFaultCount / MAX_ADDRESS_COUNT;
    double tlbHitRate = (double)tlbHitCount / MAX_ADDRESS_COUNT;
    printf("Page-fault rate: %.2f\n", pageFaultRate);
    printf("TLB hit rate: %.2f\n", tlbHitRate);

    fclose(addressesFile);
    fclose(output1);
    fclose(output2);
    fclose(output3);

    return 0;
}

