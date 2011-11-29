/*
 * Written by Chris J Arges <christopherarges@gmail.com>
 *
 */

#include "project.h"
#include "graphics.h"

/* return 0 if OK, -1 if bad */
int check_bounds(short x, short y) {
	if ((x > WIDTH-1) || (x < 0) || (y > HEIGHT-1) || (y < 0))
		return -1;

	return 0;
}

/* draw a single pixel */
void set_pixel(unsigned char *buffer, short x, short y, short val) {
	/* check bounds */
	if (check_bounds(x,y))
		return;

	/* index bit position */
	int pos = y*WIDTH + x;

	/* non-destructively mask in bit position value */
 	if (pos % 2) {
		buffer[pos/2] &= 0xf0;
		buffer[pos/2] |= (0x0f & val);
	} else {
		buffer[pos/2] &= 0x0f;
		buffer[pos/2] |= (0xf0 & (val << 4));
	}
}

/* reduce the intensity of the pixel by 1 until it reaches 0 
 * for the entire framebuffer */
void fade_buffer(unsigned char *buffer) {
	int i = 0;
	for (i = 0; i < NUM_PIXELS; i++) {
		/* change the first nibble */
		if (buffer[i] & 0x0f)
			buffer[i]-=1;

		/* change the second nibble */
		if (buffer[i] & 0xf0)
			buffer[i]-=0x10;
	}
}

/* clear the entire buffer */
void clear_buffer(unsigned char *buffer) {
	int i = 0;
	for (i = 0; i < NUM_PIXELS; i++) {
		buffer[i]=0;
	}
}
