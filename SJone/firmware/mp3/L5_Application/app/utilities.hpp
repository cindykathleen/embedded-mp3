#pragma once

inline void print_bits(uint32_t word)
{
    for (int i=31; i>=0; i--) 
    {
        printf("%i", (word & (1 << i) ? (1) : (0)));
    }
    printf("\n");
}