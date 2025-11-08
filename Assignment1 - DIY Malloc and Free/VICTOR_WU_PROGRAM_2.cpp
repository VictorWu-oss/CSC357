#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <cctype>
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>
#include <ctime>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;
typedef unsigned char BYTE;
void *startofheap = NULL;

// Chunk is piece of allocated or free memory within the heap/pages
struct chunkinfo
{
    int size;               //size of the complete chunk including the chunkinfo
    int info;               //is it free? Flag: 0 = free, occupied = 1
    BYTE *next,*prev;       //address of next and prev chunk, doubly linked list of CHUNKS
};

chunkinfo* get_last_chunk() 
{
    if(!startofheap) //I have a global void *startofheap = NULL;
    return NULL;

    chunkinfo* ch = (chunkinfo*)startofheap;
    for (; ch->next; ch = (chunkinfo*)ch->next);
    return ch;
}

// Should print the address of the program break!
void analyze()
{
    printf("\n--------------------------------------------------------------\n");
    if(!startofheap)
    {
        // Added more text to match comparison
        printf("no heap, program break on address: %p\n", sbrk(0));
        return;
    }
    chunkinfo* ch = (chunkinfo*)startofheap;
    for (int no=0; ch; ch = (chunkinfo*)ch->next,no++)
    {
        printf("%d | current addr: %p |", no, (void*)ch);
        printf("size: %d | ", ch->size);
        printf("info: %d | ", ch->info);
        printf("next: %p | ", (void*)ch->next);
        printf("prev: %p", (void*)ch->prev);
        printf(" \n");
    }
    printf("program break on address: %p\n", (void*)sbrk(0));
}

// Allocates size bytes and returns pointer to beginning of allocated memory block
BYTE *mymalloc(int sizeRequested)
{
    // Use page size of 4096 bytes
    // Start with heapsize of 0
    // EX: needed is 16 bytes from struct + requested 1000 = 1016 bytes -> chunksize is 4096 bytes (1 page), OS WORKS IN A METRIC SYSTEM WITH VIRTUAL PAGES
    int pages = (sizeRequested + sizeof(chunkinfo) + 4095) / 4096; // Calculate number of pages needed
    int chunksize = pages * 4096;                                  // Round up to nearest multiple of 4096 bytes

    // Initialization first case
    // If there is no heap, create one chunk big enough for the requested size with brk/sbrk
    if(!startofheap)
    {
        // Create first chunk, increment program break by chunksize 
        startofheap = sbrk(chunksize);
        chunkinfo *first = (chunkinfo*) startofheap;
        first->size = chunksize;
        first->info = 1;    //occupied
        first->next = first->prev = NULL;
        return (BYTE*)first + sizeof(chunkinfo);
    }

     // If there are already chunks present, traverse through them as a list and check for a BEST FIT:
    chunkinfo *best = NULL;
    for (chunkinfo *ch = (chunkinfo*) startofheap; ch; ch = (chunkinfo*)ch->next)
    {
        // If current chunk is FREE && Fits the size
        if (ch->info == 0 && ch->size >= chunksize) 
        {
            // Update best fit to curr == smallest OR FREE chunk that can fit the requested size exactly or with minimal leftover space
            if (best == NULL || ch->size < best->size)
            {
                best = ch;
            }
        }
    }

    if (best != NULL) {
        // Remainder from how much unused space would remain after taking out the space user needs = carving out chunksize from best->size
        int remainder = best->size - chunksize;

        // Split if the leftover chunk is far bigger than the user-developer needs and the rest is at least or a page size bigger.
        if (remainder >= sizeof(chunkinfo) + 4096) 
        {
            // Create new chunk at the split location
            chunkinfo *newchunk = (chunkinfo*)((BYTE*)best + chunksize);
            newchunk->size = remainder;
            newchunk->info = 0;       // free
            newchunk->next = best->next;
            newchunk->prev = (BYTE*)best;

            // If there's a chunk after the new split chunk, update its prev pointer
            if (newchunk->next != NULL) {
                chunkinfo *afternew = (chunkinfo*)newchunk->next;
                afternew->prev = (BYTE*)newchunk;
            }

            // Update chunk
            best->size = chunksize;
            best->info = 1;          // occupied    
            best->next = (BYTE*)newchunk;

            return (BYTE*)best + sizeof(chunkinfo);
        } 

        else 
        {
            // Set to occ bc it fits but not enough space to split
            best->info = 1;
            return (BYTE*)best + sizeof(chunkinfo);
        }
    }


    // LAST CASE
    // If no chunk is free or fits, move the program break/ HEAP further: create a new chunk and link correctly
    // Create new chunk, increment program break by chunksize 

    // Find tail/ last element in order to link new chunk
    chunkinfo *tail = (chunkinfo*) startofheap;
    while (tail->next)
    {
        tail = (chunkinfo*)tail->next;
    }

    chunkinfo *newchunk = (chunkinfo*) sbrk(chunksize);
    newchunk->size = chunksize;
    newchunk->info = 1;         //occupied
    newchunk->next = NULL;
    newchunk->prev = (BYTE*)tail;

    // Update previous tail chunk to link to new chunk
    tail->next = (BYTE*)newchunk;
    return (BYTE*) ((BYTE*)newchunk + sizeof(chunkinfo));
}

