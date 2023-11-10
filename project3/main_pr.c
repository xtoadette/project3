#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define VIRTUAL_ADDRESS_SIZE 65536
#define PAGE_SIZE 256
#define PAGE_TABLE_SIZE 256
#define TLB_SIZE 16
#define PHYSICAL_MEMORY_SIZE (PAGE_SIZE * 128) // 128 frames
#define BACKING_STORE_FILE "BACKING_STORE.bin"

int pageFaults = 0;
int tlbHits = 0;
int tlbPointer = 0; // for updateTLB function
int fifoPointer = 0; // for FIFO page replacement

// TLB data structure
typedef struct {
    int pageNumber; // Page number
    int frameNumber; // Frame number
    bool valid; // Valid bit to check if the entry is valid
} TLBEntry;

// Page table data structure
typedef struct {
    int frame; // Frame number
    bool valid; // Valid bit to check if the entry is valid
} PageTableEntry;

// Physical memory data structure
typedef struct {
    char data[PAGE_SIZE]; // Data stored in a page (256 bytes)
    int used;
} PhysicalMemoryPage;

// TLB (Array of TLBEntry)
TLBEntry TLB[TLB_SIZE];

// Page table (Array of PageTableEntry)
PageTableEntry pageTable[PAGE_TABLE_SIZE];

// Physical memory (Array of PhysicalMemoryPage)
PhysicalMemoryPage physicalMemory[PHYSICAL_MEMORY_SIZE];


// FIFO Queue for page replacement
// FIFO Queue for page replacement
int fifoQueue[256]; // Updated to match the physical memory size

// Initialize the FIFO queue
void initializeFIFOQueue() {
    for (int i = 0; i < 256; i++) {
        fifoQueue[i] = -1; // Initialize with -1 to indicate empty slots
    }
}

// Function to update the FIFO queue.
void updateFIFOQueue(int* pageQueue, int* queuePointer, int pageNumber) {
    pageQueue[*queuePointer] = pageNumber;

    // this is the portion that is giving us the SEG FAULT because
    // we are updating the queuePointer in multiple places
    // (*queuePointer)++;
}


int findOldestPage(int* pageQueue, int queuePointer) {
    int oldestPage = pageQueue[0];
    // Shift the elements in the queue to remove the oldest page.
    for (int i = 1; i < queuePointer; i++) {
        pageQueue[i - 1] = pageQueue[i];
    }
    return oldestPage;
}



// Function to initialize the TLB and FIFO queue.
void initializeTLB() {
    for (int i = 0; i < TLB_SIZE; i++) {
        TLB[i].valid = false;
        TLB[i].frameNumber = 0;
    }

    // this is the second place that is initializing the fifo queue!! NO GOOD
    // for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
    //     fifoQueue[i] = i;
    // }
}

// Function to check if a TLB entry exists for a page number.
bool isTLBHit(int pageNumber) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (TLB[i].valid && TLB[i].pageNumber == pageNumber) {
            return true;
        }
    }
    return false;
}

// Function to get a frame number from the TLB for a page number.
int getFrameFromTLB(int pageNumber) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if ( TLB[i].pageNumber == pageNumber ) {
            return TLB[i].frameNumber;
        }
    }
    return -1; // Not found in the TLB
}

// Function to update the TLB.
void updateTLB(int pageNumber, int frameNumber) {
    // printf("\tUpdating TBL w/ pageNumber %d tblPointer %d\n", pageNumber, tlbPointer);

    TLB[tlbPointer].pageNumber = pageNumber;
    TLB[tlbPointer].frameNumber = frameNumber;
    TLB[tlbPointer].valid = true;

    // Increment the TLB pointer and wrap around if needed.
    tlbPointer = (tlbPointer + 1) % TLB_SIZE;
}

