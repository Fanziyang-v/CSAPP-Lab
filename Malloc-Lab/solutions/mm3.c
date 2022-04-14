/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Bug Makers",
    /* First member's full name */
    "Struggle",
    /* First member's email address */
    "1229417754@qq.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* Base constants and macros */
#define WSIZE		4		/* Word and header/footer size (bytes) */
#define DSIZE		8		/* Double word size (bytes) */
#define CHUNKSIZE	(1<<12)	/* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word*/
#define PACK(size, alloc)	((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)		(*(unsigned int *)(p))
#define PUT(p, val)	(*(unsigned int *)(p) = val)

/* Get and Set pointer value by ptr at address p */
#define GET_PTR_VAL(p)	(*(unsigned long *)(p))
#define SET_PTR(p, ptr)	(*(unsigned long *)(p) = (unsigned long)(ptr))

/* Read and write pred and succ pointer at address p */
#define GET_PRED(p)	((char *)(*(unsigned long *)(p)))
#define GET_SUCC(p)	((char *)(*(unsigned long *)(p + DSIZE)))
#define SET_PRED(p, ptr)	(SET_PTR((char *)(p), ptr))
#define SET_SUCC(p, ptr)	(SET_PTR(((char *)(p)+(DSIZE)), ptr))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)		(GET(p) & ~0x7)
#define GET_ALLOC(p)	(GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)	((char* )(bp) - WSIZE)
#define FTRP(bp)	((char* )(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Segregated free list array */
static char **segregated_free_list_headp;
static char **segregated_free_list_tailp;
static int segregated_list_size;

/* heap listp*/
static char *heap_listp;


/* Remove a node from a double linked list. */
static void remove_block(char *p)
{
	void *pred = GET_PRED(p);
	void *succ = GET_SUCC(p);

	SET_SUCC(pred, succ);
	SET_PRED(succ, pred);
}

/* Insert a noe into a double linked list (Head Insertion). */
static void insert_block(char *listp, char *p)
{
	SET_PRED(p, listp);
	SET_SUCC(p, GET_SUCC(listp));
	
	SET_SUCC(listp, p);
	SET_PRED(GET_SUCC(p), p);
}


/* Get the size class free list index by the given size. */
static int get_free_list_idx(size_t size)
{
	int idx;
	if (size <= 32) {
		idx = 0;
	} else if (size <= 64) {
		idx = 1;
	} else if (size <= 128) {
		idx = 2;
	} else if (size <= 256) {
		idx = 3;
	} else if (size <= 512) {
		idx = 4;
	} else if (size <= 1024) {
		idx = 5;
	} else if (size <= 2048) {
		idx = 6;
	} else if (size <= 4096) {
		idx = 7;
	} else {	/* size > 4096 */
		idx = 8;
	}
	return idx;
}

/* coalesce -- Coalesce free block */
static void *coalesce(void *ptr)
{
	void *prevptr = PREV_BLKP(ptr);
	void *nextptr = NEXT_BLKP(ptr);
	size_t prev_alloc = GET_ALLOC(FTRP(prevptr));
	size_t next_alloc = GET_ALLOC(HDRP(nextptr));
	size_t size = GET_SIZE(HDRP(ptr));

	char *listp;
	if (prev_alloc && next_alloc) {				/* Case 1 - Nothing to do */
		return ptr;
	} else if (prev_alloc && !next_alloc) {		/* Case 2 */
		size += GET_SIZE(HDRP(nextptr));
		/* Remove next and current block in the free list. */
		remove_block(ptr);
		remove_block(nextptr);
	} else if (!prev_alloc && next_alloc) {		/* Case 3 */
		size += GET_SIZE(FTRP(prevptr));
		/* Remove previous and current block in the free list. */
		remove_block(prevptr);
		remove_block(ptr);
		ptr = prevptr;
	} else if (!prev_alloc && !next_alloc) {	/* Case 4 */
		size += GET_SIZE(HDRP(prevptr)) +
			GET_SIZE(FTRP(nextptr));
		/* Remove previous, current and next block in the free list. */
		remove_block(prevptr);
		remove_block(ptr);
		remove_block(nextptr);
		ptr = prevptr;
	}

	/* Set New header and footer */
	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));
	/* Insert new coalescing block into free list. */
	listp = segregated_free_list_headp[get_free_list_idx(size)];
	insert_block(listp, ptr);

	return ptr;
}