// Free the pointer, Sets chunk to free, merge adjunct free chunks accordingly
// *addr starts at start of user data after the struct data, before the user bytes allocated
// [*ch] [Struct] [*addr (ch+sizeof(chunkinfo)] [Allocated Bytes] 
BYTE *myfree(BYTE *addr)
{
    // Undo the mymalloc to find ptr to the start of the struct
    chunkinfo *ch = (chunkinfo*)(addr - sizeof(chunkinfo));
    ch->info = 0;   //set to free

    // If next chunk exists and is free, Merge with now free chunk
    chunkinfo *nextch = (chunkinfo*) ch->next;
    if (nextch != NULL && (nextch)->info == 0)
    {
        // Add size of next chunk to current chunk and skip over next chunk (deletion)
        ch->size = ch->size + nextch->size;
        ch->next = nextch->next;

        if (nextch->next)
        {
            ((chunkinfo*)nextch->next)-> prev = (BYTE*)ch;
        }
    }

    // Check prev chunk exists and is free
    chunkinfo *prevch = (chunkinfo*) ch->prev;
    if (prevch != NULL && prevch->info == 0)
    {
        // Add sizes in prev node and skip over the current BC theyre combined now
        prevch->size = prevch->size + ch->size;
        prevch->next = ch->next;  

        if(ch->next)
        {
            ((chunkinfo *)ch->next)->prev = (BYTE*)prevch;
        }
        ch = prevch;
    }

    // If the chunk(or merged chunk) is the last one (next == NULL), you can remove the chunk completely and move the Program Break back for the size of the chunk
    chunkinfo *lastch = (chunkinfo*) get_last_chunk();
    if (lastch == ch && ch->info == 0)
    {
        // Check if last chunk's prev EXISTS if so point its next to NULL
        if(ch->prev != NULL)
        {
            ((chunkinfo*) ch-> prev)-> next = NULL;
        }
        else
        {
            // This was the only chunk, reset heap
            startofheap = NULL;
        }
        sbrk(-(ch->size));
    }

    return 0;
}

int main(){
    //testing part
    BYTE *a[100];
    analyze();          //50% points
    for(int i=0;i<100;i++)
    a[i] = mymalloc(1000);
    for(int i=0;i<90;i++)
    myfree(a[i]);
    analyze();          //50% of points if this is correct
    myfree(a[95]);
    a[95] = mymalloc(1000);
    analyze();          //25% points, this new chunk should fill the smaller free one

    //(best fit)
    for(int i=90;i<100;i++)
    myfree(a[i]);
    analyze();          // 25% should be an empty heap now with the start address
    //from the program start
    return 0;
}