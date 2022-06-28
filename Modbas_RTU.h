/*
 * MODBAS_.c
 *
 * Created: 19.08.2017 14:27:22
 *  Author: pisanec
 */ 
#include <avr/interrupt.h>   //подгружаем прерывание 
#define On 1 
#define Off 0
#define Change_output 10
#define discrete_output_reading 11
volatile unsigned char Danie_Rx_ModbasRtu[30] = {},quantity_Data_ModbasRtu;
volatile unsigned char Bit_action_ModbasRtu,DD;
volatile unsigned int  Danie_ModbasRtu_analog_input  [ NUMBER_ANALOG_INPUT ];
volatile unsigned int  Danie_ModbasRtu_analog_Output [ NUMBER_ANALOG_Output ];
volatile unsigned char Danie_ModbasRtu_Binary_input  [ (NUMBER_Binary_inputs / 8)+ 1 ];
volatile unsigned char Danie_ModbasRtu_Binary_Output [ (NUMBER_Binary_Output/ 8)+ 1 ];
volatile unsigned char Temp_ModbasRtu;

//****************************//
    //Работа с битами
//*****************************//
//Если нужно записывать определенные биты, не стирая другие:
//x |= (1 << n);Чтобы записать единицу в бит n:
//x &= ~(1 << n);Если нужно инвертировать состояние бита:
//x ^= (1 << n);Если нужно прочитать отдельный бит:
//unsigned char x = (1 << 2) | (1 << 3) | (1 << 7);
//if (x & (1 << 2)) {  /* во второй бит вписана единица */ }
//if (x & (1 << 3)) {  /* в третий бит вписана единица */ }
//if (x & (1 << 7)) {  /* в седьмой бит вписана единица */ }

//************************//
   // определение завершение посылки данных от мастера
//************************// 
ISR  (TIMER0_COMP_vect)     //прерывание таймера T0
 { 
		 Bit_action_ModbasRtu &=~ (1<<0);	  // установка на проверку адресса устройства                  
		 TCCR0=0b00001000;  // таймер остановлен
	     if ( Bit_action_ModbasRtu & (1<<1) ) // принимались данные или нет
	      {
		    Bit_action_ModbasRtu &=~ (1<<1);	
			PORTB &=~(1<<0);  
			PORTB &=~(1<<1);	
			Bit_action_ModbasRtu |= (1<<2);   // данные приняты можно обрабатывать
	      }  		  
 }
//************************//
   //принимаем данные 
//************************//   
ISR(USART_RXC_vect)         // завершение приема байта данных
 {
     unsigned char TempModbas = UDR;// забираем принятые данные	
     unsigned char D = Bit_action_ModbasRtu;
	TCNT0 = 0;
	if (!(D & (1<<2)))   //данные не приняты и не обрабатываются
	 {
		if (!(D & (1<<0))) // 
		 {
			D |= (1<<0);         // первый байт принят
			if (TempModbas == address_Device  )     //обращение к даному устройству по адрессу- да,нет.
			 {
				D |= (1<<1);	    // разрешаем принимать пакет данные
				PORTB |=(1<<0);                     // светодиод что адрес принят устройством				
			 }
			quantity_Data_ModbasRtu = 0;                       
			TCCR0=0b00001100; // запускаем таймер для определения завершения пакета данных
		 }
		if ( D & (1<<1))
		 {
		   Danie_Rx_ModbasRtu[quantity_Data_ModbasRtu] = TempModbas; // сохраняем принятые данные
		   quantity_Data_ModbasRtu ++; //количество принятых данных
		 } 		 		 		 
	 }
	Bit_action_ModbasRtu = D; 
 }
//*************************//
//*************************//
ISR(USART_TXC_vect)
 {
   PORTD &=~ (1<<2);
   UCSRB &=~ (1 << TXCIE);	 
 }


