/* UTILS.C file for cs452 HW2
 * author: Caleb Corolewski
 * date: 9-19-26
 *  */

#include "utils.h"
#include <sys/mman.h>
#include <stddef.h>

// mmalloc function that calls mmap to create a block of memory at a location picked by mmap of size size. Sets the memory to be read/write and that it is private to this process
// and not associated with a file.
extern void *mmalloc(size_t size){
	return mmap(NULL,
		    size,
		    PROT_READ|PROT_WRITE,
		    MAP_PRIVATE|MAP_ANONYMOUS,
		    -1,
		    0);
}

//Frees up/releases the memory stored at address p
extern void mmfree(void *p, size_t size){
	munmap(p, size);
}

//Divides and rounds up
extern size_t divup(size_t n, size_t d){
	return (n+d-1)/d;
}

//Converts the number of bits into the number of bytes needed to hold said bits
extern size_t bits2bytes(size_t bits){
	return divup(bits, bitsperbyte);
}

//Uses the exponent to calculate the size in bits done by bit shifting
extern size_t e2size(int e){
	return ((size_t)1) << e;
}

//Given a size in bits, determine the smallest expontent, e, that can hold those bits
extern int size2e(size_t size){
	int e = 0;
	size_t block =1;

	while(block < size){
		block <<= 1;
		e++;
	}
	return e;
}

//Sets the bit for the bitmap shifted left
extern void bitset(void *p, int bit){
	unsigned char *byte = p;
	*byte |= (unsigned char)(1u << bit);
}

//Clears the bit for the bitmap
extern void bitclr(void *p, int bit){
	unsigned char *byte = p;
	*byte &= (unsigned char)(1u << bit);
}

//Toggles/inverts the bit 
extern void bitinv(void *p, int bit){
	unsigned char *byte = p;
	*byte ^= (unsigned char)(1u << bit);
}

//checks if a bit is set
extern int bittst(void *p, int bit){
	unsigned char *byte = p;
	return(*byte & (unsigned char)(1u << bit)) != 0;
}
