#include "cachelab.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<assert.h>


typedef struct Node {
	char valid;
	long tag;
	/* Cache line index. */
	int val;
	struct Node* prev;
	struct Node* next;
} Node;

typedef struct LinkedList {
	struct Node* head;
	struct Node* tail;
	int size;
} LinkedList;



/* Cache using LRU replacement policy */
typedef struct Cache {
	int s; 		/* Number of set index bits. */
	int E;   	/* Number of lines per set. */
	int b;		/* Number of block offset bits. */
	int t;		/* Number of tag bits. */
	int m;		/* Address bits, 64-bit. */

	LinkedList* cache;
} Cache;


int verbose = 0;

void printHelp();
void initializeCache(Cache* c);
void simulate(Cache* c, FILE* file);
int load(Cache* c, long addr);
int modify(Cache* c, long addr);
int store(Cache* c, long addr); 

void initializeList(LinkedList* list);
void addLast(LinkedList* list, struct Node* node);
void removeNode(LinkedList* list, struct Node* node);
Node* findNode(LinkedList* list, long val);
void justify(LinkedList* list, struct Node* node);


/* Initialize a empty Linked List. */
void initializeList(LinkedList* list)
{
	list->size = 0;
	list->head = (struct Node* )malloc(sizeof(Node));
	list->tail = (struct Node* )malloc(sizeof(Node));
	list->head->next = list->tail;
	list->tail->prev = list->head;
}

/* Insert node into list in tail. */
void addLast(LinkedList* list, struct Node* node)
{
	node->prev = list->tail->prev;
	node->next = list->tail;
	list->tail->prev->next = node;
	list->tail->prev = node;
	list->size++;
}

/* Remove a node from list. */
void removeNode(LinkedList* list, struct Node* node)
{
	//printf("in removeNode\n");
	node->prev->next = node->next;
	node->next->prev = node->prev;
	list->size--;
	//printf("out removeNode\n");
}

/* Find a node with a specific value. */
Node* findNode(LinkedList* list, long val)
{
	//printf("in findNode\n");
	struct Node* p;
	/* list is empty, return nullptr. */
	if (list->head->next == list->tail)
		return NULL;

	p = list->head->next;
	while (p != list->tail)
	{

		if (p->tag == val)
			return p;
		
		p = p->next;
	}
	
	//printf("out findNode\n");
	/* Cannot find the node with val. */
	return NULL;
}

void justify(LinkedList* list, Node* node)
{
	//printf("in justify\n");
	/* Remove node and Add to list in tail. */
	removeNode(list, node);
	addLast(list, node);
	//printf("out justify\n");
}




int main(int argc, char *argv[])
{
    int i;
    FILE* file;
    Cache* c = (Cache* )malloc(sizeof(Cache));
    for (i = 1; i < argc; i++) {

    	if (strcmp("-h", argv[i]) == 0) {
    		printHelp();
    		exit(0);
    	} else if (strcmp("-v", argv[i]) == 0) {
    		verbose = 1;
    	} else if (strcmp("-s", argv[i]) == 0) {
    		assert(i < argc + 1);
    		c->s = atoi(argv[i + 1]);
    		i++;
    	} else if (strcmp("-E", argv[i]) == 0) {
    		assert(i < argc + 1);
    		c->E = atoi(argv[i + 1]);
    		i++;
    	} else if (strcmp("-b", argv[i]) == 0) {
    		assert(i < argc + 1);
    		c->b = atoi(argv[i + 1]);
    		i++;
    	} else if (strcmp("-t", argv[i]) == 0) {
    		assert(i < argc + 1);
    		file = fopen(argv[i + 1], "r");
    		assert(file);
    		i++;
    	} 			    
    }
    /* Initialize Cache */
    initializeCache(c);
    
	simulate(c, file);

    return 0;
}

