/*
 * Lab4A.c
 *
 * Created: 2/12/2024 12:29:16 PM
 * Author : kiara
 */ 

#include <stdlib.h>
#include <avr/io.h>
volatile int CurrentPosition;
int Table[4] = {0b00110000, 0b00000110, 0b00101000, 0b00000101};
//add 1 
//check for limits
//output code on portA
//delay 20ms -- give enough power turn 1 step
//30 deg = 17 steps
//60 deg = 33 steps
//90 deg = 60 steps
void mTimer(int count);
void CW (int NumSteps);
void CCW (int NumSteps);

int main(void){
	CLKPR = 0x80;
	CLKPR = 0x01;
	
	TCCR1B = _BV(CS11);
	DDRA = 0x00; //sets PORTA to input
	CurrentPosition = 0;
    while (1) 
    {
		//initialize current position using 90 deg
		CW(60);
		mTimer(2000);
		
		//CW check
		CW(17);
		mTimer(2000);
		CW(33);
		mTimer(2000);
		CW(100);
		mTimer(2000);
		
		//CCW check
		CCW(17);
		mTimer(2000);
		CCW(33);
		mTimer(2000);
		CCW(100);
		mTimer(2000);
    }
}

void CW (int NumSteps){
	int i =0;
	while(i<NumSteps){
		CurrentPosition = CurrentPosition +1;
		if(CurrentPosition == 4){
			CurrentPosition = 0;
		}
		PORTA = Table[CurrentPosition];
		mTimer(20);
		i++;
	}
	
}

void CCW (int NumSteps){
	int i =0;
	while(i<NumSteps){
		CurrentPosition = CurrentPosition - 1;
		if(CurrentPosition == -1){
			CurrentPosition = 3;
		}
		PORTA = Table[CurrentPosition];
		mTimer(20);
		i++;
	}
	
}

/*void CW (int NumSteps){
	int i =0;
	while(i<NumSteps){
		//if(CurrentPosition ==1){
			CurrentPosition = CurrentPosition +1;
			PORTA = 0bx00110000 //step 1
			mTimer(20);
			i++;
			continue;
		//}
		if(CurrentPosition ==2){
			CurrentPosition = CurrentPosition +1;
			PORTA = 0bx00000110 //step 2
			mTimer(20);
			i++;
			continue;
		}
		if(CurrentPosition ==3){
			CurrentPosition = CurrentPosition +1;
			PORTA = 0bx00101000 //step 3
			mTimer(20);
			i++;
			continue;
		}
		if(CurrentPosition ==4){
			CurrentPosition = 1;
			PORTA = 0bx00101000 //step 3
			mTimer(20);
			i++;
			continue;
		}
	}
	
	
}*/

/**************************************************************************************
* DESC: Acts as a clock.
* INPUT: Amount of time that has to be counted.
* RETURNS: Nothing
*/
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


