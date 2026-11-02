/* freelist.c file for cs452 hw2
 * author: Caleb Corolewski
 * date: 9-19-26
 *  */
#include "utils.h"
#include "freelist.h"
#include <stdio.h>

/* Structure of the array:
 * Each entry is the head of a free list
 * so index 0 -> blocks of size 2^l
 * then index 1 -> blocks of size 2^(l+1)
 * ...
 * and so on until index u-l -> blocks of size 2^u
 *
 * Furthermore the next pointer is stored at the beginning of each free block
 */

//Create the array of free list heads
extern FreeList freelistcreate(size_t size, int l, int u){
	(void)size;
	
	size_t lists = (size_t)(u-l+1);
	size_t bytes = lists * sizeof(void *);

	void **heads = mmalloc(bytes);

	if((long)heads == -1){
		return NULL;
	}

	for(size_t i = 0l; i < lists; i++){
		heads[i] = NULL;
	}

	return heads;
}

//Delete the array of free list heads
extern void freelistdelete(FreeList f, int l, int u){
	if(f==NULL){
		return;
	}

	size_t lists = (size_t)(u-l+1);
	size_t bytes = lists *sizeof(void *);

	mmfree(f, bytes);
}

// Remove and return the first block from the free list at level e
extern void *freelistalloc(FreeList f, void *base, int e, int l){
	(void)base;

	void **heads = f;
	
	int index = e-l;

	void *block = heads[index];

	if(block == NULL){
		return NULL;
	}

	heads[index] = *(void **)block;

	return block;
}

//Add a block to the beginning of the free list at level e
extern void freelistfree(FreeList f, void *base, void *mem, int e, int l){
	(void)base;
	
	void **heads = f;

	int index = e-l;

	//store current head inside the free block
	*(void **)mem = heads[index];

	//make the new block the new head
	heads[index] = mem;
}

//Determine the size of a free block
extern int freelistsize(FreeList f, void *base, void *mem, int l, int u){
	(void)base;

	void **heads=f;

	for(int e = l; e <= u; e++){
		void *current = heads[e-l];

		while(current != NULL){
			if(current == mem){
				return (int)e2size(e);
			}

			current = *(void **)current;
		}
	}
	
	return 0;
}

//Print all free lists
extern void freelistprint(FreeList f, int l, int u){
	void **heads = f;

	for (int e = l; e <= u; e++){
		printf("%d (%zu):", e, e2size(e));

		void *current = heads[e-l];

		while (current != NULL){
			printf(" %p", current);
			current = *(void **)current;
		}
		
		printf("\n");
	}
}
