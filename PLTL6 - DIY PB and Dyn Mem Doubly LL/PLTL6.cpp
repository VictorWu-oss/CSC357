#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

using namespace std;

#define PAGESIZE 4096
#define MEGABYTE 1000000
typedef unsigned char BYTE;

typedef struct listelem
{
    char text[100];
    listelem *prev, *next;
} listelem;

BYTE *mem = (BYTE *)malloc(MEGABYTE); // 1mb
listelem *startofheap = NULL;

void printList()
{
    for (listelem *curr = (listelem *)startofheap; curr; curr = curr->next)
        cout << curr->text << endl;
}

void insertListElem(char text[100], int list_num)
{
    listelem *curr_elem = (listelem *)startofheap;
    listelem *new_elem = (listelem *)mem;
    mem = mem + sizeof(listelem);

    int i = 0;
    for (i; i < list_num; i++)
    {
        if (curr_elem)
        curr_elem = curr_elem->next;
        
    }

    if (list_num == 0)
    {
        curr_elem->prev = new_elem;
        new_elem->next = curr_elem;
        new_elem->prev = startofheap;
        startofheap = new_elem;
    }
    else
    {
        new_elem->prev = curr_elem->prev;
        new_elem->next = curr_elem;
        curr_elem->prev->next = new_elem;
        curr_elem->prev = new_elem;
    }
    strcpy(new_elem->text, text);
}

void removeListElem(char text[100])
{
    listelem *cur = NULL;
    for (listelem *t = startofheap; t; t = t->next)
    {
        if (strcmp(t->text, text) == 0)
            cur = t;
    }
    listelem *A = cur->next;
    listelem *B = cur->prev;
    A->prev = B;
    B->next = A;
}


void swap(int i) 
{
    listelem *cur = startofheap;
    for (int k = 0; cur && k < i; k++, cur = cur->next)
        if (k == i - 1)
        {
            // B -> Current -> A -> C
            // Current's next node
            listelem *A = cur->next;

            // Current's previous node
            listelem *B = cur->prev;

            // Node after A
            listelem *C = A->next;

            // Swapping
            // B -> A -> Current -> C
            B->next = A;
            A->prev = B;
            A->next = cur;
            cur->prev = A;
            cur->next = C;
            if (C)
                C->prev = cur;
        }
}

int main()
{

    int list_num = 0;
    char text[100] = "\0";

    // Create header elem
    listelem *header_elem = (listelem *)mem;
    mem = mem + sizeof(listelem);
    startofheap = header_elem;
    header_elem->prev = header_elem->next = NULL;

    // creates the header
    cin >> text;
    strcpy(header_elem->text, text);

    while (strcmp(text, "exit") != 0)
    {
        // text for first header
        cin >> text;

        if (strcmp(text, "insert") == 0)
        {
            cin >> text;
            cin >> list_num;
            insertListElem(text, list_num);
        }
        else if (strcmp(text, "remove") == 0)
        {
            cin >> text;
            removeListElem(text);
        }
        else if (strcmp(text, "print") == 0)
        {
            printList();
        }
        else if (strcmp(text, "deleteall") == 0)
        {
            startofheap = NULL;
        }
        else if(strcmp(text, "swap") ==0)
        {
            cin >> list_num;
            swap(list_num);
        }
        else
        { // create new element
            listelem *new_elem = (listelem *)mem;
            listelem *prev_elem = (listelem *)(mem - sizeof(listelem));
            mem = mem + sizeof(listelem);

            prev_elem->next = new_elem;
            new_elem->prev = prev_elem;
            new_elem->next = NULL;

            strcpy(new_elem->text, text);
        }
        memset(text, '\0', sizeof(text));
    }
    cout << endl;
}