ISR(USART_UDRE_vect)        //регистр данных на передачю пуст
 {	
   if (quantity_Data_ModbasRtu >= Temp_ModbasRtu )                                                                                                                                                                                                                                       
    {	
	  UDR = Danie_Rx_ModbasRtu[Temp_ModbasRtu++];	  	
    }
   else
    {
	  UCSRB &=~(1 << UDRIE);  //прерывания по опусташение регистра данных на передачю запрещаем 
	  UCSRB |= (1 << TXCIE);	
	}	 	 	 	   	 	  	
 } 
 //************************//
 // начальная установка значений
 //************************//
 void Loading_settings_modbasRtu()
 {
	 DDRD |=(1<<1);	  //вывода USART (TXD)
	 PORTD |= (1<<0);
	 DDRD |=(1<<2);
	 
	 UBRRH = 0;
	 
	 UBRRL = 103;           // ((8)-115200  (16)-57600  (25)-38400 бит, (51)-19200 бит, (103)-9600 бит.
	 
	 OCR0= 70;             //10-115200 15-57600   20-38400 бит, 40-19200 бит, 70 - 9600 бит ;

	 
	 UCSRB |= (1 << RXCIE)|(1<<RXEN)|(1<<TXEN);
	 UCSRC |= (1 << URSEL)|(1 << UCSZ1)|(1 << UCSZ0);	 
	//---------------// 
	 DDRB |=(1<<0);
	 DDRB |=(1<<1);
	//---------------// 	 
	 TCCR0=0b00001000;
	 TIMSK|=(1<<1);
	 asm("sei");
 }
//************************//
// CRC16 Modbus RTU подсчитуем контрольную сумму 
//************************//
int crc_chk ( unsigned char* data, unsigned char length )
  {
	 int j;
	 unsigned int reg_crc = 0xFFFF;
	while (length--)
	{
	  reg_crc ^= *data++;
	  for(j=0;j<8;j++)
	   {
		 if(reg_crc & 0x01)
	      {
		    reg_crc = (reg_crc >> 1) ^ 0xA001;
	      }
		else
	      {
		    reg_crc = reg_crc >> 1;
		  }
	   }
	}
	return reg_crc;
  }
 //******************************//
    // обединение двух байт
 //******************************//
 int ModbasRtu_Register_address(unsigned char Li)
 {
	 register char Hi= Li - 1;
	 return  Danie_Rx_ModbasRtu[Hi] * 256+ Danie_Rx_ModbasRtu[Li]; // считываем адресс старшего байта и младший
 }
//*****************************//
   //проверка контрольной суммы в полученой посылке данных
//*****************************//
char Data_integrity()
 { 
   register unsigned int Temp22;
   register unsigned char Temp33; 
   quantity_Data_ModbasRtu = quantity_Data_ModbasRtu - 2;        // убираем контрольную сумму от адрессов
   Temp22 = crc_chk(Danie_Rx_ModbasRtu,quantity_Data_ModbasRtu); // вычесляем контрольную сумму
   Temp33 = Temp22;                                              // выделяем старшый байт с контролльной суммы
   if ( Danie_Rx_ModbasRtu[quantity_Data_ModbasRtu] == Temp33  ) // сравнимаем с таблицы старший байт контрольной суму
	{
	  quantity_Data_ModbasRtu ++;
	  Temp33 = ( Temp22 >> 8); 
	  if ( Danie_Rx_ModbasRtu[quantity_Data_ModbasRtu] == Temp33  )
	   {	
		 return 1; 	
	   }
	}			
   return 0;		
 }
 //***********************************//
 //***********************************//
   //Работа с дискретными входами и выводами 
 //***********************************//
 //***********************************//
char _Bin_input_Output( register unsigned char NUMBER, register unsigned char state,volatile unsigned char *Masiv, volatile unsigned char Sd ) 
  {
	volatile unsigned char Temp = 0,Temp1 = 0;	
	while (NUMBER >= 8) 
	 {
	   NUMBER = NUMBER - 8;	// 
	   Temp ++; // определяем в каком регистре нужно изменить либо считать бит
	 }
	Temp1 = Masiv [ Temp ];
	if (Sd == 10 ) //выполняется если нужно изменить бит 
	 {
	   if ( state == On) 
	    Temp1 |=(1<<NUMBER);
	   else
	    Temp1 &=~(1<<NUMBER);
	   Masiv [ Temp ] =  Temp1; 	
	 } 
	else  // выполняется если нужно прочитать состояние бита
	 {
		if ( Temp1 & (1<<NUMBER) )
		 NUMBER = 1;
		else
		 NUMBER = 0;		 		 
	 }	
	 return NUMBER; // возвращает состояние прочинаного бита
  }   

