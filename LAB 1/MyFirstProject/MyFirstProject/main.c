/* ##################################################################
# MILESTONE: 1
# PROGRAM: 1
# PROJECT: Night Rider
# GROUP: 1
# NAME 1: Berezowska, Kiara, V00937549
# NAME 2: Mckinlay, Samantha, V00954147
# DESC: This program toggles LED lights on and off to achieve the night rider blinking pattern.
# DATA
# REVISED ############################################################### */
#include <stdlib.h> // the header of the general-purpose standard library of C programming language
#include <avr/io.h>// the header of I/O port
#include <util/delay_basic.h>
void delaynus(int n) // delay microsecond
{
	int k; for(k=0;k<n;k++)
	_delay_loop_1(1);
}
void delaynms(int n) // delay millisecond
{
	int k; for(k=0;k<n;k++)
	delaynus(1000);
}

/* ################## MAIN ROUTINE ################## */
int main(int argc, char *argv[]){
	//Testing for Milestone 1:
	DDRL = 0b11111111; // Sets all pins on PORTL to output
	PORTL = 0b00110001;
	
	// created while loop to repeat the night rider pattern
	while(1==1){
	DDRC = 0b11111111; // sets all pins on PORTC to output
	// initialize specific pins to high to turn on LEDs
	PORTC = 0b00000011; 
	delaynms(200); // 200ms delay 
	PORTC = 0b00000111;
	delaynms(200);
	PORTC = 0b00001111;
	delaynms(200);
	PORTC = 0b00011110;
	delaynms(200);
	PORTC = 0b00111100;
	delaynms(200);
	PORTC = 0b01111000;
	delaynms(200);
	PORTC = 0b11110000;
	delaynms(200);
	PORTC = 0b11100000;
	delaynms(200);
	PORTC = 0b11000000;
	delaynms(200);
	PORTC = 0b11100000;
	delaynms(200);
	PORTC = 0b11110000;
	delaynms(200);
	PORTC = 0b01111000;
	delaynms(200);
	PORTC = 0b00111100;
	delaynms(200);
	PORTC = 0b00011110;
	delaynms(200);
	PORTC = 0b00001111;
	delaynms(200);
	PORTC = 0b00000111;
	delaynms(200);
	PORTC = 0b00000011;
	delaynms(200);
	}
	 
	return (0); // This line returns a 0 value to the calling program
	// generally means no error was returned
}

