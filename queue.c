/*
 * Written by Chris J Arges <christopherarges@gmail.com>
 */

#include "project.h"

/* a circular queue implementation */
#define SIZE	(1<<POWER)
#define MAX	SIZE-1
struct queue {
	short int buffer[SIZE];
	int head;
	int tail;
};

#if 0
/* print queue */
static inline void print_queue(struct queue *q) {
	int i = 0;
	printf("queue %p = ", q);
	for (i = 0; i < SIZE; i++) {
		printf("%d, ", q->buffer[i]);
	}
	printf("\n");
}
#endif

/* initialize a pointer a new queue */
void init_queue(struct queue *q) {
	int i = 0;
	q->head = 0;
	q->tail = MAX;
	for (i = 0; i < SIZE; i++) {
		q->buffer[i] = 0;
	}
}

/* increment and wrap */
int inc(int i) {
	return ((i+1) % SIZE);
}

int enqueue(struct queue *q, int data) {

	/* update tail */
	q->tail = inc(q->tail);

	/* add new data to the queue */
	q->buffer[q->tail] = data;

	return 0;
}

int dequeue(struct queue *q) {
	int data = 0;

	/* remove head */
	data = q->buffer[q->head];

	/* update head */
	q->head = inc(q->head);

	/* return the new data */
	return data;
}