/* Print Helps */
void printHelp()
{
	printf("Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
	printf("Options:\n");
	printf("  -h          Print this help message.\n");
	printf("  -v          Optional verbose flag.\n");
	printf("  -s <num>    Number of set index bits.\n");
	printf("  -E <num>    Number of lines per set.\n");
	printf("  -b <num>    Number of block offset bits.\n");
	printf("  -t <file>   Trace file.\n\n");
	
	printf("Examples:\n");
	printf("  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
	printf("  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
	 
}


/* Initialize cache */
void initializeCache(Cache* c)
{
	int i;
	int S = pow(2, c->s);
	c->cache = (LinkedList* )malloc(S * sizeof(LinkedList));
	for (i = 0; i < S; i++)
	{
		/* Initialize */
		initializeList(&c->cache[i]);
	}
}

/* Access address, if hit in cache return 1, if miss return -1, if miss and eviction return -2. */
int load(Cache* c, long addr)
{
	/* Translate address. */
	int index = (addr >> c->b) & ((1 << c->s) - 1);
	/* Extract high t bit */
	long tag = (addr >> (c->b + c->s)) & ((1 << (c->b + c->s)) - 1);
	struct Node* node;
	//printf("tag=%lx, set index=%d\n", tag, index);
	
	node = findNode(&c->cache[index], tag);
	if (node != NULL) {
		/* Cache Hit */
		justify(&c->cache[index], node);
		return 1;
	}
	
	/* Cache Miss */
	if (c->cache[index].size < c->E) {
		/* Cache is not full. */
		node = (struct Node *)malloc(sizeof(Node));
		node->valid = 'y';
		node->tag = tag;
		node->val = c->cache[index].size;
		addLast(&c->cache[index], node);
		return -1;
	} else {
		/* Cache is full, use LRU replacement policy. */
		node = c->cache[index].head->next;
		node->tag = tag;
		justify(&c->cache[index], node);
		return -2;
	}
}

int store(Cache* c, long addr)
{
	int index = (addr >> c->b) & ((1 << c->s) - 1);
	/* Extract high t bit */
	long tag = (addr >> (c->b + c->s)) & ((1 << (c->b + c->s)) - 1);

	struct Node* node;
	node = findNode(&c->cache[index], tag);
	
	/* Cache Hit */
	if (node != NULL) {
		justify(&c->cache[index], node);
		return 1;
	}
	
	/* Cache Miss */
	return load(c, addr);
}


int modify(Cache* c, long addr)
{
	/* Translate address. */
	int index = (addr >> c->b) & ((1 << c->s) - 1);
	long tag = (addr >> (c->b + c->s)) & ((1 << (c->b + c->s)) - 1);
	struct Node* node;
	//printf("tag=%lx, set index=%d\n", tag, index);
	node = findNode(&c->cache[index], tag);
	
	/* Cache hit */
	if (node != NULL) {
		justify(&c->cache[index], node);
		return 1;
	}
	
	if (load(c, addr) == -1) {
		/* Cache miss*/
		store(c, addr);
		return -1;
	} else {
		store(c, addr);
		return -2;
	}
	
}


void printDetail(char op, long addr, int size, int type)
{
	//printf("op=%c,type=%d\n",op,type);
	if (type == 1) {
		if (op == 'L' || op == 'S')
			printf("%c %lx,%d hit\n", op, addr, size);
		else if (op == 'M')
			printf("%c %lx,%d hit hit\n", op, addr, size);
	} else if (type == -1) {
		if (op == 'L') 
			printf("%c %lx,%d miss\n", op, addr, size);
		else if (op == 'M') 
			printf("%c %lx,%d miss hit\n", op, addr, size);
	} else if (type == -2) {
		if (op == 'L' || op == 'S') 
			printf("%c %lx,%d miss eviction\n", op, addr, size);
		else if (op == 'M') 
			printf("%c %lx,%d miss eviction hit\n", op, addr, size);
	}
}


void simulate(Cache* c, FILE* file)
{
	//int i;
	char op;
	long addr;
	int size;
	
	int type;
	
	int misses = 0;
	int hits = 0;
	int evictions = 0;
	
	char* buf = (char *)malloc(50 * sizeof(char));
	/* Read a single line. */
	fscanf(file, "%[^\n]%*c", buf);
	while (!feof(file))
	{
		
		/* Make sure we don't handle I cache access. */
		if (buf[0] == ' ')
		{
			sscanf(buf, " %c %lx,%d", &op, &addr, &size);
			//printf("operation: %c, address: %lx, size: %d\n", op, addr, size);
			if (op == 'L') {
				/* Load value */
				type = load(c, addr);
			} else if (op == 'M') {
				/* Modify value */
				type = modify(c, addr);
			} else if (op == 'S') {
				/* Store value */
				type = store(c, addr);
			}
			//printf("Simulating~~\n");
			if (verbose) printDetail(op, addr, size, type);
			
			// Update hit, miss, eviction state.
			if (type == 1) {
				hits++;
			} else if (type == -1) {
				misses++;
			} else if (type == -2) {
				misses++;
				evictions++;
			}
			// Modify will always hit
			if (op == 'M') {
				hits++;
			}
			
		}
		
		/* Read a single line. */
		fscanf(file, "%[^\n]%*c", buf);
	}
	printSummary(hits, misses, evictions);
}






