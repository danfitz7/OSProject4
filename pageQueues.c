#include "api.h"
#include "pageQueues/h"

/*

// helper function to wrap around indexes in circular queue
void normalize_queue_index(page_queue* q, unsigned int* i){
	while (*i >= q->capacity){
		*i = *i-q->capacity;
	}
	while (*i < 0){
		*i = *i+q->capacity;
	}
}

// push entry on the queue
void push_queue(page_queue* q, Page* p){
	q->pages[q->tail] = p;
	q->tail++;
	normalize_queue_index(&(q->tail));
	
	if ((q->count == QUEUE_CAPACITY)){
		printf("WARNING: queue full: %d!\n", q->count);
	}
	if (q->tail == q->head){
		printf("WARNING: queue full or empty!\n");
	}
	
	q->count++;
	if (q->count > QUEUE_CAPACITY){
		printf("ERROR: queue count %d exceeds capacity!\n", q->count);
		exit(1);
	}
}

boolean queue_empty(page_queue* q){
	return (q->count == 0 || q->head == q->tail);
}

Page* pop_queue(Page_queue* q){
	Page* result = q->Page*s[q->head];
	
	q->head++;
	normalize_queue_index(&(q->head));
	
	if (q->count == 0){
		printf("ERROR: popping from empty queue!\n");
		exit(1);
	}
	q->count--;
	
	if (queue_empty(q)){
		printf("WARNING: queue empty: %d!\n", q->count);
	}
	
	if (q->tail == q->head){
		printf("WARNING: queue full or empty!\n");
	}
	
	return result;
}
/*Page* peek_queue(Page_queue* q){
	if (queue_empty(q)){
		return -1;
	}else{
		return q->Page*s[q->head];
	}
}*/
