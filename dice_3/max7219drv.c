/*
	max7219 communication based on
	https://tinusaur.org/2019/01/06/interfacing-a-max7219-driven-led-matrix-with-attiny85/
*/

#ifndef max7219Driver_c
#define max7219Driver_c

#define MAX7219_DATA				0		// pin 1
#define MAX7219_LOAD				1 		// pin 12
#define MAX7219_CLK					2 		// pin 13

#define MAX7219_REG_NOOP			0x00
#define MAX7219_REG_DIGIT_0			0x01
#define MAX7219_REG_DIGIT_1			0x02
#define MAX7219_REG_DIGIT_2			0x03
#define MAX7219_REG_DIGIT_3			0x04
#define MAX7219_REG_DIGIT_4			0x05
#define MAX7219_REG_DIGIT_5			0x06
#define MAX7219_REG_DIGIT_6			0x07
#define MAX7219_REG_DIGIT_7			0x08
#define MAX7219_REG_DEC_MODE		0x09
#define MAX7219_REG_INTENSITY		0x0A
#define MAX7219_REG_SCAN_LIMIT		0x0B
#define MAX7219_REG_SHUTDOWN		0x0C
#define MAX7219_REG_DISPLAY_TEST	0x0F

void max7219_init() {
	DDRB |= (1 << MAX7219_DATA);
	DDRB |= (1 << MAX7219_LOAD);
	DDRB |= (1 << MAX7219_CLK);
}

void max7219_send_bit(uint8_t bit) {
	PORTB &= ~(1 << MAX7219_CLK);
	if (bit) {
		PORTB |= (1 << MAX7219_DATA); // HIGH
	} else {
		PORTB &= ~(1 << MAX7219_DATA); // LOW
	}
	PORTB |= (1 << MAX7219_CLK);
}

void max7219_send_byte(uint8_t byte) {
	for (uint8_t bit = 0; bit < 8; ++bit) {
		max7219_send_bit((byte & 0x80) != 0); // msb, start sending from the left
		byte <<= 1;
	}
}

void max7219_send_word(uint8_t address, uint8_t data) {
	PORTB &= ~(1 << MAX7219_LOAD);
	max7219_send_byte(address);
	max7219_send_byte(data);
	PORTB |= (1 << MAX7219_LOAD);
}
#endif
