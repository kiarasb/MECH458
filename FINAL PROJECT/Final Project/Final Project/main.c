/*
TODO:
-calibrate RL sensor
-make stepper motor run
-find bug in FIFO
-change timer and ADC and timer and PWM to 10 bit from 8 bit
-set up hardware
*/ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "lcd.h"
#include "LinkedQueue.h" 
#define disable 0 //sets ENA and ENB to low disables driver
#define brake  0b01111000 //set INA (PB3),INB(PB4),DIAGA/ENA(PB5),and DIAGB/ENB(PB6) to high
#define DC_CW 0b01101000 //for DC motor
#define DC_CCW  0b01110000 //for DC motor
int StepperTable[4] = {0b00110110, 0b00101110, 0b00101101, 0b00110101};

volatile unsigned int belt; //global variable for number of pieces on belt past IR/OR sensor
volatile unsigned int sort[4]; //global variable for number of sorted pieces
//sort(0) = black, sort(1) = steel, sort(2) = white, sort(3) = aluminum
volatile unsigned int nxt; //global variable for next tray position
volatile unsigned int lowest; //global variable for lowest ADC value from RL sensor
volatile unsigned int ADC_high;
volatile unsigned int ADC_low;
volatile unsigned int ADC_result;
volatile unsigned int ADC_result_flag;
volatile char STATE;
link *head;			/* The ptr to the head of the queue */
link *tail;			/* The ptr to the tail of the queue */
link *newLink;		/* A ptr to a link aggregate data type (struct) */
link *rtnLink;		/* same as the above */
Part pTest;		/* A variable to hold the aggregate data type known as element */
volatile unsigned int cur_pos;
volatile unsigned int cur_step_dir;

//set up global struct variables for blk, stl, wht, & alu
struct Part blk = {1024,975,{1,2,3,4},0};
struct Part stl = {750,210,{2,3,4,1},3};
struct Part wht = {931,760,{3,4,1,2},2};
struct Part alu = {200,0,{3,4,1,2},1};

void mTimer(int count); //initialize mTimer
void PWM( ); //initialize PWM
void CW(int NumSteps); //initialize CW for stepper
void CCW(int NumSteps); //initialize CCW for stepper

int main(void){
	CLKPR = 0x80;
	CLKPR = 0x01; //set systems clock to 8 MHz
	TCCR1B = _BV(CS11);
	
	STATE = 0;
	
	//set ports to input or output
	DDRA = 0x00; //sets PORTA to input
	DDRF = 0x00; //sets PORTF to input
	DDRC = 0xff; //set the PORTC as output
	DDRB = 0xff; //set the PORTB as output
	DDRD = 0x00; //set the PORTD as input
	
	PWM( ); //generate PWM
	InitLCD(LS_BLINK|LS_ULINE); //Initialize LCD module
	LCDClear();//Clear the screen

	cli( ); //disable interrupts
	// config the external interrupt ======================================
	EIMSK |= (_BV(INT2)); // enable INT2 (LEFT button - RAMP DOWN SYSTEM)
	EICRA |= (_BV(ISC21)); // falling edge interrupt
	EIMSK |= (_BV(INT3)); // enable INT3 (RIGHT button - PAUSE SYSTEM)
	EICRA |= (_BV(ISC31) | _BV(ISC30)); //rising edge interrupt
	EIMSK |= (_BV(INT1)); // enable INT1 (EX Sensor)
	EICRA |= (_BV(ISC11)); //falling edge interrupt
	EIMSK |= (_BV(INT4)); // enable INT4 (OR Sensor)
	EICRB |= (_BV(ISC41) | _BV(ISC40)); // rising edge interrupt
	EIMSK |= (_BV(INT5)); // enable INT5 (HE Sensor)
	EICRB |= (_BV(ISC51)); // falling edge interrupt
	
	//configure RL sensor interrupt
	ADCSRA |= _BV(ADEN); //enable ADC (RL sensor)
	ADCSRA |= _BV(ADIE); //configure ADC interrupt
	ADMUX |= _BV(REFS0); //set REFS0 to 1, selects voltage for ADC, ADLAR = 0 
	
	rtnLink = NULL;
	newLink = NULL;
	setup(&head,&tail);
	initLink(&newLink);
	newLink->p=blk;
	enqueue(&head,&tail,&newLink);
	
	//home stepper here
	//check pin D7
	cur_pos = 0;
	cur_step_dir = 1;//CCW
	LCDClear();
	LCDWriteStringXY(0,0,"Hello");
	while((PIND&0b10000000)==0b10000000){
		CCW(1);
		//mTimer(20);
	}
	//LCDClear();
	//LCDWriteStringXY(0,0,"after while");

	PORTB = DC_CCW;
		
	sei( ); //enable all interrupts
	
	goto POLLING_STAGE;

	// POLLING STATE
	POLLING_STAGE:
	switch(STATE){
		case (0) :
		goto POLLING_STAGE;
		break;
		case (1) :
		goto HOMING_STAGE; //for homing stepper
		break;
		case (2) :
		goto REFLECTIVE_STAGE; //for reading RL
		break;
		case (3) :
		goto BUCKET_STAGE; //for turning tray to correct 'home' location
		break;
		case (4) :
		goto PAUSE_STAGE;
		break;
		case (5) :
		goto END;
		default :
		goto POLLING_STAGE;
	}//switch STATE

	HOMING_STAGE: //for homing stepper
	// Do whatever is necessary HERE
	//Reset the state variable
	STATE = 0;
	goto POLLING_STAGE;

	REFLECTIVE_STAGE:
	// Do whatever is necessary HERE
	lowest = 1023;
	initLink(&newLink);
	ADCSRA |= _BV(ADSC); //starts conversion
	//Reset the state variable
	STATE = 0;
	goto POLLING_STAGE;
	
	BUCKET_STAGE:
	//must initialize with black and then turn to next
	dequeue(&head,&tail,&rtnLink);
	LCDClear();
	LCDWriteStringXY(0,0,"Line#");
	LCDWriteIntXY(0,1,head->p.line,1);
	if(head->p.line == 4){
		CW(50);
		mTimer(20); 
	}else if(head->p.line == 2){
		CCW(50);
		mTimer(20);  
	}else if(head->p.line == 3){
		if(cur_step_dir==1){
			CCW(100);
			mTimer(20); 
		}else if(cur_step_dir==0){
			CW(100);
			mTimer(20);
		} 
	}else if(head->p.line == 1){
		//stay in the same position
	}
	PORTB=DC_CCW; //start belt
	
		//belt--; //update belt count
		//if belt = 0 turn off DC motor & display final sort
		//set remove value from FIFO & update nxt
	//Reset the state variable
	free(rtnLink);
	STATE = 0;
	goto POLLING_STAGE;
	
	PAUSE_STAGE:
	mTimer(40); //debounce
	int prev = PORTB;
	if(prev==DC_CCW){
		PORTB = brake;
		mTimer(40); //debounce
		}else{
		PORTB=DC_CCW;
		mTimer(40); //debounce
	}//end of if
	//Reset the state variable
	
	STATE = 0;
	goto POLLING_STAGE;
	
	END:
	// The closing STATE ... how would you get here?
	// Stop everything here...'MAKE SAFE'
	return(0);

} //end main

