/**
 * Moduł sterujący do Solara
 * A0 wyjście z potencjometru
 * 9  wyjście do sterownika silnika
 * 2  zrywka (podpięta do masy)
 * 3  przycisk powrotu do normalnej pracy (wciśnięcie zwiera do masy)
 * 
 * Piotrek Kuciński, 19.03.2016
 */
#include <Servo.h>

Servo esc;
int suwak_pin = 0;  //pin analogowy dla suwaka
int esc_pin = 9;  //pin PWM dla sterownika silnika
int zrywka_pin = 2; //cyfrowy pin z przerwaniem dla zrywki
int przycisk_pin = 3; //cyfrowy pin z przerwaniem dla powrotu zrywki
int val;
volatile bool zrywka_niezerwana = true;

void setup() {
  esc.attach(esc_pin);  //sygnał sterownika podpięty do pinu 9
  pinMode(zrywka_pin, INPUT_PULLUP);
  pinMode(przycisk_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(zrywka_pin), zerwanie_zrywki, RISING);  //zerwanie zrywki wywoła zbocze narastające
  attachInterrupt(digitalPinToInterrupt(przycisk_pin), test_zrywki, FALLING); //wciśnięcie przycisku wywoła zbocze opadające
}

void loop() {
  if(zrywka_niezerwana)
  {
    val = analogRead(suwak_pin);
    map(val, 0, 1023, 0, 180);
  }
  else
    val = 0;
  esc.write(val);
}

void zerwanie_zrywki()
{
  zrywka_niezerwana = false;
}

void test_zrywki()
{
  int val = digitalRead(zrywka_pin);
  if(val == LOW)
    zrywka_niezerwana = true;
}

