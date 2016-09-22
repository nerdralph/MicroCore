/*** MicroCore - wiring.c ***
An Arduino core designed for ATtiny13
Based on the work done by "smeezekitty" 
Modified and maintained by MCUdude
https://github.com/MCUdude/MicroCore

This file contains timing related
functions such as millis(), micros(),
delay() and delayMicroseconds(), but
also the init() function that set up
timers and analog related stuff.
*/

#include "wiring_private.h"
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include "core_settings.h"

/***** MILLIS() *****/
// The millis counter is based on the watchdog timer, and takes very little processing time and power.
// If 16 ms accuracy is enough, I strongly recommend you to use millis() instead of micros().
#ifdef ENABLE_MILLIS
volatile unsigned long wdt_interrupt_counter = 0;

// This ISR will execute every 16 ms, and increase 
ISR(WDT_vect)
{
	wdt_interrupt_counter++;
}

// Since the WDT counter counts every 16th ms, we'll need to
// multiply by 16 to get the correct millis value.
// The WDT uses it's own clock, so this function is valid
// for all F_CPUs.
unsigned long millis()
{	
	return wdt_interrupt_counter * 16;
}
#endif // ENABLE_MILLIS


/***** MICROS() *****/
// To achieve accurate micros() readings, we'll have to enable the overflow interrupt
// vector on timer 0. This means there will be an interrupt every 256 clock cycle.
// Interrupts as rapidly as this tends to affect the overall time keeping.
// E.g if micros() is enabled, the delay(1) function will actually last 1.3 ms instead.
#ifdef ENABLE_MICROS
volatile unsigned long timer0_overflow = 0;

// This will cause an interrupt every 256 clock cycle
ISR(TIM0_OVF_vect)
{
	timer0_overflow++; // Increment counter by one
}

unsigned long micros()
{
	unsigned long x;
	cli();
	#if F_CPU == 16000
		x = timer0_overflow * 16000;
	#elif F_CPU < 150000 && F_CPU > 80000
		x = timer0_overflow * 2000;
	#elif F_CPU == 600000
		x = timer0_overflow * 427;
	#elif F_CPU == 1000000
		x = timer0_overflow * 256;
	#elif F_CPU == 1200000
		x = timer0_overflow * 213;
	#elif F_CPU == 4000000
		x = timer0_overflow * 64;
	#elif F_CPU == 4800000
		x = timer0_overflow * 53;
	#elif F_CPU == 8000000
		x = timer0_overflow * 32;
	#elif F_CPU == 9600000
		x = timer0_overflow * 27;
	#elif F_CPU == 10000000
		x = timer0_overflow * 26;
	#elif F_CPU == 12000000
		x = timer0_overflow * 21;
	#elif F_CPU == 16000000
		x = timer0_overflow * 16;
	#else 
	#error 
		#warning This CPU frequency is not defined (choose 128 kHz or more)
		sei();
		return 0;
	#endif
	sei();
	return x;
}
#endif // ENABLE_MICROS


void delay(uint16_t ms)
{
	while(ms--)
		_delay_ms(1); 
}


//For bigger delays. Based on code by "kosine" on the Arduino forum
void uS_new(uint16_t us)
{
	uint8_t us_loops;  // Define the number of outer loops based on CPU speed (defined in boards.txt)
  #if (F_CPU == 16000000L) 
    us_loops = 16;
  #elif (F_CPU == 9600000L) || (F_CPU == 10000000)
    us += (us >> 3); // this should be *1.2 but *1.125 adjusts for overheads
    us_loops = 8;
  #elif (F_CPU == 8000000L) 
    us_loops = 8;
  #elif (F_CPU == 4800000L) 
    us += (us >> 3);
    us_loops = 4;
  #elif (F_CPU == 4000000L)
     us_loops = 4;
  #endif

  us = us >> 2;
  uint16_t us_low = us & 0xff;
  uint16_t us_high = us >> 0x08;
    
  // loop is (4 clock cycles) + (4x us) + (4x us_loops)
  // each clock cycle is 62.5ns @ 16MHz
  // each clock cycle is 833.3ns @ 1.2MHz
  asm volatile(
   "CLI\n" // turn off interrupts : 1 clock
   "MOV r28,%0\n" // Store low byte into register Y : 1 clock
   "MOV r29,%1\n" // Store high byte into register Y : 1 clock
   "MOV r30,%2\n" // Set number of loops into register Z : 1 clock
  
     // note branch labels MUST be numerical (ie. local) with BRNE 1b (ie. backwards)
     "1:\n" // : 4 clock cycles for each outer loop
     "MOV r26,r28\n" // Copy low byte into register X : 1 clock
     "MOV r27,r29\n" // Copy high byte into register X : 1 clock
    
       "2:\n" // : 4 clock cycles for each inner loop
       "SBIW r26,1\n" // subtract one from word : 2 clocks
       "BRNE 2b\n" // Branch back unless zero flag was set : 1 clock to test or 2 clocks when branching
       "NOP\n" // add an extra clock cycle if not branching
      
     "SUBI r30,1\n" // subtract one from loop counter : 1 clocks
     "BRNE 1b\n" // Branch back unless zero flag was set : 1 clock to test or 2 clocks when branching 

   "SEI\n" // turn on interrupts : 1 clock (adds extra clock cycle when not branching)
   :: "r" (us_low), "r" (us_high), "r" (us_loops) // tidy up registers
 );
}


