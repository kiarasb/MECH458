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
int prof_50[51];
int prof_100[101];

volatile unsigned int belt; //global variable for number of pieces on belt past IR/OR sensor
volatile unsigned int sort[4]; //global variable for number of sorted pieces
//sort(0) = black, sort(1) = steel, sort(2) = white, sort(3) = aluminum
volatile unsigned int nxt; //global variable for next tray position
volatile unsigned int step_pos; //global variable for current tray position
volatile unsigned int lowest; //global variable for lowest ADC value from RL sensor
volatile unsigned int ADC_high;
volatile unsigned int ADC_low;
volatile unsigned int ADC_result;
volatile unsigned int ADC_result_flag;
volatile unsigned int prev;
volatile unsigned int num_belt= 0; //number of parts left on belt
int sorted[4] = {0, 0, 0, 0};
volatile unsigned int rampdown = 0;
	
volatile char STATE;
link *head;			/* The ptr to the head of the queue */
link *tail;			/* The ptr to the tail of the queue */
link *newLink;		/* A ptr to a link aggregate data type (struct) */
link *rtnLink;		/* same as the above */
Part pTest;		/* A variable to hold the aggregate data type known as element */
volatile unsigned int cur_pos;
volatile unsigned int cur_step_dir;

void mTimer(unsigned int count); //initialize mTimer
void stopTimer(); // initialize stopTimer
void PWM( ); //initialize PWM
void CW(int NumSteps); //initialize CW for stepper
void CCW(int NumSteps); //initialize CCW for stepper

