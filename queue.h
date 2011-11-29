/*
 * Written by Chris J Arges <christopherarges@gmail.com>
 */

#include "project.h"

/* a circular queue implementation */
struct queue {
	short int buffer[SIZE];
	int head;
	int tail;
};

void init_queue(struct queue *q);
int inc(int i);
int enqueue(struct queue *q, int data);
int dequeue(struct queue *q);