/* extend_heap -- Extend heap */
static void *extend_heap(size_t words)
{
	char *bp;
	char *listp;
	size_t size;

	
	/* Allocate an even number of words to maintain alignment */
	size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;

	/* Initialize free block header/footer and the epilogue header */
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));

	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));	/* New Epilogue Block */

	listp = segregated_free_list_headp[get_free_list_idx(size)];	/* Get free list by the size. */
	insert_block(listp, bp);			/* Insert new free block into free list */

	/* Coalesce if the previous block was free */
	return coalesce(bp);
	
}

/* find_fit -- find the first fit free block in free list. */
static void *find_fit(size_t asize)
{
	//printf("find_fit(%u)--in\n", asize);
	void* p;
	int i;
	int idx = get_free_list_idx(asize);
	char *listp;
	char *endp;

	
	/* Search a big enough block */
	for (i = idx; i < segregated_list_size; i++) {
		listp = segregated_free_list_headp[i];
		endp = segregated_free_list_tailp[i];
		for (p = GET_SUCC(listp); p != endp; p = GET_SUCC(p)) {
			if (asize <= GET_SIZE(HDRP(p)))  {
				return p;
			}
		}
	}

	/* not fit */
	return NULL;
}

/* place when remaining part size is greater than 6 word, divide it. */
static void place(void *bp, size_t asize)
{
	char *listp;
	size_t size = GET_SIZE(HDRP(bp));
	void *ptr;

	if (size - asize >= 3*DSIZE) {
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		ptr = NEXT_BLKP(bp);
		PUT(HDRP(ptr), PACK(size-asize, 0));
		PUT(FTRP(ptr), PACK(size-asize, 0));

		/* Remove old block from free list */
		remove_block(bp);
		/* Insert new block into free list. */
		listp = segregated_free_list_headp[get_free_list_idx(size-asize)];
		insert_block(listp, ptr);
	} else {
		PUT(HDRP(bp), PACK(size, 1));
		PUT(FTRP(bp), PACK(size, 1));
		/* Remove this block from free list. */
		remove_block(bp);
	}

}


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	int i;
	char *p;
	char *q;

	/* Create the initial empty heap(584 bytes)--- 9 headp block, and 9 tailp block, 18 pointer array and padding 8 bytes. */
	if ((heap_listp = mem_sbrk(2*(9*DSIZE) + (3*DSIZE)*18 + 2*WSIZE)) == (void *)-1) {
		return -1;
	}

	PUT(heap_listp, PACK(0, 0));						/* Padding- for alignment 8 bytes */
	heap_listp += WSIZE;
	segregated_free_list_headp = (char **)heap_listp;	/* Get segregated free list headp array */
	heap_listp += 9*DSIZE;
	segregated_free_list_tailp = (char **)heap_listp;	/* Get segregated free list tailp array */

	heap_listp += (9*DSIZE+WSIZE);

	/* Set each free list headp and tailp */
	p = heap_listp;
	q = heap_listp + (9*(3*DSIZE));
	segregated_list_size = 9;
	for (i = 0; i < segregated_list_size; i++) {
		/* Set header and footer */
		PUT(HDRP(p), PACK(3*DSIZE, 1));
		PUT(FTRP(p), PACK(3*DSIZE, 1));
		PUT(HDRP(q), PACK(3*DSIZE, 1));
		PUT(FTRP(q), PACK(3*DSIZE, 1));
		/* Set pred and succ */
		SET_PRED(p, NULL);
		SET_SUCC(p, q);
		SET_PRED(q, p);
		SET_SUCC(q, NULL);
		/* Add p and q into segregated free list headp and tailp array */
		segregated_free_list_headp[i] = p;
		segregated_free_list_tailp[i] = q;
		/* Iterate p and q */
		p = NEXT_BLKP(p);
		q = NEXT_BLKP(q);
	}

	PUT(HDRP(heap_listp+(18*(3*DSIZE))), PACK(0, 1));		/* Epilogue Header */
	
	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
		return -1;
	}

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;		/* Adjusted block size */
    size_t extendsize;	/* Amount to extend heap if no fit */
   	char *bp;
   	
	//printf("malloc(%u) -- IN\n", size);
   	/* Ignore spurious requests */
   	if (size == 0)
   		return NULL;
   	
   	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= 2*DSIZE) {
		asize = 3*DSIZE;
	} else {
		asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
	}

   	/* Search the free list for a fit */
   	if ((bp = find_fit(asize)) != NULL) {
   		place(bp, asize);
   		return bp;
   	}
   	
   	/* No fit found. Get more memory and place the block */
   	extendsize = MAX(asize, CHUNKSIZE);
   	if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
   		return NULL;
   	place(bp, asize);

   	return bp; 	

}

