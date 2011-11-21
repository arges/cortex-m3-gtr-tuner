/*
 * Written by Chris J Arges <christopherarges@gmail.com>
 */

/* a circular queue implementation */
#define SIZE	128
#define MAX	SIZE-1
struct queue {
	int buffer[SIZE];
	int head;
	int tail;
};

/* print queue */
static inline void print_queue(struct queue *q) {
	int i = 0;
	printf("queue %p = ", q);
	for (i = 0; i < SIZE; i++) {
		printf("%d, ", q->buffer[i]);
	}
	printf("\n");
}

/* initialize a pointer a new queue */
static inline void init_queue(struct queue *q) {
	int i = 0;
	q->head = 0;
	q->tail = MAX;
	for (i = 0; i < SIZE; i++) {
		q->buffer[i] = 0;
	}
}

/* increment and wrap */
static inline int inc(int i) {
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


