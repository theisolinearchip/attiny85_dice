#include <avr/io.h>
#include <util/delay.h> 
#include <avr/interrupt.h>
#include <stdlib.h>

#include "max7219drv.c"

#define		PORT_BUTTON_ROLL	PB3
#define		PIN_BUTTON_ROLL		PIN3
#define		PORT_BUTTON_SET		PB4
#define		PIN_BUTTON_SET		PIN4

static char state = 0; // 0 display something / 1 rolling / 2 setting values

static char button_roll_pressed = 0;
static char button_roll_previously_pressed = 1;
static char button_set_pressed = 0;
static char button_set_previously_pressed = 1;

static char waiting_for_release = 1;

volatile unsigned int seed = 0;

// dices
const char dice_sides_length = 6;
const char dice_sides[6] = {4, 6, 8, 10, 12, 20};

static char dice_values[3] = { -1, -1, -1 };
static char dice_rolling_value[3] = { -1, -1, -1 };
static char dice_sides_index[3] = { 5, 5, 5 }; // index for 20-sides dice

// set mode
static int index_dice_set = 0;

// timers and stuff
volatile int ticks_interval_rolling = 2;
volatile int total_intervals_rolling = 30; // 30 * 2 ~= 122/2 (half of a sec according to preescaler + freq)
volatile int current_intervals_rolling = 0;

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
	// (the value will be reset on each comparison thanks to the CTC mode but if
	// we didn't reach the last one before disabling the timer, some middle
	// value may persist there, so we best start at 0 each time)
	TCNT1 = 0;
}

void stop_timer1() {
	TCCR1 = (1 << CTC1); // no clock source, just keep the CTC1 (clear the CS)
}

ISR(TIMER0_COMPA_vect) {
	// doesn't mind if we overflow this one, we just wanna have
	// a "random" value between powering up and the first click;
	// it'll be a "debounce" function to avoid keeping the button
	// pressed while turning it on
	seed++;
}

// waiting
void waiting_start() {
	waiting_for_release = 1;
	state = 0;
}

// rolling
void roll_start() {
	// set final values at the beginning (according to each dice total sides)
	for (int i = 0; i < 3; i++) {
		dice_values[i] = (rand() % dice_sides[ (int) dice_sides_index[i] ]) + 1;
	}
	current_intervals_rolling = 0;

	state = 1;

	start_timer1();
}

void roll_stop() {
	stop_timer1();
	state = 0;
}

void roll_animate(int dice) {
	// get some random value given the current dice sides (avoid repeating the same previous value by adding 2)
	int sides = dice_sides[ (int) dice_sides_index[dice] ];
	char previous_value = dice_rolling_value[dice];

	dice_rolling_value[dice] = (rand() % sides) + 1;
	if (dice_rolling_value[dice] == previous_value) {
		dice_rolling_value[dice] = (dice_rolling_value[dice] + 2) % sides;
	}
}

// set mode
void setmode_start() {
	waiting_for_release = 1;
	index_dice_set = 0; // reset index

	state = 2;
}

void setmode_change_index_dice() {
	index_dice_set++;
	if (index_dice_set > 2) index_dice_set = 0;
}

void setmode_change_sides_dice() {
	dice_sides_index[index_dice_set]++;
	if (dice_sides_index[index_dice_set] >= dice_sides_length) dice_sides_index[index_dice_set] = 0;
}

ISR(TIMER1_COMPA_vect) {
	current_intervals_rolling++;
	if (current_intervals_rolling >= total_intervals_rolling) {
		roll_stop();
	} else {
		roll_animate(0);
		roll_animate(1);
		roll_animate(2);
	}
}

