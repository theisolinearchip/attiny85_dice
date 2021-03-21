#include <avr/io.h>
#include <util/delay.h> 
#include <avr/interrupt.h>
#include <stdlib.h>

#define		PORT_DATA	PB2
#define		PORT_LATCH	PB1
#define		PORT_CLOCK	PB0

#define		PORT_BUTTON	PB4
#define		PIN_BUTTON	PIN4

/*
	Leds connected following columns on the dice:

	#0		#4

	#1	#3	#5

	#2		#6

	So to turn on the middle one we send:
	0001 0000
	^  ^   ^
	0  3   6
	
	   0123 4567
	   ---------
	1: 0001 0000	0x10
	2: 0010 1000	0x28
	3: 0011 1000	0x38
	4: 1010 1010	0xAA
	5: 1011 1010	0xBA
	6: 1110 1110	0xEE

*/

const char dice_numbers[7] = {
	0x10,
	0x28,
	0x38,
	0xAA,
	0xBA,
	0xEE,
	0x00, // 6 is empty and not appear when rolling
};

static char state = 0; // 0 display something / 1 rolling

static char button_pressed = 0;
static char button_previously_pressed = 1; // avoid start with the button already pressed

volatile unsigned int seed = 0;

volatile int display_number_index = 6;
volatile int choosen_number_index = 6;

volatile int ticks_interval_rolling = 2;
volatile int total_intervals_rolling = 30; // 30 * 2 ~= 122/2 (half of a sec according to preescaler + freq)
volatile int current_intervals_rolling = 0;

void set_leds(char bits) {

	PORTB &= ~(1 << PORT_DATA);
	PORTB &= ~(1 << PORT_LATCH);
	PORTB &= ~(1 << PORT_CLOCK);

	for (char a = 0; a < 8; a++) {
		// first ones are "pushed" into the last positions

		if ((1 << a) & bits) {
			PORTB |= (1 << PORT_DATA); // on
		} else {
			PORTB &= ~(1 << PORT_DATA); // off
		}

		PORTB |= (1 << PORT_CLOCK); // rise clock
		PORTB &= ~(1 << PORT_CLOCK);
	}
	PORTB |= (1 << PORT_LATCH); // rise LATCH

}

void prepare_interrupts() {

	/*
		presescaler at 8192, so frequency / preescaler:
		We're running at 1Mhz, so:

		1*10^6 / 8192 = 122,0703...

		That means we can "reach" 1 second every 122ish ticks/cicles/interrupts,
		so to trigger the ISR every second, OCR1C = 122;

		https://embeddedthoughts.com/2016/06/06/attiny85-introduction-to-pin-change-and-timer-interrupts/
	*/
	
	// timer 0 used on the first button press to get a random seed
	TCCR0A = (1 << WGM01); // CTC, compares with OCR0A, see datasheet
	OCR0A = 1;

	// timer 1 used	for the rolling time
	TCCR1 |= (1 << CTC1); // reset after comparing with OCR1C (only C!)
	OCR1C = ticks_interval_rolling;
	
	TIMSK = (1 << OCIE0A) | (1 << OCIE1A) ; // enable compare match interrupts

	sei();
	
}

void start_timer0() {
	TCCR0B = (1 << CS02) | (1 << CS00);  // prescaler / 1024
	// do not clear TCNT0 like in timer1 since this will be called
	// only one time, so it'll be always 0
}

void stop_timer0() {
	TCCR0B = 0; // set everything to 0 (the others values doesn't mind here)
}

void start_timer1() {
	TCCR1 |= (1 << CS13) | (1 << CS12) | (1 << CS11); //clock prescaler 8192
	// clear the current time counter in case we have something from the last time
	// (the value will be reset on each comparison thanks to the TCCR1 |= (1 << CTC1)
	// but if we didn't reach the last one before disabling the timer, some middle
	// value may persist there, so we best start at 0 each time
	TCNT1 = 0;
}

void stop_timer1() {
	TCCR1 = (1 << CTC1); // no clock source, just keep the CTC1 (clear the CS)
}

void start_rolling() {
	choosen_number_index = rand() % 6; // from 0 to 5
	current_intervals_rolling = 0;

	state = 1; // rolling

	start_timer1();
}

void stop_rolling() {
	stop_timer1();
	state = 0;

	display_number_index = choosen_number_index;
}

void anim_rolling() {
	int random_value = rand() % 6;
	if (random_value == display_number_index) {
		random_value = (random_value + 2) % 6;
	}
	display_number_index = random_value;
}


ISR(TIMER0_COMPA_vect) {
	// doesn't mind if we overflow this one, we just wanna have
	// a "random" value between powering up and the first click;
	// it'll be a "debounce" function to avoid keeping the button
	// pressed while turning it on
	seed++;
}

ISR(TIMER1_COMPA_vect) {
	current_intervals_rolling++;
	if (current_intervals_rolling >= total_intervals_rolling) {
		stop_rolling();
	} else {
		anim_rolling();
	}
}

int main(void) {

	DDRB |= (1 << PORT_DATA);
	DDRB |= (1 << PORT_LATCH);
	DDRB |= (1 << PORT_CLOCK);

	set_leds(0x00);

	// DDRB &= ~(1 << PORT_BUTTON); // input mode by default, no need to do that
	PORTB |= (1 << PORT_BUTTON); // activate pull up on PB4 (use PORTB since now it's on input mode)

	prepare_interrupts();
	start_timer0();

	while(1) {

		button_pressed = !(PINB & (1 << PIN_BUTTON));

		if (state == 0 && !button_previously_pressed && button_pressed) {
			srand(seed); // init again with an always changing seed (using timer0)
			start_rolling();
		}

		set_leds(dice_numbers[display_number_index]);

		button_previously_pressed = button_pressed;
	}

}
