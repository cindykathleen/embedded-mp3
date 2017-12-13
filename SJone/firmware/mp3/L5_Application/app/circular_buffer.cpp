#include "circular_buffer.hpp"
#include <cstdlib>
#include <stdio.h>
#include <cstring>


CircularBuffer::CircularBuffer()
{
    Head = NULL;
    Tail = NULL;
    BufferSize = 0;
}

CircularBuffer::~CircularBuffer()
{
    if (Head)
    {
        Node *temp;
        while (Head)
        {
            temp = Head;
            Head = Head->next;
            delete temp;
            temp = NULL;
        }
    }
}

void CircularBuffer::InsertBack(char *element)
{
    Node *new_node = new Node;
    memcpy(new_node->element, element, MAX_NAME_LENGTH);

    // Empty list
    if (BufferSize == 0)
    {
        new_node->prev = NULL;
        new_node->next = NULL;
        Head = new_node;
    }
    else
    {
        Tail->next = new_node;
        new_node->prev = Tail;
        new_node->next = NULL;
    }

    Tail = new_node;
    ++BufferSize;
}

char* CircularBuffer::PopFront()
{
    if (Head)
    {
        Node *temp = Head;
        Head = Head->next;
        char* return_value = temp->element;
        delete temp;
        temp = NULL;
        return return_value;
    }
    else
    {
        return NULL;
    }

    --BufferSize;
}

char* CircularBuffer::PopBack()
{
    if (Tail)
    {
        Node *temp = Tail;
        Tail = Tail->prev;
        char* return_value = temp->element;
        delete temp;
        temp = NULL;
        return return_value;
    }
    else
    {
        return NULL;
    }

    --BufferSize;
}

char* CircularBuffer::PopByName(char* element)
{
    Node *current = Head;
    char* return_value;
    while (current)
    {
        if (strcmp(current->element, element) == 0)
        {
            current->prev->next = current->next;
            current->next->prev = current->prev;
            return_value = current->element;
            delete current;
            current = NULL;
            --BufferSize;
            return return_value;
        }
    }

    // Not found
    current = NULL;
    return NULL;
}

void CircularBuffer::RotateForward()
{
    Node *temp = Head->next;

    Head->prev = Tail;
    Head->next = NULL;
    Tail->next = Head;

    // New head and tail
    Tail = Head;
    Head = temp;

    temp = NULL;
}

void CircularBuffer::RotateBackward()
{
    Node *temp = Tail->prev;

    Tail->prev = NULL;
    Tail->next = Head;
    Head->prev = Tail;

    // New head and tail
    Head = Tail;
    Tail = temp;

    temp = NULL;
}

void CircularBuffer::ShuffleList()
{
    // Put linked lists into an array
    Node **array = new Node*[BufferSize];
    Node *temp = Head;
    uint16_t counter = 0;
    while (temp && counter < BufferSize)
    {
        array[counter++] = temp;
        temp = temp->next;
    }

    printf("%d vs %d\n", counter, BufferSize);
    for (int i=0; i<counter; i++)
    {
        printf("%i : %s\n", i, array[i]->element);
    }

    // Fisher Yates Shuffle
    for (int i=0; i<counter; i++)
    {
        uint16_t random = rand() % MAX(i, 1);
        fprintf(stderr, "%d\n", random);
        temp = array[random];
        array[random] = array[i];
        array[i] = temp;
    }

    // Reorganize each node's next/prev with their new next and previous nodes
    Head = array[0];
    for (int i=0; i<counter; i++)
    {
        temp = array[i];

        if (i < counter-1) temp->next = array[i+1];
        else               temp->next = NULL;

        if (i > 0)         temp->prev = array[i-1];
        else               temp->prev = NULL;
    }
    Tail = temp;
    
    temp = NULL;
    delete [] array;
}

char* CircularBuffer::GetHead()
{
    return (Head) ? (Head->element) : NULL;
}

uint16_t CircularBuffer::GetBufferSize()
{
    return BufferSize;
}

void CircularBuffer::PrintBuffer()
{
    Node *temp = Head;
    uint16_t counter = 0;
    while (temp)
    {
        printf("%u : %s", counter++, temp->element);
    }
    temp = NULL;
}