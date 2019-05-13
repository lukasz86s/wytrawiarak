/*
 * main.c
 *
 *  Created on: 24 maj 2017
 *      Author: fet
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr\pgmspace.h>
#include <util/delay.h>

#include "LCD/lcd44780.h"
#include "1Wire/ds18x20.h"

#define grzalka (1<<PB2)				// pin sterujacy grzalka
#define grzalka_OFF PORTB &= ~grzalka  // wylaczenkie grzalki
#define grzalka_ON  PORTB |=  grzalka	//wl¹czenie grza³ki

// stale do obsugi klawidzy
#define klawisz1 (1<<PB3)
#define klawisz2 (1<<PB4)
#define klawisz3 (1<<PB5)
#define key_mask (klawisz1|klawisz2|klawisz3)
#define pauza 20									// dlugosc pauzy x 10ms

void display_temp(uint8_t x);			//zmienna do wyswietlania pomiarow
void heating (void);					// deklaracja funkcji do nagrzewania
void mieszanie(void);					//deklaracja funkcji mieszania
uint8_t i,x ;
struct{									// struktury do obslugi menu 1 i 2 pozinmu
	uint8_t menu:4;
	uint8_t podMenu:1;
	uint8_t start:1;
}idx;
uint8_t regTemp;						// zmienna do regulacji temperatury
uint8_t przemieszaj;					// zmienna okreslajaca czestotliwosc mieszania
uint8_t czujnik_cnt;					// liczna podlaczanych czujnkikow
volatile uint8_t s1_flag , ms330_flag;				// flaga sekund i 330ms
volatile uint8_t sekundy;				// zmienna odliczajaca sekundy
volatile uint8_t ms10,odliczanie;
uint8_t * wskZmienna;
uint8_t subzero, cel, cel_fract_bits;	// nosniki danych z czujnika
enum menu{menuG, temp, dodatkowe};		// typy wyliczeniowe

int main (void){
	DDRB &= ~(klawisz1|klawisz2|klawisz3);	// ustawienie pingow klawiszy jako wejscia
	PORTB |= (klawisz1|klawisz2|klawisz3);	// ustawienie stanu wysokiego na klawiszach

	DDRB |= grzalka;						// ustawienie pinu sterujacego grza³ka jako wyjscie
	grzalka_OFF;							// ustawienie pinu grzalki w stan niski


	////////////////////////////////////////////////////////////////////////////////////////////////////////
		/* ustawienie fast PWM dla timera 1 10bit */
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	TCCR1A |= ((1<<WGM12)|(1<<WGM11)|(1<<WGM10)|(1<<COM1A1));			//ustaw 10 bit i clear on top
	TCCR1B |= (1<<CS12);												//preskaler 256
	DDRB |= (1<<PB1);													// ustaw jako wyjscie pin oc1a

	OCR1A = 20;


	/* ustawienia timer dla cpu =8mzh*/
	TCCR2|= (1<<WGM21);							// tryb CTC
	TCCR2|= ((1<<CS20)|(1<<CS21)|(1<<CS22));	// preskaler 1024;
	OCR2 = 77; 									// dodatkowy podzial przez 78 bo 0 tez sie liczy
	TIMSK |= (1<<OCIE2);						// zezwolenie na przerawania
	sei();			// zezwolenie na globalne przerwania





// sprawdzanie ile czujnikow jest podpietych
czujnik_cnt = search_sensors();
//wysylamy rozkaz pomiaru wszystkich czujnikow
//DS18X20_start_meas(DS19X20_POWE_EXTERN,NULL);
//_delay_ms(750);

// ustawienie wartoœci pocz¹tkowych
przemieszaj = 5;
regTemp = 40;
idx.menu = menuG;
idx.podMenu = 0;
idx.start = 0;
odliczanie = 0;
x=0;

lcd_init();
lcd_locate(0,0);
lcd_str_P(PSTR("temp."));
lcd_locate(1,0);
lcd_str("stop");

