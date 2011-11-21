/*
 *	(C) 2011 Chris J Arges <christopherarges@gmail.com>
 *
 *  Guitar Tuner Project
 *  
 */

#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/pwm.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/adc.h"
#include "driverlib/sysctl.h"
#include "drivers/rit128x96x4.h"
#include "utils/ustdlib.h"

#include "queue.c"
#include "graphics.c"

char buffer[21];
int enabled = 0;

/* 128 x 96 pixels but only 4 bits per pixel */
unsigned char framebuffer[128 * 96 / 2];

/* the global circular buffer */
struct queue data;

void display_data() {
	int i = 0;
	for (i = 0; i < 128; i++) {
		short y = data.buffer[i] / 16;
		set_pixel(framebuffer, i, y, 0xf);
	}
}

void GPIOPortFIntHandler(void) {
	GPIOPinIntClear(GPIO_PORTF_BASE, GPIO_PIN_1);
	IntMasterDisable();
	enabled = !enabled;
	IntMasterEnable();
}

/* the timer handler inputs the sample and places it into
 * the circular buffer */
void Timer0IntHandler(void) {
	unsigned long value[1];
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

	/* begin the ADC conversion */
	ADCProcessorTrigger(ADC0_BASE, 3);
	while(!ADCIntStatus(ADC0_BASE, 3, false)) { ;; }

	/* read value */
	ADCSequenceDataGet(ADC0_BASE, 3, value);

	/* add value to the circular buffer */
	unsigned long val = value[0];
	enqueue(&data, val);
}

/* draw the frame */
void Timer1IntHandler(void) {
	fade_buffer(framebuffer);
	display_data();
	TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
	RIT128x96x4ImageDraw(framebuffer, 0, 0, 128, 96);
}

void init(void) {
	/* set up clocks */
	SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
	
	/* set up display */
	RIT128x96x4Init(1000000);

	/* set up pushbuttons */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_1);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);
	GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_FALLING_EDGE);
	GPIOPinIntEnable(GPIO_PORTF_BASE, GPIO_PIN_1);
	IntEnable(INT_GPIOF);

	/* set up timers */
	unsigned int sysclk = SysCtlClockGet();
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_32_BIT_PER);
	TimerConfigure(TIMER1_BASE, TIMER_CFG_32_BIT_PER);
	TimerLoadSet(TIMER0_BASE, TIMER_A, sysclk/4410);	/* Fs = 4410 */
	TimerLoadSet(TIMER1_BASE, TIMER_A, sysclk/15);		/* 15 fps */
	TimerEnable(TIMER0_BASE, TIMER_A);
	TimerEnable(TIMER1_BASE, TIMER_A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
	IntEnable(INT_TIMER0A);
	IntEnable(INT_TIMER1A);

	/* set up adc */
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_7);
	ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);
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
	while(1) { ;; }
}