//************************************	
 //....................................
  //изменение дискретного вывода Команда 0x05
 //.................................... 
 //************************************ 	
 void Changing_Discrete_Output(void)
   {
	  register unsigned int address;
	  address = ModbasRtu_Register_address(3); //Адрес регистра к которомцу обращается мастер	  
	  if (  address >  NUMBER_Binary_Output ) // проверка что адресс не превышает допустимый
	   {
		  Error_modbasRtu (0x02); // не допустимый адресс  
	   }
	   else
	   {
		 if (Danie_Rx_ModbasRtu[4] == 255) 
		   _Bin_input_Output (address,On,Danie_ModbasRtu_Binary_Output,Change_output);  
	     else
           _Bin_input_Output (address,Off,Danie_ModbasRtu_Binary_Output,Change_output);     
	   }	  	 	  	  	  	
  }
  //********************************
  //.................................
    //чтение дискретного вывода и входа Команда 0x01,0x02
  //.................................	
  //********************************
 void Reading_Discrete_Output(unsigned char *Massiv, register unsigned char Number_)
  {
	volatile unsigned int address,Number_bits;
	register unsigned char Temp = 0,Danie,Temp2 = 0,address2 = 0,Temp3 = 2;
	address = ModbasRtu_Register_address(3); // Адрес регистра к которому обращается мастер	  
	if (  address > Number_  )    // проверка что адресс не превышает допустимый
	 {
		Error_modbasRtu (0x02); // не допустимый адресс 
	 }
	 else
	 {
		Number_bits =  ModbasRtu_Register_address(5); //количество бит которые нужно передать
		while (address >= 8) // узнаем номер ячейки массива с которой начнем считывать данные
		 {
		   address = address - 8;	// address - по завершению преобразования хранится бит с которого нужно начинать считывание  
		   Temp ++;  //номер байта в массиве к которому изначально происходит обращение
		 }
		Danie = Massiv [ Temp ];	// считуем данные 	 
		//----------------
		  //считуем побитно и формируем данные для отправки
		//----------------  
		while ( Number_bits > 0) // проверка что все биты запроса переданны
		  {
			Number_bits --;      
			if ( Danie & (1 << address) )	// 
			 {	  	
			   Temp2 |=(1<<address2);
			 }
			address2 ++;
			address ++; 
			if (address2 == 8 ) 
			 {
			   address2 = 0;
			   Temp3 ++;
			   Danie_Rx_ModbasRtu[Temp3] = Temp2; 
			   Temp2 = 0;
			 }	 		 		 
			if ( address == 8)
			 {
				address = 0;
				Temp++; 
				Danie = Massiv [ Temp ];	// считуем данные 
			 }
		 }
		if ( address2 > 0 )
		 {
		   Temp3 ++;
		   Danie_Rx_ModbasRtu[Temp3] = Temp2;	
		 }
		//----------------
		//----------------	
		Danie_Rx_ModbasRtu[2] = Temp3 - 2; // количество переданных байт (без учета адресса и кода команды)
		Temp3 ++;
		check_sum ( Temp3); //подсчитуем контрольную сумму для передачи данных		 		 
	 }	  	    
  }	
 //********************************
  //.................................
    //чтение аналогового входа и выхода? Команда 0x04,0x03
  //.................................	
  //******************************** 
 void Read_analog_input(unsigned char *Massiv, register unsigned char Number_, unsigned char Vt)
 {
	volatile unsigned int address,Number_bits,Danie;
	volatile unsigned  char Adress = 4;
	address = ModbasRtu_Register_address(3); // Адресс регистра к которому обращается мастер	  
	if (  address >  Number_ )     // проверка что адресс не превышает допустимый
	 {
		Error_modbasRtu (0x02); // не допустимый адресс 
	 }
	 else
	 {		   
		Number_bits =  ModbasRtu_Register_address(5); //количество INT байт которые нужно передать (старший и младший )
		Danie_Rx_ModbasRtu[2] = Number_bits * 2; // количество байт информащии что будут переданны 
		Adress = 3;
		while (Number_bits > 0 ) 
		 {
			if ( Vt == 1 ) // определение что считывать, вход или выход
			{
			  Danie = Danie_ModbasRtu_analog_input[ address ];	
			}
			else
		    {
			  Danie = Danie_ModbasRtu_analog_Output[ address ];		
			}			 
			address++;
			Massiv = &Danie;  
			Danie_Rx_ModbasRtu[Adress ++] = Massiv[1];	// считуем старший байт
			Danie_Rx_ModbasRtu[Adress++] = Massiv [0];  // считуем младший байт	
			Number_bits = Number_bits - 1 ;	 
		 }
		check_sum ( Adress); //подсчитуем контрольную сумму для передачи данных				
	 }	
 }
 //************************************//
 //..............................
      //  запись аналогового выхода? Команда 0x06
 //..............................
 //************************************//
 void analog_output_recording(void)
  {
	register int address;  
	address = ModbasRtu_Register_address(3);   // Адресс регистра к которому обращается мастер 
	if (  address >  NUMBER_ANALOG_Output )    // проверка что адресс не превышает допустимый
	 {
		Error_modbasRtu (0x02); // не допустимый адресс 
	 }
	 else
	 {
	   Danie_ModbasRtu_analog_Output [address] = ModbasRtu_Register_address(5); // данные которые нужно записать 
	 } 	  
  }
  //************************************//
  //-----------------------------------
                   //АВАРИЯ
  //-----------------------------------
  //************************************//
 void Error_modbasRtu (volatile unsigned char Temp_Error)
  {
	Danie_Rx_ModbasRtu[1] |= (1<<7); 
	Danie_Rx_ModbasRtu[2] = Temp_Error; // код ошибки
	check_sum (3); //подсчитуем контрольную сумму для передачи данных						
  }
  //************************************//
     //Формеруем ответ контрольной суммы
  //************************************//
 void check_sum ( register unsigned char Adress)
  {
	register unsigned int RC;  
	RC = crc_chk(Danie_Rx_ModbasRtu,Adress); // вычесляем контроллную сумму
	Danie_Rx_ModbasRtu[Adress] = RC ; // младший байт контрольной ссуммы
	Adress++;
	Danie_Rx_ModbasRtu[Adress] = RC >> 8; // старший байт контрольной суммы
	quantity_Data_ModbasRtu = Adress;  
  }
  //***********************************//
  //***********************************//
  //ФОРМЕРУЕМ ДЕЙСТВИЕ И ОТВЕТ НА ПРИНЯТЫЕ КОМАНДЫ ИЛИ ФОРМИРУЕМ ОТВЕТ ОБ ОШИбКАХ
  //***********************************//
  //***********************************//
  void modbasRtu_Answer()
  {
	  Temp_ModbasRtu = 0;
	  switch (Danie_Rx_ModbasRtu[1])
	  {
		  case 1:
		         Reading_Discrete_Output(Danie_ModbasRtu_Binary_Output,NUMBER_Binary_Output); // Modbus RTU чтение дискретного выхода? Команда 0x01
		         break;
		  case 2:
		         Reading_Discrete_Output(Danie_ModbasRtu_Binary_input,NUMBER_Binary_inputs);   // Modbus RTU чтение дискретного входа? Команда 0x02
		         break;
		  case 3:
		         Read_analog_input( Danie_ModbasRtu_analog_Output,NUMBER_ANALOG_Output,0);     // Modbus RTU на чтение аналогового выхода Команда 0x03
		  break;
		  case 4:
		         Read_analog_input(Danie_ModbasRtu_analog_input,NUMBER_ANALOG_INPUT,1);        // Modbus RTU на чтение аналогового входа? Команда 0x04
		         break;
		  case 5:
		         Changing_Discrete_Output(); // Modbus RTU на запись дискретного вывода? Команда 0x05
		         break;
		  case 6:  
		         analog_output_recording();// Modbus RTU на запись аналогового выхода? Команда 0x06				 
		         break;
		  case 15: // Modbus RTU на запись нескольких дискретных выводов? Команда 0x0F
		         asm("nop");
		//  break;
		  case 16:
		         asm("nop");
		       // Modbus RTU на запись нескольких аналоговых выводов? Команда 0x10
		 // break;
		  default: // команды не подерживаются
		        Error_modbasRtu (0x01); //Принятый код функции не может быть обработан. 
		  break;
	  }	
	 PORTD |= (1<<2);    
	 UCSRB |=(1 << UDRIE); // ответ отправляем либо код ошибки	 
	 Bit_action_ModbasRtu &=~ (1<<2); // данные обработаны можем принимать следующие
  }
  //*************************//
     //подпрограммы для установки значений
  //*************************//
 char read_digital_inputs( volatile unsigned char Temp1 ) // прочитать бит входов
  {
	  return  _Bin_input_Output (Temp1,On,Danie_ModbasRtu_Binary_input,discrete_output_reading);   // считать состояние  выхода из буферного массива
  }
