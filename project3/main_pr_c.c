#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NUM_FRAMES 128                   // change number of required frames here
#define PAGE_SIZE 256
#define PAGE_TABLE_SIZE 256
#define TLB_SIZE 16
#define PHYSICAL_MEMORY_SIZE 65536
#define BACKING_STORE_FILE "BACKING_STORE.bin"

int pageFaults = 0;
int tlbHits = 0;
int tlbPointer = 0; // for updateTLB function
int fifoPointer = 0; // for FIFO page replacement

// TLB data structure
typedef struct {
    int pageNumber; // Page number
    int frameNumber; // Frame number
    bool valid; // Valid bit
} TLBEntry;

// Page table data structure
typedef struct {
    int frameNumber; // Frame number
    bool valid; // Valid bit
} PageTableEntry;

// Physical memory data structure
typedef struct {
    char data[ PAGE_SIZE ];
    // int used;
} PhysicalMemoryPage;

//*******************************
//custom structures to make a queue

typedef struct node {
    int value;
    struct node *next;
} node;

typedef struct{
    node *head;
    node *tail;
} queue;
//*******************************

// TLB (Array of TLBEntry)
TLBEntry TLB[ TLB_SIZE ];

// Page table (Array of PageTableEntry)
PageTableEntry pageTable[ NUM_FRAMES ] ;

// Physical memory (Array of PhysicalMemoryPage)
PhysicalMemoryPage physicalMemory[ NUM_FRAMES ];        // physical memory is the number of frames itself

// FIFO Queue for page replacement
int fifoQueue[PAGE_SIZE];

// Initialize fifo queue
void initializeFIFOQueue() {
    for (int i = 0; i < PAGE_SIZE; i++) {
        fifoQueue[i] = -1;
    }
}

//initialize queue
void init_q(queue *q) {
    q -> head = NULL;
    q -> tail = NULL;
}

//enqueue
void en_q (queue *q, int value){
    // new node is created and initialized with the given value
    node * new_n = malloc(sizeof(node));
    new_n -> value = value;
    new_n -> next = NULL;

    // the node is attached at the end of the queue
    if (q -> tail != NULL)
        q -> tail -> next = new_n;

    q -> tail = new_n;

    // checking if the head was not already asigned
    if (q -> head == NULL)
        q -> head = new_n;
}


//dequeue
int de_q (queue *q){

    node *tmp = q -> head;

    int result = tmp -> value;

    q -> head = q -> head -> next;

    if (q -> head == NULL)
        q -> tail = NULL;

    free (tmp);
    return result;
}

//update fifo queue
void updateFIFOQueue(int* pageQueue, int* queuePointer, int pageNumber) {
    pageQueue[*queuePointer] = pageNumber;
}

// initialize tlb and page table
void initializeTables() {
    // initializing the TLB
    for (int i = 0; i < TLB_SIZE; i++) {
        
        TLB[i].frameNumber = 0;
    }

    // initializing the page table
    for (int i = 0; i < NUM_FRAMES; i++) {
        pageTable[i].valid = false;
        pageTable[i].frameNumber = 0;
    }
}

//check for tlb hits
bool isTLBHit(int pageNumber) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (TLB[i].valid && TLB[i].pageNumber == pageNumber) {
            return true;
        }
    }
    return false;
}

// get the frame from the tlb
int getFrameFromTLB(int pageNumber) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if ( TLB[i].pageNumber == pageNumber ) {
            return TLB[i].frameNumber;
        }
    }
    return -1; // Not found in the TLB
}

//update the tlb
void updateTLB(int pageNumber, int frameNumber) {

    TLB[tlbPointer].pageNumber = pageNumber;
    TLB[tlbPointer].frameNumber = frameNumber;
    TLB[tlbPointer].valid = true;

    // Increment the TLB pointer and wrap around if needed
    tlbPointer = (tlbPointer + 1) % TLB_SIZE;
}