/*
 * mm_free - Freeing a block and insert it into a suitable size class.
 */
void mm_free(void *ptr)
{
	char *listp;
	size_t size = GET_SIZE(HDRP(ptr));

	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));

	/* Insert to listp */
	listp = segregated_free_list_headp[get_free_list_idx(size)];
	insert_block(listp, ptr);

	/* Coalesce if previous or next block is free. */
	coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
	void *nextptr;
	char *listp;

	size_t blockSize;
    size_t extendsize;
	size_t asize;	/* Adjusted block size */
	size_t sizesum;

	if (ptr == NULL) { 			/* If ptr == NULL, call mm_alloc(size) */
		return mm_malloc(size);
	} else if (size == 0) { 	/* If size == 0, call mm_free(size) */
		mm_free(ptr);
		return NULL;
	}

	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= 2*DSIZE) {
		asize = 3*DSIZE;
	} else {
		asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
	}

	blockSize = GET_SIZE(HDRP(ptr));

	
	if (asize == blockSize) {						/* Case 1: asize == block size, nothing to do.  */
		return ptr;
	} else if (asize < blockSize) {					/* Case 2: asize < blockSize, */
		if (blockSize-asize >= 3*DSIZE) {
			/* Change this block header and footer */
			PUT(HDRP(ptr), PACK(asize, 1));
			PUT(FTRP(ptr), PACK(asize, 1));
			/* Change next block header and footer */
			nextptr = NEXT_BLKP(ptr);
			PUT(HDRP(nextptr), PACK(blockSize-asize, 0));
			PUT(FTRP(nextptr), PACK(blockSize-asize, 0));
			
			/* insert next block into free list */
			listp = segregated_free_list_headp[get_free_list_idx(blockSize-asize)];
			insert_block(listp, nextptr);
		} 
		/*  OR remaining block size is less than 24 bytes, cannot divide, nothing to do */
		return ptr;
	} else {										/* Case 3: asize > blockSize */
		/* Check next block */
		nextptr = NEXT_BLKP(ptr);
		sizesum = GET_SIZE(HDRP(nextptr))+blockSize;	/* Sum of this and next block size */
		if (!GET_ALLOC(HDRP(nextptr)) && sizesum >= asize) {	/* Next block is free and size is big enough. */
			/* Remove next free block from free list. */
			remove_block(nextptr);
			if (sizesum-asize >= 3*DSIZE) {
				
				//printf("sizesum is big enough!==>OUT\n");
				PUT(HDRP(ptr), PACK(asize, 1));
				PUT(FTRP(ptr), PACK(asize, 1));
				/* Set next block header, footer and pred, succ and insert it into free list. */
				nextptr = NEXT_BLKP(ptr);
				PUT(HDRP(nextptr), PACK(sizesum-asize, 0));
				PUT(FTRP(nextptr), PACK(sizesum-asize, 0));
				listp = segregated_free_list_headp[get_free_list_idx(sizesum-asize)];
				insert_block(listp, nextptr);
			} else {
				PUT(HDRP(ptr), PACK(sizesum, 1));
				PUT(FTRP(ptr), PACK(sizesum, 1));
			}
			return ptr;
		} else {		/* Next block is allocated or size is not enough. */
			newptr = find_fit(asize);
			if (newptr == NULL) {
				extendsize = MAX(asize, CHUNKSIZE);
				if ((newptr = extend_heap(extendsize/WSIZE)) == NULL) {	/* Can not find a fit block, it must allocate memory from heap. */
					return NULL;
				}
			}
			place(newptr, asize);
			memcpy(newptr, oldptr, blockSize-2*WSIZE);
			mm_free(oldptr);
			return newptr;
		}
	}

}