void proccess_inputs() {
	switch(state) {
		default:
			break;
		case 0: // waiting for roll / set

			if (waiting_for_release && !button_set_pressed && !button_roll_pressed) {
				// debounce between mode: when entering this mode the buttons need
				// to be released in order to continue. Meanwhile we're stucked here
				waiting_for_release = 0;

			} else if (
				!button_set_pressed && !button_set_previously_pressed &&
				button_roll_pressed && !button_roll_previously_pressed
			) {
				// add seed
				srand(seed);

				roll_start();

			} else if (
				button_set_pressed && !button_set_previously_pressed &&
				!button_roll_pressed && !button_roll_previously_pressed
			) {
				setmode_start();
			}

			break;
		case 2: // set mode

			if (waiting_for_release && !button_set_pressed && !button_roll_pressed) {
				waiting_for_release = 0;
				
			} else if (
				!button_set_pressed && button_set_previously_pressed &&
				!button_roll_pressed && !button_roll_previously_pressed
			) {
				// change the current dice
				setmode_change_index_dice();
			} else if (
				!button_set_pressed && !button_set_previously_pressed &&
				!button_roll_pressed && button_roll_previously_pressed
			) {
				// change current dice sides
				setmode_change_sides_dice();
			} else if
			(
				button_set_pressed && button_roll_pressed
			) {
				// exit set mode
				waiting_start();
			}

			break;
	}
}

void show_numbers(char number, char digit_register_first, char digit_register_second, char decimal_point) {
	char first_value = 0x0F; // blank by default (no DP)
	char second_value = 0x0F; // blank by default (no DP)

	if (number > -1) {
		if (number < 10) {
			first_value = 0x00;
			second_value = number;
		} else {
			first_value = number / 10;
			second_value = number % 10;
		}
	}

	if (decimal_point) {
		second_value |= 0x80;
	}

	max7219_send_word(digit_register_first, first_value);
	max7219_send_word(digit_register_second, second_value);
}

void process_display() {
	// in set mode show the dice sides (add DP to the one selected by index_dice_set)
	// in waiting mode show the current value
	// in roll mode show the display-when-rolling value

	char val_0 = -1;
	char val_1 = -1;
	char val_2 = -1;

	switch(state) {
		default:
			break;
		case 0:
			val_0 = dice_values[0];
			val_1 = dice_values[1];
			val_2 = dice_values[2];
			break;
		case 1:
			val_0 = dice_rolling_value[0];
			val_1 = dice_rolling_value[1];
			val_2 = dice_rolling_value[2];
			break;
		case 2:
			val_0 = dice_sides[ (int) dice_sides_index[0] ];
			val_1 = dice_sides[ (int) dice_sides_index[1] ];
			val_2 = dice_sides[ (int) dice_sides_index[2] ];
			break;
	}

	show_numbers(val_0, MAX7219_REG_DIGIT_0, MAX7219_REG_DIGIT_1, (state == 2 && index_dice_set == 0) );
	show_numbers(val_1, MAX7219_REG_DIGIT_2, MAX7219_REG_DIGIT_3, (state == 2 && index_dice_set == 1) );
	show_numbers(val_2, MAX7219_REG_DIGIT_4, MAX7219_REG_DIGIT_5, (state == 2 && index_dice_set == 2) );
}

int main(void) {

	// DDRB &= ~(1 << PORT_BUTTON_SET); // input mode by default, no need to do that
	PORTB |= (1 << PORT_BUTTON_SET); // activate pull up on PB4 (use PORTB since now it's on input mode)
	PORTB |= (1 << PORT_BUTTON_ROLL);

	// init max7219 stuff
	max7219_init(); 

	max7219_send_word(MAX7219_REG_DEC_MODE, 0xFF); // 00 = No decode, FF decode
    max7219_send_word(MAX7219_REG_INTENSITY, 0x0F); // Intensity from 0x00 to 0x0F (max)
    max7219_send_word(MAX7219_REG_SCAN_LIMIT, 0x05); // digits from 0 to 5 (six digits)
    max7219_send_word(MAX7219_REG_SHUTDOWN, 0x01); // 0x01 normal operation, 0x00 shutdown
    max7219_send_word(MAX7219_REG_DISPLAY_TEST, 0x00); // Test mode OFF

    prepare_interrupts();
	start_timer0();

	while(1) {
		button_roll_pressed = !(PINB & (1 << PIN_BUTTON_ROLL));
		button_set_pressed = !(PINB & (1 << PIN_BUTTON_SET));

		proccess_inputs();
		process_display();

		button_roll_previously_pressed = button_roll_pressed;
		button_set_previously_pressed = button_set_pressed;
	}

}
