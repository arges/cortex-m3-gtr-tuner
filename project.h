/* project parameters */
#define POWER		7
#define FFT_SIZE	1<<POWER
#define SIZE	(1<<POWER)

#define WIDTH		128
#define HEIGHT		80
#define NUM_PIXELS  (WIDTH*HEIGHT/2)

#define FPS		10
#define SAMPLING_FREQ	5280 // 330Hz * 2 (nyquist) * 8 (oversampling)

#define DIFF_THRESHOLD	32

