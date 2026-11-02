CC = gcc
CFLAGS = -Wall -Wextra -g

# Shared buddy allocator files
BALLOC_OBJS = balloc.o freelist.o bbm.o bm.o utils.o

# Default: build both test programs
all: try balloctests


# ============================================================
# DEQ tests using the malloc/free/realloc wrapper
# ============================================================

try: main.o deq.o wrapper.o $(BALLOC_OBJS)
	$(CC) $(CFLAGS) -o try main.o deq.o wrapper.o $(BALLOC_OBJS)


# ============================================================
# Direct buddy allocator tests
# ============================================================

balloctests: main_2_balloctests.o $(BALLOC_OBJS)
	$(CC) $(CFLAGS) -o balloctests main_2_balloctests.o $(BALLOC_OBJS)


# ============================================================
# Object files
# ============================================================

main.o: main.c deq.h
	$(CC) $(CFLAGS) -c main.c

main_2_balloctests.o: main_2_balloctests.c balloc.h freelist.h utils.h
	$(CC) $(CFLAGS) -c main_2_balloctests.c

deq.o: deq.c deq.h error.h
	$(CC) $(CFLAGS) -c deq.c
	
wrapper.o: wrapper.c balloc.h
	$(CC) $(CFLAGS) -c wrapper.c

balloc.o: balloc.c balloc.h freelist.h bbm.h utils.h
	$(CC) $(CFLAGS) -c balloc.c

freelist.o: freelist.c freelist.h utils.h
	$(CC) $(CFLAGS) -c freelist.c

bbm.o: bbm.c bbm.h bm.h utils.h
	$(CC) $(CFLAGS) -c bbm.c

bm.o: bm.c bm.h utils.h
	$(CC) $(CFLAGS) -c bm.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c


# ============================================================
# Run targets
# ============================================================

run: try
	./try

run-balloc: balloctests
	./balloctests


# ============================================================
# Cleanup
# ============================================================

clean:
	rm -f *.o try balloctests

.PHONY: all run run-balloc clean