void translateAddresses(int* logicalAddresses, int addressCount) {
    // Open files
    FILE* fp1 = fopen("out1.txt", "wt");
    FILE* fp2 = fopen("out2.txt", "wt");
    FILE* fp3 = fopen("out3.txt", "wt");
    FILE* backingStore = fopen(BACKING_STORE_FILE, "rb");

    // Initialize an array to keep track of the loaded pages in physical memory

    //   ****************************************************************
    // this array variable is not being used right here and the code works withought it

    // bool loadedPages[PAGE_TABLE_SIZE];
    // for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
    //     loadedPages[i] = false;
    // }
    //   ****************************************************************



    int pageQueue[PAGE_TABLE_SIZE]; // FIFO queue to track loaded pages
    int queuePointer = 0;           // Pointer to the front of the queue, tied to pageNumber
    int physicalAddress;            // not being used
    int max_address = 0;


    int logicalAddress;
    int pageNumber;     // always less than 256
    int offset;





    for (int i = 0; i < addressCount; i++) {

        logicalAddress = logicalAddresses[i];
        pageNumber = (logicalAddress >> 8) & 0xFF;
        offset = logicalAddress & 0xFF;


        // printf("pageNumber %d\toffset %d\n", pageNumber, offset);
        int frameNumber = -1;

        if (isTLBHit(pageNumber)) {
            // TLB hit: Get the frame number from the TLB.
            frameNumber = getFrameFromTLB(pageNumber);
            tlbHits++;
        } 
        else {
            // TLB miss: You'll need to consult the page table and handle page faults.
            // Implement the logic for page table lookup and page faults here.

            // Check if the page is in physical memory. If not, handle page fault.
            if (pageTable[pageNumber].valid) {
                frameNumber = pageTable[pageNumber].frame;
                // printf("I was here! for pageNumber %d\n", pageNumber);
            } 
            else {
                // Page fault: Find an available frame for the new page
                frameNumber = -1;
                //   ****************************************************************
                if (queuePointer < (256) ) {
                //   ****************************************************************


                    frameNumber = queuePointer;
                    queuePointer++;
                } else {
                    // Apply FIFO page replacement to find the oldest page to replace
                    printf("The FIFO getting called at i = %d\n", i);
                    int oldestPage = findOldestPage(pageQueue, queuePointer);
                    frameNumber = pageTable[oldestPage].frame;
                }

                pageQueue[queuePointer - 1] = pageNumber; // Add the new page to the queue
                fseek(backingStore, pageNumber * PAGE_SIZE, SEEK_SET);
                fread(physicalMemory[frameNumber].data, sizeof(char), PAGE_SIZE, backingStore);
                // printf("\tdata %s\n", physicalMemory[frameNumber].data);
                pageTable[pageNumber].valid = true;
                pageTable[pageNumber].frame = frameNumber;
                pageFaults++;
            }

            // Update the TLB with the new entry (updateTLB) if a page fault didn't occur.
            // printf("Updating TBL w/ pageNumber %d\n", pageNumber);
            updateTLB(pageNumber, frameNumber);
            // printf("tblPointer %d\n", tlbPointer);

            updateFIFOQueue(pageQueue, &queuePointer, pageNumber);

        }

        // Use the frame number and offset to access physical memory and retrieve the value.
        physicalAddress = frameNumber * PAGE_SIZE + offset;
        // printf("physicalAddress %d\n", physicalAddress);


        signed char value = physicalMemory[frameNumber].data[offset];
        physicalMemory[frameNumber].used = 1;

        // printf("\tmemory access @ physicalAddress %d\n", physicalAddress);

        if (pageNumber > max_address)
            max_address = pageNumber;

        // Save to files
        fprintf(fp1, "%d\n", logicalAddress);
        fprintf(fp2, "%d\n", physicalAddress);
        // printf("\tI got here for i = %d\t%d\n", i, value);

        fprintf(fp3, "%d\n", value);
        // printf("I got here for i = %d\t%d\n", i, value);

    }




    FILE* fp4 = fopen("random_file.txt", "wt");

    for (int i = 0; i < PHYSICAL_MEMORY_SIZE; i++){
        fprintf(fp4, "%d\n", physicalMemory[i].used );

    }







    // Close files
    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
    fclose(fp4);
    fclose(backingStore);

    printf("\n******* The greatest pageNumber was %d\n", max_address);
}

int main(int argc, char* argv[]) {
    char* filename = argv[1];
    int* logicalAddresses = NULL;
    int addressCount = 0;
    int capacity = 10;

    // Open file
    FILE* file = fopen(filename, "r");

    // Allocate array
    logicalAddresses = (int*)malloc(capacity * sizeof(int));

    // Read file
    int num;
    while (fscanf(file, "%d", &num) == 1) {
        // Reallocate if full
        if (addressCount == capacity) {
            capacity *= 2;
            logicalAddresses = (int*)realloc(logicalAddresses, capacity * sizeof(int));
            if (logicalAddresses == NULL) {
                perror("Memory reallocation error");
                return 1;
            }
        }
        logicalAddresses[addressCount] = num;
        addressCount++;
    }



    // Close file
    fclose(file);

    initializeTLB();

    // Initialize Page Table (if required).
    initializeFIFOQueue();

    // Translate logical addresses and retrieve values.
    translateAddresses(logicalAddresses, addressCount);

    // Print statistics
    printf("Page Faults: %d / %d\n", pageFaults, addressCount);
    printf("TLB Hits: %d / %d\n", tlbHits, addressCount);

    // Clean up resources.
    free(logicalAddresses);

    return 0;
}