void change_digital_inputs( volatile unsigned char Temp1,volatile unsigned char Temp2 ) // изменить бит входов
  {
	  _Bin_input_Output (Temp1,Temp2,Danie_ModbasRtu_Binary_input,Change_output);
  }
char read_digital_Output( volatile unsigned char Temp1 ) // прочитать бит выходов
  {
	  return _Bin_input_Output (Temp1,On,Danie_ModbasRtu_Binary_Output,discrete_output_reading);   // считать состояние  выхода из буферного массива
  }
void change_digital_Output( volatile unsigned char Temp1,volatile unsigned char Temp2 ) // изменить бит выходов
  {
	  _Bin_input_Output (Temp1,Temp2,Danie_ModbasRtu_Binary_Output,Change_output);
  } 
void change_analogue_Output (volatile unsigned char nomer, int Danie) // записать значение аналоговые выходов 
  {
	  Danie_ModbasRtu_analog_Output [ nomer ] = Danie; 	  	  
  }
void change_analogue_input (volatile unsigned char nomer, int Danie)  // записать значение аналоговых входов
  {
	  Danie_ModbasRtu_analog_input [ nomer ] = Danie; 	  	  
  }   
 int read_analogue_Output (volatile unsigned char nomer) // считать значение аналоговые выходов
  {
	 return  Danie_ModbasRtu_analog_Output [ nomer ];
  }
 int read_analogue_input (volatile unsigned char nomer)  // считать значение аналоговых входов
  {
	 return  Danie_ModbasRtu_analog_input [ nomer ];
  }
  //*****************************//
 void modbasRtu_Slave( void )
   {
    if (Bit_action_ModbasRtu & (1<<2))      // данные приняты
    {
	    if ( Data_integrity() == 0 )         // праверка принятых данных по контрольной сумме 1- норм, 0-ошибка
	    {
		    PORTB |=(1<<1);	                  // пакет данных поврежден
		    Error_modbasRtu (0x04);	          // Невосстанавливаемая ошибка имела место, пока ведомое устройство пыталось выполнить затребованное действие.
		    Bit_action_ModbasRtu &=~ (1<<2);  // данные обработаны можем принимать следующие
	    }
	    else
	    {
		    modbasRtu_Answer(  );              // выполняем команду и формеруем ответ, на выполнение либо ашибки
	    }
    }  
  }
   // Пример записи и считывания	
	  //******************************//
	  //******************************// 
	     /*
		   
           change_digital_inputs(23,On);  // изменить дискретный вход пина 23 на 1 (On-1,Off-0)  
	       BV = read_digital_inputs(23);  // считать состояние дискретного входа 23 (возвращает char)
	      
	       change_digital_Output(20,On);  // изменить дискретный выход пина 20 на 1 (On-1,Off-0)  
	       BV = read_digital_Output(20);  // считать состояние дискретного выхода 20 (возвращает char)
		   
		   change_analogue_Output(1,567); // записать значение 567, первого аналогового выхода, (ячейка массива 1) 
		   change_analogue_input (2,300); // записать значение 300, врторого аналогового входа,( ячейка массива 2) 
		   
		   BV = read_analogue_Output (1); // считать значение первого аналогового выхода (возвращает int)
		   BV = read_analogue_input (2);  // считать значение первого аналогового входа (возвращает int)
		   
	    */   