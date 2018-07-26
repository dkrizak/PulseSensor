/*
 * pulsefinaltest.c
 *
 * Created: 3.9.2016. 23:02:53
 *  Author: John
 */ 


#define F_CPU	 7372800UL
#define F_PWM		 1000U			/* PWM freq ~ 1kHz    */
#define N			 8
#define TOP			 (((F_CPU/F_PWM)/N)-1)
#define CONTRAST_DEF TOP/3

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "lcd.h"

//!init adc
void adc_init()
{
	// AREF = AVcc
	ADMUX = (1<<REFS0);
	
	// ADC Enable and prescaler of 128
	// 7372800/128 = 57600
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

//! read adc value
uint16_t adc_read(uint8_t ch)
{
	
	ADMUX = (ADMUX & 0xF8)|ch;     // clears the bottom 3 bits before ORing
	
	// start single conversion
	// write '1' to ADSC
	ADCSRA |= (1<<ADSC);
	
	// wait for conversion to complete
	// ADSC becomes '0' again
	// till then, run loop continuously
	while(ADCSRA & (1<<ADSC));
	
	return (ADC);
}


//puls varijable
//!adc pulse value
uint16_t puls;
//!heartbeat counter
uint8_t beat=0;
//!heartbeat average
uint8_t beatp=0;
//!heartbeat average in 5 sec interval
uint16_t beatsum=0;
//!signal amplitude average
uint16_t avg=550;
//!max amplitude
uint16_t max=0;
//!average amplitude
uint16_t amp=0;
//!min amplitude
uint16_t min=900;
int8_t i=0;
uint8_t j=0;
uint8_t z=0;
//!type of menu
uint8_t menu=0;
//!second counter
uint16_t sec=0;
//!heartbeat flag
uint8_t flag=0;
//!1 second counter 40 * 25ms = 1 sec
uint8_t cnt=40;
//!5 sec period
uint16_t period=200;
//!period counter
uint8_t n=1;
char buffer[10];
//!heart sign string
unsigned char srce[8]={0x00, 0x0a, 0x15, 0x11, 0x11, 0x0a, 0x04, 0x00};

//sat varijable
//!ms counter for time display
uint16_t milisek = 0;
//!second counter for time display
uint8_t sek = 0;
//!detects 1 min stop counting
uint8_t minuta = 1;

//!debounce varible
int Pressed = 0;
//!Measure button press cofidence
int Pressed_Confidence_Level = 0; 
//!Measure button release confidence
int Released_Confidence_Level = 0; 

//!debounce check function
int debounce(void){
	while (1){
		if (bit_is_clear(PINB, 0)){
			Pressed_Confidence_Level ++; //Increase Pressed Conficence
			Released_Confidence_Level = 0; //Reset released button confidence since there is a button press
			if (Pressed_Confidence_Level >250) //Indicator of good button press
			{
				if (Pressed == 0)
				{
					Pressed = 1;
				}
				//Zero it so a new pressed condition can be evaluated
				Pressed_Confidence_Level = 0;
				return 1;
			}
		}
		else
		{
			Released_Confidence_Level ++; //This works just like the pressed
			Pressed_Confidence_Level = 0; //Reset pressed button confidence since the button is released
			if (Released_Confidence_Level >250){
				Pressed = 0;
				Released_Confidence_Level = 0;
				return 0;
			}
		}
	}
}

int main(void)
{	
	DDRD |= _BV(PD4);
	PORTD |= _BV(PD4);
	
	DDRB |= 0<<PB0;
	PORTB |= 1<<PB0;
	
	TCCR1A |= _BV(COM1B1)|_BV(WGM11)|_BV(WGM10);
	TCCR1B |= _BV(WGM13)|_BV(WGM12)|_BV(CS11);
	OCR1A   = TOP;				/* set TOP value */
	OCR1B   = CONTRAST_DEF;		/* set default contrast value */
	
	// initialize adc and lcd
	adc_init();
	lcd_init(LCD_DISP_ON);
	lcd_clrscr();
	
	// init srce
	lcd_command(0x40);
	for (i=0; i<8; i++){
		lcd_data(srce[i]);
	}
	i=0;
	
	// display the labels on LCD
	lcd_home();
	lcd_puts("Puls ");
	
	_delay_ms(50);
	
	TCCR0 |= _BV(WGM01) | _BV(CS02) | _BV(CS00);
	OCR0 = 180;
	
	char poruka[] = {"Welcome to pulse meter!!!"};
		
    while(1)
    {
		//MENU
        for (i=15; i>=0; i--){
			lcd_clrscr();
			lcd_gotoxy(i, 0);
			lcd_puts(poruka);
			lcd_gotoxy(3,1);
			if (z%3 == 0) lcd_puts_P("Press key1");
			z++;
			_delay_ms(200);
			if (bit_is_clear(PINB, 0)){ 
				menu=1;
				break;
			}
			
			if (i==0){
				for (j=1; j<=(strlen(poruka)); j++){
					lcd_clrscr();
					lcd_home();
					while (poruka[i+j] != '\0'){
						lcd_putc(poruka[i+j]);
						i++;
					}
					i=0;
					lcd_gotoxy(3,1);
					if (z%3 == 0) lcd_puts_P("Press key1");
					z++;
					_delay_ms(200);
					if (bit_is_clear(PINB, 0)){
						menu=1;
						break;
					}
				}
				_delay_ms(200);
			}
		}

		while(menu){
			lcd_clrscr();
			lcd_home();
			lcd_puts_P("Start/Stop/Reset");
			lcd_gotoxy(3,1);
			if (z%3==0) lcd_puts_P("Press key1");
			z++;
			_delay_ms(200);
			
			if (bit_is_clear(PINB, 0)){
				
				while (debounce()!=0);
				
				TIMSK |= _BV(OCIE0);
				sei();
				menu=0;			
			}
			
			while (menu == 0){
				if (bit_is_clear(PINB, 0)){
					
					while (debounce()!=0);
					
					cli();
					menu=2;	
	
				}
			}
			
			while (menu == 2){
				if (bit_is_clear(PINB, 0)){
			
					while (debounce()!=0);
					//reset
					beat=0;
					beatp=0;
					beatsum=0;
					avg=550;
					max=0;
					amp=0;
					min=900;
					i=0;
					sec=0;
					flag=0;
					n=1;	

					milisek = 0;
					sek = 0;
					minuta = 1;
					menu=1;
				}
			}
			
		}
    }
}

//!25ms timer
ISR(TIMER0_COMP_vect){
	
	puls = adc_read(0);      // read adc value at PA0
		
	// signal processing
		
	if (puls>max) max=puls;
		
	if (puls<min) min=puls;
		
	i++;
	sec++;
		
	if (i==cnt){
		avg=max-((max-min)/2);
		amp=(max-min)/4;
		i=0;
		max=0;
		min=900;
	}
		
	if ((puls>(avg+amp)) && flag==0){
		beat++;
		flag=1;
	}
	
	if ((puls<(avg-amp)) && flag==1){
		flag=0;
	}
		
	if (sec==period){
		beatsum+=beat*12;
		beatp=beatsum/n;
		sec=0;
		beat=0;
		n++;
	}
		
	//sat
	milisek++;
	if (milisek==40){
		sek++;
		milisek=0;
	}

	if (minuta){	
		
		//pulse display
		lcd_clrscr();
		lcd_home();
		lcd_puts("Pulse ");
		
		if (puls<(avg+amp)){
			lcd_gotoxy(7,0);
			lcd_puts_P(" ");
		}
		else {
			lcd_gotoxy(7,0);
			lcd_data(0);
		}

		lcd_gotoxy(0,1);
		lcd_puts("Time");
		
		itoa(beatp, buffer, 10);
		lcd_gotoxy(13,0);
		lcd_puts(buffer);
		
		//time display
		itoa(sek, buffer, 10);
		lcd_gotoxy(10,1);
		if (sek<10) lcd_puts_P("0");
		lcd_puts(buffer);
		lcd_puts_P(":");
		if ((milisek*25)/10 < 10) lcd_puts_P("0");
		itoa((milisek*25)/10, buffer, 10);
		lcd_puts(buffer);
	
		if (sek==60 && milisek==0) minuta=0;		
	}
	
}