#ifndef PAGEQUEUES_H
#define PAGEQUEUES_H
/*
#include "api.h"

typedef unsigned int queue_index;

// page queue struct
typedef struct {
	unsigned int capacity;
	queue_index head; 	// index of the element that is the head of the queue
	queue_index tail; 	// always the index circularly-after the last element in the queue. If this is equal to the head index, the queue is either full or empty
	unsigned int count;	// number of items in the queue
	Page* pages[];
} page_queue;

struct page_queue make_queue(unsigned int cap){
	struct page_queue q = {.capacity = cap, .head = 0, .tail = 1, .count = 0, .pages = Page*[cap]};
	return q;
}

void push_queue(page_queue* q, Page* p);

boolean queue_empty(page_queue* q);

Page* pop_queue(page_queue* q);
*/
#endif
