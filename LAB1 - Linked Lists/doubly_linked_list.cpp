// doubly_linked_list.cpp
// Simple doubly linked list with int values
// build: g++ -std=c++17 doubly_linked_list.cpp -o doubly_linked_list

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
using namespace std;

// Node contains pointers to prev and next nodes, and an int value
struct Node {
    Node *prev, *next;
    int value;
};

// Append a new node with given value to the end of the list
void append(int val, Node **head, Node **tail) {
    Node *newnode = new Node;
    
    // Initialize newnode with null prev and next pointers
    newnode->prev = newnode->next = NULL;
    
    // Assign the value
    newnode->value = val;
    
    // Insert newnode at the tail of the linked list, if head null insert there instead
    if (*head == NULL) {
        *head = newnode;
        *tail = newnode;
    } else {
        (*tail)->next = newnode;
        newnode->prev = *tail;
        *tail = newnode;
    }
}

// Remove first occurrence of a node with given value
void remove_element(int val, Node **head, Node **tail) {
    Node *curr = *head;
    
    // Traverse to find the node with matching value
    while (curr != NULL && curr->value != val) {
        curr = curr->next;
    }
    
    // If found, remove it
    if (curr != NULL) {
        if (curr->prev != NULL) {
            curr->prev->next = curr->next;
        } else {
            *head = curr->next;
        }
        
        if (curr->next != NULL) {
            curr->next->prev = curr->prev;
        } else {
            *tail = curr->prev;
        }
        
        delete curr;
    }
}

// Delete entire list and free all memory
void delete_list(Node **head, Node **tail) {
    Node *curr = *head;
    
    while (curr != NULL) {
        Node *holder = curr;
        curr = curr->next;
        delete holder;
    }
    
    *head = NULL;
    *tail = NULL;
}

int main() {
    Node *head = NULL;
    Node *tail = NULL;
    
    // Append some values
    append(10, &head, &tail);
    append(20, &head, &tail);
    append(30, &head, &tail);
    append(40, &head, &tail);
    
    // Remove an element
    remove_element(20, &head, &tail);
    
    // Delete entire list
    delete_list(&head, &tail);
    
    return 0;
}
