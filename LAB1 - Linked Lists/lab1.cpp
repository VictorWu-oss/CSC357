#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <cctype>
using namespace std;

// Every word node contains pter to prev and next WORD node, and the word text
struct WordNode {
    WordNode *prev, *next;
    std::string text;
};

// Every letter node contains pter to prev and next LETTER node, the letter itself and pters to the 
// head and tail of its own word linked list
struct LetterNode {
    LetterNode *prev, *next;
    char ch;
    WordNode *wordhead, *wordtail;
};


void new_word(const string& word, WordNode **head, WordNode **tail){
    WordNode *newel = new WordNode;

    // Initialize newel with null prev and next ptrs
    newel->prev = newel->next = NULL;

    // Assign the word to its text field
    newel->text = word;

    // Insert newel at the tail of the linked list, if head null insert there instead
    if(*head == NULL){
        *head = newel;
        *tail = newel;

    } else {
        (*tail)->next = newel;
        newel->prev = *tail;
        *tail = newel;
    }
}

int main(){
    // Initialize the 26 letter nodes

    LetterNode *prev = NULL;
    LetterNode *letterhead = NULL;

    // At first, I thought to use int, but char works too because both relate to the ASCII values
    for (char ch = 'a'; ch <= 'z'; ch++){
        LetterNode *newletter = new LetterNode;
        newletter->prev = newletter->next = NULL;
        newletter->wordhead = newletter->wordtail = NULL;

        // Increment letter
        newletter->ch = ch;

        if (ch == 'a'){
            // Initialize the head node
            letterhead = newletter;
            newletter->prev = NULL;
            newletter->next = NULL;

        } else {
            // Link the previous node to the current node
            newletter->prev = prev;
            prev->next = newletter;
        }

        if (ch == 'z'){
            // Initialize the tail node
            newletter->next = NULL;
        }

        prev = newletter;
    }

    string input;
    cout << "Enter words here (press enter to append word & type 'print' to finish):" << endl;

    // At first I thought to while input != "print", but this way is cleaner
    while (std::cin >> input) {
        if (input == "print"){
            break;
        }

        // Convert first character to lowercase value
        int first = std::tolower((unsigned char)input[0]);

        // Pter to letter list head, traverse to find matching letter node
        LetterNode *curr = letterhead;
        while (curr != NULL && curr->ch != first){
            curr = curr->next;
        }

        // Found letter to word
        // Add the word to the corresponding letter node's word linked list use that LETTER's node for wordhead and wordtail
        if (curr != NULL && curr->ch == first){
            new_word(input, &curr->wordhead, &curr->wordtail);
        }
    }

    // In each letter node from a-z, its eayzs to print now just traverse its word linked list to print and free memory
    LetterNode *letter = letterhead;
    while (letter != NULL) {
        WordNode *w = letter->wordhead;
        while (w != NULL) {
            cout << w->text << endl;
            std::cout<< " " << std::endl;
            WordNode *wordholder = w;
            w = w->next;
            delete wordholder;
        }
        LetterNode *letterholder = letter;
        letter = letter->next;
        delete letterholder;
    }

    return 0;
}

