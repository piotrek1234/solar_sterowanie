/*
 * solar_sterowanie.c
 *  Author: Piotrek
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

typedef enum{ false = 0, true = !false } bool;
#define ILE_ADC 10

volatile int licznik = 0;
volatile uint8_t licznik_adc = 0;
volatile int wartosc;	//przechowuje zadawan� warto�� sygna�u steruj�cego
volatile bool normalna_praca = false;
volatile int analog[ILE_ADC];

void init_timer(void)
{
	TCCR0A = (1<<WGM01);
	TCCR0B = (1<<CS00)|(1<<CS01);	//preskaler 64, tryb CTC
	TIMSK0 |= (1<<OCIE0A);			//przerwanie przy doliczeniu do OCR0A
	OCR0A = 1;
	// przerwanie nast�pi co (64)/16MHz = 4us
	// Dlaczego 4us? 1ms/250 = 4, co daje rozdzielczo�� 1/250
	// Dodatkowo co 20ms trzeba wyzerowa� wyj�cia

}

void init_interrupts(void)
{
	EICRA |= (1<<ISC11)|(1<<ISC00)|(1<<ISC01);
	//zbocze opadaj�ce na INT1 (D3, 3 - przycisk powrotu do pracy)
	//zbocze narastaj�ce na INT1 (D2, 2 - zrywka)
	
	EIMSK |= (1<<INT1)|(1<<INT0);	//w��czenie przerwa� dla tych pin�w	
}

void init_adc(void)
{
	ADMUX |= (1<<REFS0);	//napi�cie odniesienia na AVcc, kondensator na Aref
	ADCSRA |= (1<<ADEN);	//w��czenie przetwornika ADC
	//przemy�le� ustawianie cz�stotliwo�ci, ADPSx
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

//przerwanie od timera co 4us (generowanie PWM)
ISR(TIMER0_COMPA_vect)
{
	if(normalna_praca)
	{
		if(licznik>250/9)					//min�o przynajmniej 1ms
		{
			if(licznik-wartosc>=25)	//min�o 1+(warto��/250)ms
			{
				PORTB &= ~(1<<PB1);					//stan niski na konkretnym wyj�ciu
			}
		}
		if(licznik >= 500)	//min�o 20ms
		{
			licznik = 0;
			PORTB |= (1<<PB1);	//wpisz 1 na wszystkie wyj�cia
			
			add_measure(ADCL | (ADCH<<8));	//odczytanie warto�ci z ADC
			wartosc = get_analog();	//przeliczenie �redniej
			ADCSRA |= (1<<ADSC);	//uruchomienie pomiaru ADC
		}
		licznik++;
	}
	else
		PORTB &= ~(1<<PB1);
}

void test_zrywki(void)
{
	if(!(PIND & PD2))
	{
		normalna_praca = true;
	}
	else
	{
		normalna_praca = false;
	}
}

//przerwanie od zrywki
ISR(INT0_vect)
{
	normalna_praca = false;
}

//przerwanie od przycisku powrotu do pracy
ISR(INT1_vect)
{
	test_zrywki();
}

int main(void)
{
	//wyj�cia serwowe
    DDRB |= (1<<PB1);	//wyj�cie na serwo
	PORTD |= (1<<PD2)|(1<<PD3);	//pullup na zrywk� i przycisk
    init_interrupts();
	init_adc();
    init_timer();
	
	test_zrywki();
	
    sei();
    
    while(1)
    {
	    //nic
    }
}