#ifndef MM_H
#define MM_H

void mm_init(void);
void *mm_alloc_page(void);
void mm_free_page(void *ptr);

#endif