int main(void){
	CLKPR = 0x80;
	CLKPR = 0x01; //set systems clock to 8 MHz
	TCCR1B = _BV(CS11);
	
	STATE = 0;
	
	//define stepper profiles
	prof_50[0]= 20000;
	prof_100[0]= 20000;
	float slope_50 = 1500;
	float slope_100= 750;
	for(int i=1;i<51;i++){
		if(i<11){
			prof_50[i]= prof_50[i-1]-slope_50;
		}else if(i<40){
			prof_50[i]= prof_50[i-1];
		}else if(i>39){
		prof_50[i]= prof_50[i-1]+slope_50;
		}
	}
	for(int i=1;i<101;i++){
		if(i<21){
			prof_100[i]= prof_100[i-1]-slope_100;
			}else if(i<80){
			prof_100[i]= prof_100[i-1];
			}else if(i>79){
			prof_100[i]= prof_100[i-1]+slope_100;
		}
	}
	
	//set ports to input or output
	DDRA = 0x00; //sets PORTA to input
	DDRF = 0x00; //sets PORTF to input
	DDRC = 0xff; //set the PORTC as output
	DDRB = 0xff; //set the PORTB as output
	DDRD = 0x00; //set the PORTD as input
	
	PWM( ); //generate PWM
	InitLCD(LS_BLINK|LS_ULINE); //Initialize LCD module
	LCDClear();//Clear the screen
	LCDWriteString("hello");
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

	
	//configure RL sensor interrupt
	ADCSRA |= _BV(ADEN); //enable ADC (RL sensor)
	ADCSRA |= _BV(ADIE); //configure ADC interrupt
	ADMUX |= _BV(REFS0); //set REFS0 to 1, selects voltage for ADC, ADLAR = 0 
	
	rtnLink = NULL;
	newLink = NULL;
	setup(&head,&tail);
	initLink(&newLink);
	
	step_pos = 0; //inital stepper position
	
	sei( ); //enable all interrupts
	
	STATE=1;
	
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
	cur_pos = 0;
	cur_step_dir = 1;//CCW
	while((PIND&0b10000000)==0b10000000){
		CCW(1);
		mTimer(20000);
	}
	int center = 0;
	while(center < 4){
		CCW(1);
		mTimer(20000);
		center++;
	}
	PORTB = DC_CCW;
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
	num_belt--;
	nxt = rtnLink->p.cur_pos;
	sorted[rtnLink->p.cur_pos]++;
	int diff = step_pos-nxt;
	if(diff == 1|| diff==-3){
		CW(50);
		mTimer(10000); 
	}else if(diff == -1|| diff == 3){
		CCW(50);
		mTimer(10000);  
	}else if(diff==2||diff==-2){
		if(cur_step_dir==1){
			CCW(100);
			mTimer(10000); 
		}else if(cur_step_dir==0){
			CW(100);
			mTimer(10000);
		} 
	}else if(diff == 0){
		//stay in the same position
	}
	step_pos = rtnLink->p.cur_pos;
	PORTB=DC_CCW; //start belt
	//Reset the state variable
	free(rtnLink);
	STATE = 0;
	goto POLLING_STAGE;
	
	PAUSE_STAGE:
	prev = PORTB;
	if(prev==DC_CCW){
		PORTB = brake;
		LCDClear();
		LCDWriteStringXY(0,0,"BELT");
		LCDWriteStringXY(5,0,"BL");
		LCDWriteStringXY(8,0,"ST");
		LCDWriteStringXY(11,0,"AL");
		LCDWriteStringXY(14,0,"WT");
		LCDWriteIntXY(0,1,num_belt,2);
		LCDWriteIntXY(5,1,sorted[0],2);
		LCDWriteIntXY(8,1,sorted[1],2);
		LCDWriteIntXY(11,1,sorted[3],2);
		LCDWriteIntXY(14,1,sorted[2],2);
		
		}else{
		PORTB=DC_CCW;
		LCDClear();
	}//end of if
	//Reset the state variable
	
	STATE = 0;
	goto POLLING_STAGE;
	
	END:
	PORTB = disable;
	LCDClear();
	LCDWriteStringXY(0,0,"BELT");
	LCDWriteStringXY(5,0,"BL");
	LCDWriteStringXY(8,0,"ST");
	LCDWriteStringXY(11,0,"AL");
	LCDWriteStringXY(14,0,"WT");
	LCDWriteIntXY(0,1,0,2);
	LCDWriteIntXY(5,1,sorted[0],2);
	LCDWriteIntXY(8,1,sorted[1],2);
	LCDWriteIntXY(11,1,sorted[3],2);
	LCDWriteIntXY(14,1,sorted[2],2);
	
	cli( ); //disable interrupts
	return 0;

} //end main

ISR(INT4_vect){
	STATE = 2;
} //end ISR

ISR(INT1_vect){
	if(isEmpty(&head)==1){
		STATE = 0;
	}else if(isEmpty(&head)==0){
		PORTB = brake; // stop belt
		STATE = 3;
	}
} //end ISR*/

ISR(INT3_vect){
	mTimer(20000); //debounce
	STATE = 4;
	while((PIND&0x08)==0x08);
	mTimer(20000); //debounce
} //end ISR

ISR(INT2_vect){ //rampdown
	mTimer(20000); //debounce
	//call timer function to start rampdown count
	stopTimer();
	//while((PIND&0x04)==0x04);
	//mTimer(20000); //debounce
	STATE = 0; // go to polling state
} //end ISR*/
	
ISR(TIMER3_COMPA_vect){
	LCDClear();
	LCDWriteString("TIMER END");
	STATE = 5;
}//end ISR

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
		if(100>lowest){
			LCDClear();
			LCDWriteStringXY(0,0,"ALUMINUM");
			LCDWriteIntXY(0,1,lowest,4);
			newLink->p.cur_pos = 3; //alu
			enqueue(&head,&tail,&newLink);
			num_belt++;
			return;
		}else if(650>lowest){
			LCDClear();
			LCDWriteStringXY(0,0,"STEEL");
			LCDWriteIntXY(0,1,lowest,4);
			newLink->p.cur_pos = 1; //stl
			enqueue(&head,&tail,&newLink);
			num_belt++;
			return;
		}else if(930>lowest){
				LCDClear();
				LCDWriteStringXY(0,0,"WHITE");
				LCDWriteIntXY(0,1,lowest,4);
				newLink->p.cur_pos = 2; //wht
				enqueue(&head,&tail,&newLink);
				num_belt++;
				return;
		}else if(1024>lowest){
			LCDClear();
			LCDWriteStringXY(0,0,"BLACK");
			LCDWriteIntXY(0,1,lowest,4);
			newLink->p.cur_pos = 0; //blk
			enqueue(&head,&tail,&newLink);
			num_belt++;
			return;
		}
	}// end if/else
} //end ISR

