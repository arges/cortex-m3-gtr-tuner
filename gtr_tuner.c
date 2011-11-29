/*
 *	(C) 2011 Chris J Arges <christopherarges@gmail.com>
 *
 *  Guitar Tuner Project. This project demonstrates a real-time application
 *  using the Stellaris ARM board. It requires the guitar to be amplified
 *  with a simple pre-amp, and then is digitized using the ADC. The samples
 *  are collected every SAMPLING_FREQ. We need to maintain 2 times the highest
 *  frequency the guitar can produce according to the Nyquist theorem. Once
 *  the sample is digitized it is placed in a circular buffer and the zero 
 *  crossings are counted and compared with a known value.
 *
 */

#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/gpio.h"
#include "driverlib/adc.h"
#include "driverlib/sysctl.h"
#include "drivers/rit128x96x4.h"
#include "utils/ustdlib.h"

#include "project.h"
#include "graphics.h"
#include "queue.h"

int string=0;
char guitar_strings[6][21] = {
	"E - - - - -",
	"- a - - - -",
	"- - d - - -",
	"- - - g - -",
	"- - - - b -",
	"- - - - - e"
};
int guitar_zeros[6] = { 115, 154, 204, 183, 229, 154 };

enum {
	WAVEFORM,
	TUNER,
};
int state=TUNER;		/* put into starting mode */
struct queue data;		/* the global circular buffer */
char buffer[21];		/* char buffer for usprintf */

enum inputs {
	UP = 14,
	DOWN = 13,
	LEFT = 11,
	RIGHT = 7
};

/* 128 x 80 pixels with 4 bits per pixel */
unsigned char framebuffer[NUM_PIXELS];

/* calculate a moving average of the data,
 * this will give us the 0 value */
unsigned int average = 0;
unsigned int total = 0;
unsigned int count = 0;
static inline void update_average(unsigned int next) {
	count = count + 1;
	total = total + next;
	/* overflow condition */
	if (count == 0) {
		count = 1;
		total = 0;
	}
	average = total/count;
}

/* update the number of zero crossings */
unsigned int zeros = 0;
unsigned int prev = 0;
unsigned int max = 0;
unsigned int min = 1024;
static inline void update_zero_crossings(unsigned int next) {
	/* ensure we aren't counting the noise floor */
	if ((max-min) > DIFF_THRESHOLD ) {
		/* if the samples cross from below to above the average */
		if ((prev < average) && (next >= average))
			zeros++;
		/* if the samples cross from above to below */
		else if ((prev > average) && (next <= average))
			zeros++;
	}
	/* store the sample for the next iteration */
	prev = next;	
}

static inline void update_minmax(unsigned int next) {
	if (next > max)
		max = next;

	if (next < min)
		min = next;
}

static inline void reset_stats() {
	zeros = 0;
	max = 0;
	min = 1024;
}

/* display guitar tuner */
static inline void display_tuner() {
	int x = 0;
	int y = 0;

	/* draw middle line */
	for (y = 0; y < 32; y++) {
		set_pixel(framebuffer, WIDTH/2, y+16, 0x9);
	}

	/* draw tuning line */
	if ((max-min) > DIFF_THRESHOLD ) {
		x = (WIDTH/2) + ((zeros-guitar_zeros[string])*4);
		for (y = 0; y < 16; y++) {
			set_pixel(framebuffer, x, y+24, 0xf);
			set_pixel(framebuffer, x+1, y+24, 0xf);
		}
	}
}

/* display waveform data */
static inline void display_waveform() {
	int x = 0;
	for (x = 0; x < WIDTH; x++) {
		unsigned int d = data.buffer[x];
		short y = HEIGHT - ((d + 512-average) / 16);
		set_pixel(framebuffer, x, y, 0xf);
	}
}

/* handle button presses */
void GPIOPortFIntHandler(void) {
	GPIOPinIntClear(GPIO_PORTF_BASE, GPIO_PIN_1);
	IntMasterDisable();
	/* cycle through states */
	state = (state + 1) % 3;
	IntMasterEnable();
}

