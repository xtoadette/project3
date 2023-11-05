#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define VIRTUAL_ADDRESS_SIZE 65536
#define PAGE_SIZE 256
#define PAGE_TABLE_SIZE 256
#define TLB_SIZE 16
#define PHYSICAL_MEMORY_SIZE (PAGE_SIZE * PAGE_TABLE_SIZE)
#define BACKING_STORE_FILE "BACKING_STORE.bin"

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
} PhysicalMemoryPage;

// TLB (Array of TLBEntry)
TLBEntry TLB[TLB_SIZE];

// Page table (Array of PageTableEntry)
PageTableEntry pageTable[PAGE_TABLE_SIZE];

// Physical memory (Array of PhysicalMemoryPage)
PhysicalMemoryPage physicalMemory[PHYSICAL_MEMORY_SIZE];


int tlbPointer = 0; //for updateTLB function

// Function to initialize the TLB.
void initializeTLB() 
{
    for (int i = 0; i < TLB_SIZE; i++)
    {
        TLB[i].valid = false;
    }
}

// Function to check if a TLB entry exists for a page number.
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

// Function to get a frame number from the TLB for a page number.
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

// Function to update the TLB.
void updateTLB(int pageNumber, int frameNumber)
{
    // Implement TLB update using FIFO policy.
        TLB[tlbPointer].pageNumber = pageNumber;
        TLB[tlbPointer].frameNumber = frameNumber;
        TLB[tlbPointer].valid = true;

        // Increment the TLB pointer and wrap around if needed.
        tlbPointer = (tlbPointer + 1) % TLB_SIZE;
}

// Function to translate logical addresses to physical addresses and retrieve values.
void translateAddresses(int* logicalAddresses, int addressCount)
{
    // Open the BACKING_STORE.bin file.
    FILE* backingStore = fopen(BACKING_STORE_FILE, "rb");
    if (backingStore == NULL) {
        perror("Error opening BACKING_STORE.bin");
        return;
    }

    for (int i = 0; i < addressCount; i++) {
        int logicalAddress = logicalAddresses[i];
        int pageNumber = (logicalAddress >> 8) & 0xFF;
        int offset = logicalAddress & 0xFF;
        int frameNumber = -1;

        if (isTLBHit(pageNumber)) {
            // TLB hit: Get the frame number from the TLB.
            frameNumber = getFrameFromTLB(pageNumber);
        } else {
            // TLB miss: You'll need to consult the page table and handle page faults.
            // Implement the logic for page table lookup and page faults here.
            
            // Check if the page is in physical memory. If not, handle page fault.
            if (pageTable[pageNumber].valid) {
                frameNumber = pageTable[pageNumber].frame;
            } else {
                // Page fault: Read the page from BACKING_STORE.bin into physical memory.
                frameNumber = PAGE_TABLE_SIZE; // The page table size serves as the next available frame.
                fseek(backingStore, pageNumber * PAGE_SIZE, SEEK_SET);
                fread(physicalMemory[frameNumber].data, sizeof(char), PAGE_SIZE, backingStore);
                pageTable[pageNumber].valid = true;
                pageTable[pageNumber].frame = frameNumber;
            }

            // Update the TLB with the new entry (updateTLB) if a page fault didn't occur.
            updateTLB(pageNumber, frameNumber);
        }

        // Use the frame number and offset to access physical memory and retrieve the value.
        int physicalAddress = frameNumber * PAGE_SIZE + offset;
        signed char value = physicalMemory[frameNumber].data[offset];

        // Print the results (logical address, physical address, and retrieved value).
        printf("Logical Address: %d, Physical Address: %d, Value: %d\n", logicalAddress, physicalAddress, value);
    }

    // Close BACKING_STORE.bin.
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
    if (file == NULL)
    {
        perror("Error opening the file");
        return 1;
    }
    
    //Allocate array
    logicalAddresses = (int *)malloc(capacity * sizeof(int));

    if (logicalAddresses == NULL) 
    {
            perror("Memory allocation error");
            return 1;
    }
    
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

        // Clean up resources.
        free(logicalAddresses);
    
    return 0;
}

