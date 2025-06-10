#include "mm.h"
#include <stdint.h>

#define PAGE_SIZE 4 * 1096 //4kB
#define MAX_PAGES 16 //Heap Size 64kB

extern char __heap_start[];
extern char __heap_end[];

static uint8_t *heap_base;
static uint8_t *heap_top;
static uint8_t page_bitmap[MAX_PAGES];

void mm_init(void)
{
    heap_base = (uint8_t*)__heap_start;
    heap_top = (uint8_t*)__heap_end;
    for (int i = 0; i < MAX_PAGES; i++) page_bitmap[i] = 0;
}

void *mm_alloc_page(void)
{
    for (int i =0; i < MAX_PAGES; i++)
    {
        if (page_bitmap[i] == 0)
        {
            page_bitmap[i] = 1;
            return (void*)(heap_base + i * PAGE_SIZE);
        }
    }
    //No free page
    return 0;
}

void mm_free_page(void *ptr)
{
    uintptr_t offset = (uint8_t*)ptr - heap_base;
    int index = offset / PAGE_SIZE;
    if (index >= 0 && index < MAX_PAGES) page_bitmap[index] = 0;
}