void GPIOPortEIntHandler(void)
{
	GPIOPinIntClear(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	IntMasterDisable();

	unsigned long ulData = GPIOPinRead(GPIO_PORTE_BASE,
					   (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2
					    | GPIO_PIN_3));

	switch (ulData) {
	case LEFT:
		string = (string + 5) % 6;
		break;
	case RIGHT:
		string = (string + 1) % 6;
		break;
	}

	IntMasterEnable();
}

/* the adc handler inputs the sample and places it into
 * the circular buffer */
void ADCIntHandler(void) {
	unsigned long val = 0;
	ADCIntClear(ADC_BASE, 0);

	/* get the oversampled value, and place it into the circular buffer */
	ADCSoftwareOversampleDataGet(ADC_BASE, 0, &val, 1);
	enqueue(&data, val);

	/* calculate statistics */
	update_minmax(val);
	update_average(val);
	update_zero_crossings(val);
}

/* draw the frame */
int frame = 0;
void Timer1IntHandler(void) {
	TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

	/* clear the frame */
	clear_buffer(framebuffer);

	/* draw different things depending on mode */
	switch(state) {
		case WAVEFORM: {
			display_waveform();
			usprintf(buffer, "wave: %04u %04u", zeros, max-min);
			break;
		}
		case TUNER: {
			display_tuner();
			usprintf(buffer, "TUNER %s %03u", guitar_strings[string], zeros);
		}
	}

	/* draw frame */
	RIT128x96x4ImageDraw(framebuffer, 0, 0, WIDTH, HEIGHT);

	/* draw string */
	RIT128x96x4StringDraw(buffer,0,HEIGHT,0xf);

	/* reset counts */
	reset_stats();
}

void init(void)
{
	/* set up clocks */
	SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_PLL | SYSCTL_OSC_INT |
		       SYSCTL_XTAL_8MHZ);
	unsigned int sysclk = SysCtlClockGet();

	/* set up display */
	RIT128x96x4Init(1000000);

	/* set up pushbuttons */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_1);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA,
			 GPIO_PIN_TYPE_STD_WPU);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);
	GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_FALLING_EDGE);
	GPIOPinIntEnable(GPIO_PORTF_BASE, GPIO_PIN_1);
	IntEnable(INT_GPIOF);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	GPIOPinTypeGPIOInput(GPIO_PORTE_BASE,
			     GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	GPIOPadConfigSet(GPIO_PORTE_BASE,
			 GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
			 GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
	GPIOIntTypeSet(GPIO_PORTE_BASE,
		       GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
		       GPIO_FALLING_EDGE);
	GPIOPinIntEnable(GPIO_PORTE_BASE,
			 GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	IntEnable(INT_GPIOE);

	/* set up frame drawing timer */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
	TimerConfigure(TIMER1_BASE, TIMER_CFG_32_BIT_PER);
	TimerLoadSet(TIMER1_BASE, TIMER_A, sysclk / FPS);
	TimerEnable(TIMER1_BASE, TIMER_A);
	TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
	IntEnable(INT_TIMER1A);

	/* set up adc and timer */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_7);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	ADCSequenceConfigure(ADC_BASE, 0, ADC_TRIGGER_TIMER, 0);
	ADCSoftwareOversampleConfigure(ADC_BASE, 0, 8);
	ADCSoftwareOversampleStepConfigure(ADC_BASE, 0, 0,
					   (ADC_CTL_CH0 | ADC_CTL_IE |
					    ADC_CTL_END));
	TimerConfigure(TIMER0_BASE, TIMER_CFG_32_BIT_PER);
	TimerLoadSet(TIMER0_BASE, TIMER_A, sysclk / SAMPLING_FREQ);
	TimerControlTrigger(TIMER0_BASE, TIMER_A, true);
	ADCSequenceEnable(ADC_BASE, 0);
	ADCIntEnable(ADC_BASE, 0);
	IntEnable(INT_ADC0);
	TimerEnable(TIMER0_BASE, TIMER_A);
	ADCIntClear(ADC_BASE, 0);

	/* init the data queue */
	init_queue(&data);

	/* enable interrupts */
	IntMasterEnable();
}

int main(void) {
	/* init all peripherals and hardware */
	init();

	/* infinite loop */
	while(1) { ;; }
}

