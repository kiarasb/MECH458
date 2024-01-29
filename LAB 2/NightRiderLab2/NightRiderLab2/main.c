/*
 * NightRiderLab2.c
 *
 * Created: 1/29/2024 1:06:53 PM
 * Author : kiara
 */ 

#include <stdlib.h> // the header of the general-purpose standard library of C programming language
#include<avr/io.h> // the header of I/O port
#include<avr/interrupt.h>
void mTimer (int count){
   /***
      Setup Timer1 as a ms timer
	  Using polling method not Interrupt Driven
   ***/
	  
   int i;

   i = 0;

   //TCCR1B |= _BV (CS11);  // Set prescaler (/8) clock 16MHz/8 -> 2MHz
   /* Set the Waveform gen. mode bit description to clear
     on compare mode only */
   TCCR1B |= _BV(WGM12);

   /* Set output compare register for 1000 cycles, 1ms */
   OCR1A = 0x03E8;
 
   /* Initialize Timer1 to zero */
   TCNT1 = 0x0000;

   /* Enable the output compare interrupt */
   TIMSK1 = TIMSK1 | 0b00000010;

   /* Clear the Timer1 interrupt flag and begin timing */
   TIFR1 |= _BV(OCF1A);

   /* Poll the timer to determine when the timer has reached 1ms */
   while (i < count){
      if((TIFR1 & 0x02) == 0x02){
	
	   /* Clear the interrupt flag by WRITING a ONE to the bit */
	   TIFR1 |= _BV(OCF1A);
	   i++;
	   }
	 } 
   return;
}  /* mTimer */


/* ################## MAIN ROUTINE ################## */
int main(int argc, char *argv[]){
	// created while loop to repeat the night rider pattern
	CLKPR = 0x80;
	CLKPR = 0x01;
	
	TCCR1B = _BV(CS11);
	
	while(1==1){
		DDRC = 0b11111111; // sets all pins on PORTC to output
		// initialize specific pins to high to turn on LEDs
		PORTC = 0b00000011;
		mTimer(200);
		while(PORTC != 0b11000000){
			PORTC = PORTC << 1;
			mTimer(200);
		}
		while(PORTC != 0b00000110){
			PORTC = PORTC >> 1;
			mTimer(200);
		}
	}
	return (0); // This line returns a 0 value to the calling program
	// generally means no error was returned
}


