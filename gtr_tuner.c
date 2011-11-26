/*
 *	(C) 2011 Chris J Arges <christopherarges@gmail.com>
 *
 *  Guitar Tuner Project. This project demonstrates a real-time application
 *  using the Stellaris ARM board. It requires the guitar to be amplified
 *  with a simple pre-amp, and then is digitized using the ADC. The samples
 *  are collected every SAMPLING_FREQ. We need to maintain 2 times the highest
 *  frequency the guitar can produce according to the Nyquist theorem. Once
 *  the sample is digitized it is placed in a circular buffer and an fft
 *  is calculated on this data to deteremine the primary frequency. Using this
 *  it can be determined if the string is a higher or lower frequency and 
 *  this data can be displayed to the user to adjust the strings accordingly.
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
#include "driverlib/uart.h"
#include "driverlib/adc.h"
#include "driverlib/sysctl.h"
#include "drivers/rit128x96x4.h"
#include "utils/ustdlib.h"

/* project parameters */
#define POWER	7

#define WIDTH	128
#define HEIGHT	80
#define NUM_PIXELS  (WIDTH*HEIGHT/2)

#define FPS				10
#define SAMPLING_FREQ	8820

#include "queue.c"
#include "graphics.c"
#include "fft.c"

enum state {
	FFT=0,
	WAVEFORM=1,
	TUNER=2,
};

int state=FFT; 	/* put into starting mode */

short int fft[(1<<POWER)];	/* fft buffer */
int enabled = 0;
char buffer[21];		/* char buffer for usprintf */
struct queue data;		/* the global circular buffer */
/* 128 x 80 pixels with 4 bits per pixel */
unsigned char framebuffer[NUM_PIXELS];
int ready = 0;
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

/* display waveform data */
static inline void display_waveform() {
	int x = 0;
	for (x = 0; x < WIDTH; x++) {
		unsigned int d = data.buffer[x];
		short y = HEIGHT - ((d + 512-average) / 16);
		set_pixel(framebuffer, x, y, 0xf);
	}
}

/* display result of fft function */
static inline void display_fft() {
	int x = 0;
	for (x = 0; x < WIDTH; x++) {
		unsigned int d = fft[x];
		short y = d / 16;
		set_pixel(framebuffer, x, y, 0xf);
	}
}

void GPIOPortFIntHandler(void) {
	GPIOPinIntClear(GPIO_PORTF_BASE, GPIO_PIN_1);
	IntMasterDisable();
	state ^= 1;	/* cycle state */
	IntMasterEnable();
}

/* the timer handler inputs the sample and places it into
 * the circular buffer */
void Timer0IntHandler(void) {
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntMasterDisable();
	unsigned long value[1];

	/* begin the ADC conversion */
	ADCProcessorTrigger(ADC0_BASE, 3);
	while(!ADCIntStatus(ADC0_BASE, 3, false)) { ;; }

	/* read value */
	ADCSequenceDataGet(ADC0_BASE, 3, value);

	/* add value to the circular buffer */
	unsigned long val = value[0];
	enqueue(&data, val);

	/* use current value and calculate average */
	update_average(val);

	IntMasterEnable();
}

/* draw the frame */
void Timer1IntHandler(void) {
	TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

	/* clear the frame */
	clear_buffer(framebuffer);

	/* draw different things depending on mode */
	switch(state) {
		case WAVEFORM:
			display_waveform();
			usprintf(buffer, "waveform: %03u %04u", average, count);
			break;
		case FFT:
			display_fft();
			usprintf(buffer, "fft");
			break;
	}

	/* draw frame */
	RIT128x96x4ImageDraw(framebuffer, 0, 0, WIDTH, HEIGHT);

	/* draw string */
	RIT128x96x4StringDraw(buffer,0,HEIGHT,0xf);

	/* ready to calculate another fft */
	ready = 1;
}

void init(void) {
	/* set up clocks */
	SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_PLL
		| SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
	
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

	/* set up timers */
	unsigned int sysclk = SysCtlClockGet();
	/* set up sampling timer */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_32_BIT_PER);
	TimerLoadSet(TIMER0_BASE, TIMER_A, sysclk/SAMPLING_FREQ);
	TimerEnable(TIMER0_BASE, TIMER_A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	IntEnable(INT_TIMER0A);
	/* set up frame drawing timer */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
	TimerConfigure(TIMER1_BASE, TIMER_CFG_32_BIT_PER);
	TimerLoadSet(TIMER1_BASE, TIMER_A, sysclk/FPS);
	TimerEnable(TIMER1_BASE, TIMER_A);
	TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
	IntEnable(INT_TIMER1A);

	/* set up adc */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_7);
	ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 |
		ADC_CTL_IE | ADC_CTL_END);
	ADCSequenceEnable(ADC0_BASE, 3);
	ADCIntClear(ADC0_BASE, 3);

	/* enable interrupts */
	IntMasterEnable();
}
int main(void) {
	/* init all peripherals and hardware */
	init();

	/* init the data queue */
	init_queue(&data);

	/* infinite loop */
	while(1) {
		/* perform another calculation that will
			be interrupted by getting more samples. */
		if (ready) {
			do_fft(data.buffer, fft);
			ready = 0;
		}
	}
}