void translateAddresses(int* logicalAddresses, int addressCount, queue *fifoQueue) {
    // Open files
    FILE* fp1 = fopen("out1.txt", "wt");
    FILE* fp2 = fopen("out2.txt", "wt");
    FILE* fp3 = fopen("out3.txt", "wt");
    FILE* backingStore = fopen(BACKING_STORE_FILE, "rb");

    
    bool loadedPages[PAGE_SIZE];
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        loadedPages[i] = false;
    }

    // more variables
    int pageQueue[PAGE_TABLE_SIZE]; // FIFO queue to track loaded pages
    int queuePointer = 0;           // Pointer to the front of the queue, tied to pageNumber
    int physicalAddress;
    int logicalAddress;
    int pageNumber;     // always less than 256
    int offset;
    int oldestPage;

    //start looping through addresses
    for (int k = 0; k < addressCount; k++) {

        //pull address and find page and offset
        logicalAddress = logicalAddresses[k];
        pageNumber = (logicalAddress >> 8) & 0xFF;
        offset = logicalAddress & 0xFF;

        //resets frame number
        int frameNumber = -1;

        //checks for tlb hit
        if (isTLBHit(pageNumber)){
            frameNumber = getFrameFromTLB(pageNumber);
            tlbHits++;
        } 
        else{
            // Check if the page is in physical memory. If not, handle page fault
            if (pageTable[pageNumber].valid) {
                frameNumber = pageTable[pageNumber].frameNumber;
            } 
            else {
                pageFaults++;

                //Find an available frame for the new page
                if (pageTable[pageNumber].valid == false){
                    frameNumber = k % NUM_FRAMES;
                    pageTable[pageNumber].frameNumber = frameNumber;
                    pageTable[pageNumber].valid = true;

                    //add to fifoQueue
                    en_q(fifoQueue, frameNumber);
                }
                else {
                    // Apply FIFO page replacement to find the oldest page to replace
                    oldestPage = de_q(fifoQueue);
                    en_q(fifoQueue, oldestPage);

                    frameNumber = pageTable[oldestPage].frameNumber;
                }
                //read from backing store
                fseek(backingStore, pageNumber * PAGE_SIZE, SEEK_SET);
                fread(physicalMemory[frameNumber].data, sizeof(char), PAGE_SIZE, backingStore);
                
                pageTable[pageNumber].frameNumber = frameNumber;
            }

            // Update the TLB with the new entry (updateTLB) if a page fault didn't occur
            updateTLB(pageNumber, frameNumber);
        }

        // Use the frame number and offset to access physical memory and retrieve the value
        physicalAddress = frameNumber * PAGE_SIZE + offset;
        signed char value = physicalMemory[frameNumber].data[offset];

        // Save to files
        fprintf(fp1, "%d\n", logicalAddress);
        fprintf(fp2, "%d\n", physicalAddress);
        fprintf(fp3, "%d\n", value);
    }

    // Close files
    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
    fclose(backingStore);
}

int main(int argc, char* argv[]) {
    char* filename = argv[1];
    int* logicalAddresses = NULL;
    int addressCount = 0;
    int capacity = 10;

    // Open file
    FILE* file = fopen(filename, "r");

    // Allocate array and queue
    logicalAddresses = (int*)malloc(capacity * sizeof(int));
    queue oldest;


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

    initializeTables();

    // Initialize Page Table (if required).
    initializeFIFOQueue();
    init_q(&oldest);

    // Translate logical addresses and retrieve values.
    translateAddresses(logicalAddresses, addressCount, &oldest);

    double pageFaultRatio = (pageFaults / (double)addressCount);
    double tlbHitRatio = (tlbHits / (double)addressCount);
    // Print statistics
    printf("Page Faults: %d / %d, %0.3f\n", pageFaults, addressCount, pageFaultRatio);
    printf("TLB Hits: %d / %d, %0.3f\n", tlbHits, addressCount, tlbHitRatio);

    // Clean up resources
    free(logicalAddresses);

    return 0;
}


