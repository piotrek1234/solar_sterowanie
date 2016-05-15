/*
 * solar_sterowanie.c
 *  Author: Piotrek
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

typedef enum{ false = 0, true = !false } bool;
#define ILE_ADC 10

#define SERVO_MIN 200
#define SERVO_MAX 600
volatile uint8_t licznik_adc = 0;
volatile int wartosc;	//przechowuje zadawan¹ wartoœæ sygna³u steruj¹cego
volatile bool normalna_praca = false;
volatile int analog[ILE_ADC];

#define BAUD 9600
#define ubrr F_CPU/16/BAUD-1

void init_timer(void)
{
	TCCR1A|=(1<<COM1A1)|(1<<COM1B1)|(1<<WGM11);        //NON Inverted PWM
	TCCR1B|=(1<<WGM13)|(1<<WGM12)|(1<<CS11)|(1<<CS10); //PRESCALER=64 MODE 14(FAST PWM)

	ICR1=4999;  //fPWM=50Hz (Period = 20ms Standard).

	DDRB|=(1<<PB1);   //PWM Pins as Out

}

void uart( unsigned char data )
{
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) )
	;
	/* Put data into buffer, sends the data */
	UDR0 = data;
}

void init_interrupts(void)
{
	EICRA |= (1<<ISC11)|(1<<ISC00)|(1<<ISC01);
	//zbocze opadaj¹ce na INT1 (D3, 3 - przycisk powrotu do pracy)
	//zbocze narastaj¹ce na INT0 (D2, 2 - zrywka)
	
	EIMSK |= (1<<INT1)|(1<<INT0);	//w³¹czenie przerwañ dla tych pinów	
}

void init_adc(void)
{
	ADMUX |= (1<<REFS0)|(1<<ADLAR);	//napiêcie odniesienia na AVcc, kondensator na Aref
	ADCSRA |= (1<<ADEN)|(1<<ADPS1)|(1<<ADPS2);	//w³¹czenie przetwornika ADC
	//przemyœleæ ustawianie czêstotliwoœci, ADPSx
}

void add_measure(int value)
{
	analog[licznik_adc] = value;
	licznik_adc++;
	if(licznik_adc >= ILE_ADC)
		licznik_adc = 0;
}

int get_analog(void)
{
	int ret=0;
	for(int i=0; i<ILE_ADC; ++i)
	{
		ret += analog[i];
	}
	ret /= ILE_ADC;
	
	return ret;
}

void test_zrywki(void)
{
	if(!(PIND & PD2))
	{
		normalna_praca = true;
		uart('+');
	}
	else
	{
		normalna_praca = false;
		uart('-');
	}
}

//przerwanie od zrywki
ISR(INT0_vect)
{
	normalna_praca = false;
	uart('z');
}

//przerwanie od przycisku powrotu do pracy
ISR(INT1_vect)
{
	test_zrywki();
	uart('j');
}

int rescale(int in_left, int in_right, int out_left, int out_right, int value)
{
	return out_left+(out_right-out_left)*(value-in_left)/(in_right-in_left);
}

int main(void)
{
	UBRR0H = (unsigned char)((ubrr)>>8);
	UBRR0L = (unsigned char)(ubrr);
	UCSR0B = (1<<TXEN0);
	UCSR0C = (3<<UCSZ00);
	
	//wyjœcia serwowe
    //DDRB |= (1<<PB1);	//wyjœcie na serwo
	PORTD |= (1<<PD2)|(1<<PD3);	//pullup na zrywkê i przycisk
	
    init_interrupts();
	init_adc();
    init_timer();
	
	test_zrywki();
	
    sei();
	ADCSRA |= (1<<ADSC);
	
    while(1)
    {
	   while( ADCSRA & (1<<ADSC) );
	   add_measure(ADCH);
	   wartosc = get_analog();
	   uart(wartosc);
	   if(normalna_praca)
			OCR1A = rescale(0,255,SERVO_MIN, SERVO_MAX, wartosc);
	   else
			OCR1A = 0;
	   ADCSRA |= (1<<ADSC);
	   _delay_ms(200);
	   
    }
}