ISR(INT4_vect){
	STATE = 2;
} //end ISR

ISR(INT1_vect){
	PORTB = brake; // stop belt
	STATE = 3;
} //end ISR*/

ISR(INT3_vect){
	STATE = 4;
} //end ISR

/*ISR(INT5_vect){

} //end ISR*/

ISR(ADC_vect){
	//pick lowest voltage reading (center of piece)
	ADC_low = ADCL;
	ADC_high = ADCH;
	ADC_result = (ADC_high << 8) | ADC_low;//assign value to global variable
	if(ADC_result<lowest){
		lowest = ADC_result;
	}// end if
	if((PINE&0x10)==0x10){
		ADCSRA |= _BV(ADSC);
	}else{
		if(blk.ADC_valmax>lowest && blk.ADC_valmin<lowest){
			//LCDClear();
			//LCDWriteStringXY(0,0,"BLACK");
			//LCDWriteIntXY(0,1,lowest,4);
			newLink->p = blk;
			enqueue(&head,&tail,&newLink);
		}else if(wht.ADC_valmax>lowest && wht.ADC_valmin<lowest){
			//LCDClear();
			//LCDWriteStringXY(0,0,"WHITE");
			//LCDWriteIntXY(0,1,lowest,4);
			newLink->p = wht;
			enqueue(&head,&tail,&newLink);
		}else if(alu.ADC_valmax>lowest && alu.ADC_valmin<lowest){
			//LCDClear();
			//LCDWriteStringXY(0,0,"ALUMINUM");
			//LCDWriteIntXY(0,1,lowest,4);
			newLink->p = alu;
			enqueue(&head,&tail,&newLink);
		}else if(stl.ADC_valmax>lowest && stl.ADC_valmin<lowest){
			//LCDClear();
			//LCDWriteStringXY(0,0,"STEEL");
			//LCDWriteIntXY(0,1,lowest,4);
			newLink->p = stl;
			enqueue(&head,&tail,&newLink);
		}else{
			LCDClear();
			LCDWriteStringXY(0,0,"UNDETERMINED");
			LCDWriteIntXY(0,1,lowest,4);
		}// end if/else
	}// end if/else
} //end ISR

ISR(BADISR_vect){
	//handle bad ISR triggers
} //end ISR*/

void mTimer(int count){
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
   //TIMSK1 = TIMSK1 | 0b00000010;

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
} //end mTimer

void PWM( ){
	TCCR0A |= _BV(WGM01)|_BV(WGM00); //selecting Fast PWN mode 3
	//TIMSK0 |= _BV(OCIE0A); //enable output compare interrupt for timer0
	TCCR0A |= _BV(COM0A1);//set compare match output mode to clear and set output compare A when timer reaches TOP
	TCCR0B |= _BV(CS01);//sets prescale factor to 8
	OCR0A = 0x80;//default duty cycle
} //end PWM

void CW(int NumSteps ){
	int i = 0;
	while(i<NumSteps){
		cur_pos++;
		if(cur_pos==4){
			cur_pos=0;
		}
		PORTA = StepperTable[cur_pos];
		mTimer(20);
		i++;
	} //end while
	cur_step_dir =0;
} //end CW

void CCW(int NumSteps ){
	int i = 0;
	while(i<NumSteps){
		cur_pos--;
		if(cur_pos==-1){
			cur_pos=3;
		}
		PORTA = StepperTable[cur_pos];
		mTimer(20);
		i++;
	}
	cur_step_dir =1;
} //end CCW





