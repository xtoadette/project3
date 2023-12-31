#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define FRAME_SIZE 256
#define PAGE_SIZE 256
#define PAGE_TABLE_SIZE 256
#define TLB_SIZE 16
#define PHYSICAL_MEMORY_SIZE 65536
#define BACKING_STORE_FILE "BACKING_STORE.bin"

int pageFaults = 0;
int tlbHits = 0;
int tlbPointer = 0; //for updateTLB function

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
    char data[FRAME_SIZE]; // Data stored in a page (256 bytes)
} PhysicalMemoryPage;

// TLB (Array of TLBEntry)
TLBEntry TLB[TLB_SIZE];

// Page table (Array of PageTableEntry)
PageTableEntry pageTable[PAGE_TABLE_SIZE];

// Physical memory (Array of PhysicalMemoryPage)
PhysicalMemoryPage physicalMemory[PHYSICAL_MEMORY_SIZE];

//initialize the TLB
void initializeTLB()
{
    for (int i = 0; i < TLB_SIZE; i++)
    {
        TLB[i].valid = false;
    }
}

//check for tlb hit
bool isTLBHit(int pageNumber)
{
    for (int i = 0; i < TLB_SIZE; i++)
    {
            if (TLB[i].valid && TLB[i].pageNumber == pageNumber)
            {
                return true;
            }
        }
        return false;
}

// get a frame number from the TLB for a page number
int getFrameFromTLB(int pageNumber)
{
    for (int i = 0; i < TLB_SIZE; i++)
    {
            if (TLB[i].valid && TLB[i].pageNumber == pageNumber)
            {
                return TLB[i].frameNumber;
            }
        }
        return -1; // Not found in the TLB
}

//update the TLB
void updateTLB(int pageNumber, int frameNumber)
{
    // Implement TLB update using FIFO policy
        TLB[tlbPointer].pageNumber = pageNumber;
        TLB[tlbPointer].frameNumber = frameNumber;
        TLB[tlbPointer].valid = true;

        // Increment the TLB pointer and wrap around if needed.
        tlbPointer = (tlbPointer + 1) % TLB_SIZE;
}

//translate logical addresses to physical addresses and retrieve values
void translateAddresses(int* logicalAddresses, int addressCount)
{
    //open files
    FILE* fp1 = fopen("out1.txt", "wt");
    FILE* fp2 = fopen("out2.txt", "wt");
    FILE* fp3 = fopen("out3.txt", "wt");
    FILE* backingStore = fopen(BACKING_STORE_FILE, "rb");

    for (int i = 0; i < addressCount; i++) {
        int logicalAddress = logicalAddresses[i];

        // shifting the address by 8 to the right and masking the rightmost 8 bits
        int pageNumber = (logicalAddress >> 8) & 0xFF;
        int offset = logicalAddress & 0xFF;
        int frameNumber = -1;

        if (isTLBHit(pageNumber))
        {
            // TLB hit
            frameNumber = getFrameFromTLB(pageNumber);
            tlbHits++;
        }
        else
        {
            // TLB miss
            
            // Check if the page is in physical memory. If not, handle page fault.
            if (pageTable[pageNumber].valid)
            {
                frameNumber = pageTable[pageNumber].frame;
            }
            else
            {
                // Page fault: Read the page from BACKING_STORE.bin into physical memory.
                frameNumber = PAGE_TABLE_SIZE;
                fseek(backingStore, pageNumber * PAGE_SIZE, SEEK_SET);
                fread(physicalMemory[frameNumber].data, sizeof(char), PAGE_SIZE, backingStore);
                pageTable[pageNumber].valid = true;
                pageTable[pageNumber].frame = frameNumber;
                pageFaults++;
            }

            // Update the TLB with the new entry (updateTLB) if a page fault didn't occur
            updateTLB(pageNumber, frameNumber);
        }

        // Use the frame number and offset to access physical memory and retrieve the value.
        int physicalAddress = frameNumber * PAGE_SIZE + offset;
        signed char value = physicalMemory[frameNumber].data[offset];

        // save to files
        fprintf(fp1, "%d\n", logicalAddress);
        fprintf(fp2, "%d\n", physicalAddress);
        fprintf(fp3, "%d\n", value);
        
    }

    // close files
    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
    fclose(backingStore);
}

int main(int argc, char* argv[])
{
    char* filename = argv[1];
    int* logicalAddresses = NULL;
    int addressCount = 0;
    int capacity = 10;

    //Open file
    FILE *file = fopen(filename, "r");
    
    //Allocate array
    logicalAddresses = (int *)malloc(capacity * sizeof(int));
    
    //Read file
    int num;
    while(fscanf(file, "%d", &num) == 1)
    {
        //Reallocate if full
        if (addressCount == capacity)
        {
            capacity *= 2;
            logicalAddresses = (int *)realloc(logicalAddresses, capacity * sizeof(int));
            if (logicalAddresses == NULL)
            {
                perror("Memory reallocation error");
                return 1;
            }
        }
        logicalAddresses[addressCount] = num;
        addressCount++;
    }
    //Close file
    fclose(file);
    
    initializeTLB();

    // Initialize Page Table (if required).

    // Translate logical addresses and retrieve values.
    translateAddresses(logicalAddresses, addressCount);
    
    // Print statistics
    printf("Page Faults: %d / %d\n", pageFaults, addressCount);
    printf("TLB Hits: %d / %d\n", tlbHits, addressCount);

    // Clean up resources.
    free(logicalAddresses);
    
    return 0;
}


