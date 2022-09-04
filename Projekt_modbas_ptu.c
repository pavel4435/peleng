/*
 * Projekt_modbas_ptu.c
 * Created: 21.08.2017 7:46:29
 *  Author: pisanec
 */ 
#include <avr/io.h> /**********************/
#define address_Device  100      // адресс сети устройства (1 - 247)
#define NUMBER_ANALOG_INPUT 11   // количество данных для аналоговых входов (по два байта на каннал, пример: 0 старший 1 младший, данные для первого каннала)
#define NUMBER_ANALOG_Output 10  // количество данных для аналоговых выходов (по два байта на каннал, пример: 0 старший 1 младший, данные для первого каннала
#define NUMBER_Binary_inputs 32  // количество дискретных входов (выделяется бит для одного порта c 1 - 32)
#define NUMBER_Binary_Output 50  // количество дискретных выходов (выделяется бит для одного порта c 1 - 50)
#include "Modbas_RTU.h"
volatile unsigned int BV,ACP;
//******************//
//******************//
ISR  (ADC_vect)               // прерывание ACP           
  {             
    ACP  = ADCW;              // сохраняем данные ацп   	
	ADCSRA |= (1<<ADSC); 
  }
int main(void)
 {	
	Loading_settings_modbasRtu();  	
	DDRB |=(1<<2);	
	ADMUX |=(1<<REFS0);                                                                                                                                                                          
    ADCSRA|=(1<<ADEN)|(1<<ADIE)|(1<<ADPS1)|(1<<ADSC); 
	ADCSRA |= (1<<ADSC);            // запуск разового преобразования ацп							   	 
//*****************//	 
   while(1)
    {	 
	  //******************************//
	  //******************************//
	  change_analogue_Output (3,ACP);//ACP
	 // change_analogue_input (3,100);
	 // change_digital_Output(23,On);	
	  BV = read_digital_Output(23);  // считать состояние дискретного выхода 23
	  if ( BV == 1)
	   {
	     PORTB |=(1<<2);
		 //change_digital_Output(20,On);
	   }
	   else
	   {  
	     PORTB &=~(1<<2);
		// change_digital_Output(20,Off);
	   }      	  
	 //************//	   
	    modbasRtu_Slave(); //прием данных по модбасу
	 //************// 	   
	}
 }
 {
 }
 
 
 
 
