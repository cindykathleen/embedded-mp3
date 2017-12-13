#pragma once
#include "common.hpp"

/**
 * TODO : Convert this to a statically allocated linked list
 * This class simulates a circular buffer in which elements are rotated from the front to the back
 * and elements can be randomly inserted or removed.
 * However, each element is dynamically allocated and may be prone to its limitations in
 * embedded environments.
*/

class CircularBuffer
{
public:

    // Constructor
    CircularBuffer();

    // Destructor, frees the list
    ~CircularBuffer();

    // @description   : Inserts an element at the head of the list
    // @param element : The value of the element to be inserted
    void InsertFront(char* element);

    // @description   : Inserts an element at the tail of the list
    // @param element : The value of the element to be inserted
    void InsertBack(char *element);

    // @description   : Pops an element from the head of the list
    // @returns       : The value of the element popped, or NULL
    char* PopFront();

    // @description   : Pops an element from the tail of the list
    // @returns       : The value of the element popped, or NULL
    char* PopBack();

    // @description   : Tries to find an element to pop from the list matching the input
    // @element       : The element to search for
    // @returns       : The value of the element popped, or NULL
    char* PopByName(char* element);

    // @description   : Rotate the head node to the tail node
    void RotateForward();

    // @description   : Rotate the tail node to the head node
    void RotateBackward();

    // @description   : A simple shuffle algorithm using Fisher-Yates / Knuths
    void ShuffleList();

    // @description   : Gets the head
    // @returns       : The value of the head element
    char* GetHead();

    // @description   : Get the current size
    // @returns       : The size
    uint16_t GetBufferSize();

    // @description   : Iterates through the list and prints their elements
    void PrintBuffer();

private:

    // A generic linked doubly linked list node
    typedef struct node
    {
        struct node *next;
        struct node *prev;
        char element[MAX_NAME_LENGTH];
    } Node;

    // The circular buffer is composed of a doubly linked list
    Node *Head;
    Node *Tail;

    // Current size of the list
    uint16_t BufferSize;
};