while(1){
	if((PINB & key_mask) != key_mask)
	{					        // jesli ktorykolwiek klawisz wcisniety
		odliczanie = 1;										    // likwidacja drgan stykow
		if((ms10 == pauza) && ((PINB & key_mask)!= key_mask))
		{
//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////									//////////////////////////////////////////////////
//////////// 		1 KLAWISZ					//////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
			if(!(PINB & klawisz1))
			{
				if(idx.podMenu == 0)
				{
					++idx.menu;
					i = 1;
					if(idx.menu==3)idx.menu = menuG;
				}
				if(idx.podMenu == 1)
				{
					if(idx.menu == menuG)++idx.start; 		// wlaczanie lub wylaczanie jesli jest w pozycji menug³ownego
					else ++*wskZmienna;


				}
			}
//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////									//////////////////////////////////////////////////
//////////// 		2 KLAWISZ					//////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
			if(!(PINB & klawisz2))
			{
				idx.podMenu^= 1;

			}
//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////									//////////////////////////////////////////////////
//////////// 		3 KLAWISZ					//////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
			if(!(PINB & klawisz3))
			{
				if(idx.podMenu == 0)
				{
					--idx.menu;
					i = 1;

					if(idx.menu==15)idx.menu = dodatkowe;
				}
				if(idx.podMenu == 1)
				{
					if(idx.menu == menuG)--idx.start; 		// wlaczanie lub wylaczanie jesli jest w pozycji menug³ownego
					else --*wskZmienna ;					// dodawanie wartosci do pobranej zminnej



				}
			}
			ms10 = 0;
		}

	}// klamra konczaca funkcje klawiszy
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////     szukanie sensorow i start pomiaru dla nich   		////////////////////////////////////
//////////////////// jesli warunki spelnione zalcz grzalke
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if(s1_flag == 1)
	{
		if(0 == (sekundy%3))czujnik_cnt = search_sensors();					// szukanie sensorow
		if(1 == (sekundy%3))DS18X20_start_meas(DS18X20_POWER_EXTERN,NULL);	// start pomiaru dla wszystkich sensorów
		if(2 == (sekundy%3))
		{
			if(idx.start == 1){heating();}
			else grzalka_OFF;
		}
		// jesli wlaczone menu g³ówne odczyt i wyswietlenie zostanie przeprowadzone w nim
		// w innym wypatku zostanie przeprowadzone teraz bez wyswietlania
		if(idx.menu == menuG)s1_flag =1;
		else
		{
			if(2 == sekundy%3)DS18X20_read_meas(gSensorIDs[0], &subzero, &cel, &cel_fract_bits);
			s1_flag = 0;

		}
	}
	if(idx.start == 1){
		mieszanie();
	}

////////////////////////////////////////////////////////////////////////////////////////////////
///////////																////////////////////////
//////////			1. MENU PIERWSZY EKRAN								////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
if(idx.menu == menuG)
 {
	if(i){
			lcd_cls();
			lcd_locate(0,0);
			lcd_str("temp.");
			lcd_locate(0,15);
			lcd_str("C");
			i = 0;

	     }
	if(s1_flag)
	{
	//
			//if(2 == (sekundy%3))
			//{
						// odczyt temperwatury z czujnika
				if( DS18X20_OK == DS18X20_read_meas(gSensorIDs[0], &subzero, &cel, &cel_fract_bits) ) display_temp(11);
				else
				{
					lcd_locate(0,6);
					lcd_str("error");
				}
			//}
	s1_flag = 0;
	}
	if(idx.podMenu == 1)
		if(ms330_flag)
		{	lcd_locate(1,6);
			lcd_str("<");
			if(idx.start == 0){
				lcd_locate(1,0);
				lcd_str("stop ");
			}
				else if(idx.start == 1){
					lcd_locate(1,0);
					lcd_str("start");
				}
			ms330_flag = 0;
		}
	if(idx.podMenu == 0)
		{
			if(ms330_flag)
			{
			lcd_locate(1,6);
			lcd_str(" ");
			if(idx.start == 0){
							lcd_locate(1,0);
							lcd_str("stop ");
						}
							else if(idx.start == 1){
								lcd_locate(1,0);
								lcd_str("start");
							}


			}
			ms330_flag = 0;
		}

 }
////////////////////////////////////////////////////////////////////////////////////////////////
///////////																////////////////////////
//////////			2. MENU DRUGI EKRAN									////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
if(idx.menu == temp)
	{
		if(i)
		{
			wskZmienna = &regTemp;
			lcd_cls();
			lcd_locate(0,0);
			lcd_str("ustaw temp.");
			i = 0;
		}
		if(ms330_flag ==1)
		{
			if(idx.podMenu == 1)
			 {
				lcd_locate(1,3);
				lcd_str("<");

			 }
			else
			 {
				lcd_locate(1,3);
				lcd_str(" ");
			 }
			lcd_locate(1,0);
			lcd_int(regTemp);
			ms330_flag = 0;

		}
	}
////////////////////////////////////////////////////////////////////////////////////////////////
///////////																////////////////////////
//////////			 3. MENU TRZECI EKRAN								////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
if(idx.menu == dodatkowe)
	{
		if(i){

			wskZmienna = &przemieszaj;
			lcd_cls();
			lcd_locate(0,0);
			lcd_str("Mieszanie");
			i = 0;
		}

		if(ms330_flag ==1)
				{
					if(idx.podMenu == 1)
					 {
						lcd_locate(1,15);
						lcd_str("<");

					 }
					else
					 {
						lcd_locate(1,15);
						lcd_str(" ");
					 }
					lcd_locate(1,0);
					lcd_str("mieszaj co:");
					lcd_locate(1,11);
					lcd_int(przemieszaj);
					lcd_locate(1,14);
					lcd_str("s");
					ms330_flag = 0;

				}

	}
}	// klamra petli while
}	// klamra main

