#pragma once
#include <stdio.h>

// @description : Prints each bit of a 32-bit word
// @param word  : The 32-bit word to be printed
inline void print_bits(uint32_t word)
{
    for (int i=31; i>=0; i--) 
    {
        printf("%i", (word & (1 << i) ? (1) : (0)));
    }
    printf("\n");
}

// @description : Copies a string from source to destination of the chosen size, ensures null termination
// @param size  : The number of bytes to copy
// @param src   : The source string
// @param dest  : The destination string
// @note        : dest and src MUST be larger than size otherwise still chance for unsafe behavior
inline void safe_strcpy(char *dest, char *src, uint32_t size)
{
    // Flag to track if a null termination exists in the copied segment
    bool terminated = false;
    for (uint32_t i=0; i<size; i++)
    {
        // Copy
        dest[i] = src[i];

        // Found null termination in the source
        if (src[i] == '\0') 
        {
            terminated = true;
            break;
        }
    }

    // If source never had a null character, then insert one into the destination
    if (!terminated)
    {
        dest[size] = '\0';
    }
}