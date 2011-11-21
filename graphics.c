/*
 * Written by Chris J Arges <christopherarges@gmail.com>
 *
 */

#define MAX_WIDTH	128
#define MAX_HEIGHT	96
#define NUM_PIXELS	MAX_WIDTH*MAX_HEIGHT/2

/* return 0 if OK, -1 if bad */
static inline int check_bounds(short x, short y) {
	if ((x > MAX_WIDTH-1) || (x < 0) || (y > MAX_HEIGHT-1) || (y < 0))
		return -1;

	return 0;
}

/* draw a single pixel
 * valid ranges are x: 0-127, y: 0-95 */
void set_pixel(unsigned char *buffer, short x, short y, short val) {
	/* check bounds */
	if (check_bounds(x,y))
		return;

	int pos = y*MAX_WIDTH + x;
 	if (pos % 2)
		buffer[pos/2] = val;
	else
		buffer[pos/2] = 0xf0 & (val << 4);
}

/* reduce the intensity of the pixel by 1 until it reaches 0 
 * for the entire framebuffer */
void fade_buffer(unsigned char *buffer) {
	int i = 0;
	for (i = 0; i < NUM_PIXELS; i++) {
		/* change the first nibble */
		if (buffer[i] & 0x0f)
			buffer[i]--;

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
