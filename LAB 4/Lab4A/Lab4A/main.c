/* 	
	Course		: UVic Mechatronics 458
	Milestone	: 4A
	Title		: INTERFACING

	Name 1:	Mckinlay, Samantha	            Student ID: V00954147
	Name 2:	Berezowska, Kiara				Student ID: V00937549
	
	Description: Interface with simple DC stepper motor, setting up CW and CCW rotation. Implemented PWM funtion.
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
void PWM();

int main(void){
	CLKPR = 0x80;
	CLKPR = 0x01;
	
	TCCR1B = _BV(CS11);
	DDRA = 0x00; //sets PORTA to input
	
	CurrentPosition = 0; //initializes position
	
	PWM();//calls PMW
	
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

//one line of code for each step (no more than 7)
void PWM(){
	TCCR0A |= _BV(WGM01)|_BV(WGM00); //selecting Fast PWN mode 3
	TIMSK0 |= _BV(OCIE0A); //enable output compare interrupt for timer0
	TCCR0A |= _BV(COM0A1);//set compare match output mode to clear and set output compare A when timer reaches TOP
	TCCR0B |= _BV(CS01);//sets prescale factor to 8
	OCR0A = 0x80;//set  output compare register A to TOP
	DDRB = 0b11111111; //set all PORTB to output
}


