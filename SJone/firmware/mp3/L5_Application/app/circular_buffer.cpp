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

// void CircularBuffer::InsertFront(char* element)
// {
//     Node *new_node    = new Node;
//     new_node->prev    = NULL;
//     new_node->next    = Head;
//     new_node->element = element;

//     Head = new_node;

//     if (BufferSize == 0)
//     {
//         Tail = new_node;
//     }

//     ++BufferSize;
// }

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
    while (temp)
    {
        array[counter++] = temp;
        temp = temp->next;
    }

    // Fisher Yates Shuffle
    for (int i=1; i<BufferSize; i++)
    {
        uint16_t random = rand() % i;
        temp = array[random];
        array[random] = array[i];
        array[i] = temp;
    }

    Head = array[0];
    temp = Head;
    temp->prev = NULL;
    for (int i=1; i<BufferSize; i++)
    {
        temp->next = array[i];
        array[i]->prev = temp;
        temp = temp->next;
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
    Node *temp;
    uint16_t counter = 0;
    while (temp)
    {
        printf("%i : %s", temp->element);
    }
    temp = NULL;
}