void delayMicroseconds(uint16_t us)
{
	if(us == 0)
		return;
		
	#if F_CPU == 16000000 || F_CPU == 12000000
		if(us > 99)
		{
			uS_new(us);
			return;
		}	
		us--;
		us <<= 2;
		//us -= 2; 
	
	#elif F_CPU == 8000000 || F_CPU == 9600000 || F_CPU == 10000000
		if(us > 199)
		{
			uS_new(us); 
			return;
		}
		us -= 3;
		us <<= 1;	
		//us--; //underflow possible?
	
	#elif F_CPU == 4000000 || F_CPU == 4800000
		if(us > 299)
		{
			uS_new(us); 
			return;
		}
		us -= 6;
		// For 4MHz, 4 cycles take a uS. This is good for minimal overhead
	
	#elif F_CPU == 1000000 || F_CPU == 1200000//For slow clocks, us delay is marginal.
		us -= 16;
		us >>= 2; 
		//us--; //Underflow?
	
	#elif F_CPU == 600000
		us -= 32;
		us >>= 3;
	
	#elif F_CPU < 150000 && F_CPU > 80000
		us -= 125;
		us >>= 5;
	
	#else 
		#warning Invalid F_CPU value (choose 128 kHz or more)
		return;
	#endif
		
	asm __volatile__("1: sbiw %0,1\n\t"
			 "brne 1b" : "=w" (us) : "0" (us));
}


// This init() function will be executed before the setup() function does
// Edit the core_settings.h file to choose what's going to be initialized 
// and what's not.
void init()
{
	#ifdef SETUP_PWM	
		// Set Timer0 prescaler
		#if defined(PRESCALER_NONE)				// PWM frequency = (F_CPU/256) / 1
			TCCR0B |= _BV(CS00);
		#elif  defined(PRESCALER_8)				// PWM frequency = (F_CPU/256) / 8
			TCCR0B |= _BV(CS01);
		#elif  defined(PRESCALER_64)			// PWM frequency = (F_CPU/256) / 64
			TCCR0B |= _BV(CS00) | _BV(CS01);
		#elif  defined(PRESCALER_256)			// PWM frequency = (F_CPU/256) / 256
			TCCR0B |= _BV(CS02);
		#elif  defined(PRESCALER_1024)		// PWM frequency = (F_CPU/256) / 1024
			TCCR0B |= _BV(CS00) | _BV(CS02);
		#endif
		
		// Set waveform generation mode
		#if defined(PWM_FAST)
			TCCR0A |= _BV(WGM00) | _BV(WGM01);
		#elif defined(PWM_NORMAL)
			TCCR0A &= ~_BV(WGM00) | ~_BV(WGM01);
		#elif defined(PWM_PHASE_CORRECT)
			TCCR0A |= _BV(WGM00);
		#elif defined(PWM_CTC)
			TCCR0A |= _BV(WGM01);
		#endif	
	#endif	
		
	#ifdef ENABLE_MILLIS
		// Disable global interrupts			
		cli();
		// Reset watchdog
		wdt_reset();
		// Set up WDT interrupt with 16 ms prescaler
		WDTCR = _BV(WDTIE);
		// Enable global interrupts
		sei();
	#endif
	
	// WARNING! Enabling micros() will affect timing functions heavily!
	#ifdef ENABLE_MICROS
		// Enable overflow interrupt on Timer0
		TIMSK0 |= _BV(TOIE0);
		// Set timer0 couter to zero
		TCNT0 = 0; 
		// Turn on global interrupts
		sei();
	#endif
	
	#ifdef SETUP_ADC
		ADMUX = 0;
		
		// Less or equal to 200 kHz
		#if F_CPU <= 200000 
			// Enable the ADC, keep the prescaler of 2 --> F_CPU / 2
			ADCSRA |= _BV(ADEN);
			
		// Between 200 kHz and 1.2 MHz	
		#elif F_CPU <= 1200000 
			// Enable the ADC, set the prescaler to 4 --> F_CPU / 4
			ADCSRA |= _BV(ADEN) | _BV(ADPS1);
			
		// Between 1.2 MHz and 6.4 MHz
		#elif F_CPU <= 6400000
			// Enable the ADC, set the prescaler to 16 --> F_CPU / 16
			ADCSRA |= _BV(ADEN) | _BV(ADPS2);
		
		// More than 6.4 MHz	
		#else
			// Enable the ADC, set the prescaler to 128 --> F_CPU / 128
			ADCSRA |= _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
		#endif
	#endif
}
	