void display_temp(uint8_t x){
	lcd_locate(0,x);				// ustawienie kursora w drugim wierszu na pozycji x
	if(subzero)lcd_str("-");		// jesli temp. jest ujemna wyswietl znak
	else lcd_str(" ");
	lcd_int(cel+1);					// wyswiet czesc calkowita temperatury
	lcd_str(".");					// wyswietl kropke
	lcd_int(cel_fract_bits);		// wyswietl czesc dziesiatna stopnia
	lcd_str("C");

}
//// przerwania co 10 milisekund
ISR(TIMER2_COMP_vect){

	static uint8_t cnt = 0;				// statyczna zmienna do odlicznaia dziesietnych ms
	if(cnt%33 == 0)ms330_flag = 1;		// ustawienie flagi 330ms
	if(++cnt>100){

		s1_flag = 1;					// ustawieni flagi sekundy
		sekundy++;						// dodanie sekundy do licznika
		if(sekundy>59)sekundy = 0;		// zerowanie licznika sekund po minucie
		cnt = 0;						// zerowanie licznika ms
	}
	if(odliczanie == 1)
	{										// wlaczenie odliczania
		if(++ms10 > pauza )
		{								// jeli licznik przekroczy dlugosc pauzy wyloczenie i zerowanie licznika
			odliczanie = 0;
			ms10 = 0;
		}
	}



}

///////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////			funkcja wlaczajaca grzalek 			///////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
void heating (void){

	if(cel<regTemp)grzalka_ON;
	else grzalka_OFF;

}

void mieszanie(void){
	if((sekundy%(przemieszaj+2)) == 0){				// 2sekundy dondane na ruch serwa
		OCR1A = 40;
	}
	if((sekundy%(przemieszaj+2)) == 1){
		OCR1A = 15;
	}
}

