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
volatile int wartosc;	//przechowuje zadawan¹ wartoœæ sygna³u steruj¹cego
volatile bool normalna_praca = false;
volatile int analog[ILE_ADC];

void init_timer(void)
{
	TCCR0A = (1<<WGM01);
	TCCR0B = (1<<CS00)|(1<<CS01);	//preskaler 64, tryb CTC
	TIMSK0 |= (1<<OCIE0A);			//przerwanie przy doliczeniu do OCR0A
	OCR0A = 1;
	// przerwanie nast¹pi co (64)/16MHz = 4us
	// Dlaczego 4us? 1ms/250 = 4, co daje rozdzielczoœæ 1/250
	// Dodatkowo co 20ms trzeba wyzerowaæ wyjœcia

}

void init_interrupts(void)
{
	EICRA |= (1<<ISC11)|(1<<ISC00)|(1<<ISC01);
	//zbocze opadaj¹ce na INT1 (D3, 3 - przycisk powrotu do pracy)
	//zbocze narastaj¹ce na INT1 (D2, 2 - zrywka)
	
	EIMSK |= (1<<INT1)|(1<<INT0);	//w³¹czenie przerwañ dla tych pinów	
}

void init_adc(void)
{
	ADMUX |= (1<<REFS0);	//napiêcie odniesienia na AVcc, kondensator na Aref
	ADCSRA |= (1<<ADEN);	//w³¹czenie przetwornika ADC
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

//przerwanie od timera co 4us (generowanie PWM)
ISR(TIMER0_COMPA_vect)
{
	if(normalna_praca)
	{
		if(licznik>250/9)					//minê³o przynajmniej 1ms
		{
			if(licznik-wartosc>=25)	//minê³o 1+(wartoœæ/250)ms
			{
				PORTB &= ~(1<<PB1);					//stan niski na konkretnym wyjœciu
			}
		}
		if(licznik >= 500)	//minê³o 20ms
		{
			licznik = 0;
			PORTB |= (1<<PB1);	//wpisz 1 na wszystkie wyjœcia
			
			add_measure(ADCL | (ADCH<<8));	//odczytanie wartoœci z ADC
			wartosc = get_analog();	//przeliczenie œredniej
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
	//wyjœcia serwowe
    DDRB |= (1<<PB1);	//wyjœcie na serwo
	PORTD |= (1<<PD2)|(1<<PD3);	//pullup na zrywkê i przycisk
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