ISR(BADISR_vect){
	//handle bad ISR triggers
} //end ISR
	
void stopTimer (){
	/***
      Setup Timer3 as a s timer
	  Using Interrupt Driven Method
   ***/
	TCCR3B |= _BV(WGM32); //set to CTC mode
	
	//set prescale clock /1024
	TCCR3B |=_BV (CS30)|_BV (CS32);
	
	//set compare register to 46875 cycles, 6s
	OCR3A = 0xB71B;
	
	/* Initialize Timer2 to zero */
	TCNT3 = 0x0000;
	
	//turn on timer compare A interrupt flag
	TIMSK3 = TIMSK3 | 0b00000010;
}


void mTimer(unsigned int count){
	/***
      Setup Timer1 as a ms timer
	  Using polling method not Interrupt Driven
   ***/
  
   /* Set the Waveform gen. mode bit description to clear
     on compare mode only */
   TCCR1B |= _BV(WGM12); 

   OCR1A =count; // Timer value for 0.001ms resolution
   
   /* Clear the Timer1 interrupt flag and begin timing */
   TIFR1 |= _BV(OCF1A);

   /* Initialize Timer1 to zero */
   TCNT1 = 0x0000;

   /* Enable the output compare interrupt */
   //TIMSK1 = TIMSK1 | 0b00000010;



   /* Poll the timer to determine when the timer has reached 1ms */
   
   while((TIFR1 & 0x02) != 0x02);
   TIFR1 |= _BV(OCF1A);
	   
   return;
} //end mTimer

void PWM( ){
	TCCR0A |= _BV(WGM01)|_BV(WGM00); //selecting Fast PWN mode 3
	//TIMSK0 |= _BV(OCIE0A); //enable output compare interrupt for timer0
	TCCR0A |= _BV(COM0A1);//set compare match output mode to clear and set output compare A when timer reaches TOP
	TCCR0B |= _BV(CS01);//sets prescale factor to 8
	OCR0A = 196;//default duty cycle
	/*if(isEmpty(&head)==1){
		OCR0A = 220;
		}else{
		OCR0A = 200;//default duty cycle
		}*/
	
} //end PWM

//for implementing acceleration, use mTimer (we do not need two new timers)
//then you will need to create 2 matrices for 50 steps and for 100 steps
//these matrices will include the amount of time between each step
//example:
//while (i<=50)
//CW(1)
//mTimer(matrix index)
//i++
//slopes will be the same for 50 steps and for 100 steps
//must find the max speed of the stepper, this is when the stepper shuts down
//the max speed is not 0ms, 1ms, or 2ms, the min speed is 20ms

void CW(int NumSteps ){
	int i = 0;
	while(i<NumSteps){
		cur_pos++;
		if(cur_pos==4){
			cur_pos=0;
		}
		PORTA = StepperTable[cur_pos];
		if(NumSteps==50){
			mTimer(prof_50[i]);
		}else if(NumSteps==100){
			mTimer(prof_100[i]);
		}
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
		if(NumSteps==50){
			mTimer(prof_50[i]);
			}else if(NumSteps==100){
			mTimer(prof_100[i]);
		}
		i++;
	}
	cur_step_dir =1;
} //end CCW





