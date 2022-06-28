/*
 * MODBAS_.c
 *
 * Created: 19.08.2017 14:27:22
 *  Author: pisanec
 */ 
#include <avr/interrupt.h>   //���������� ���������� 
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
    //������ � ������
//*****************************//
//���� ����� ���������� ������������ ����, �� ������ ������:
//x |= (1 << n);����� �������� ������� � ��� n:
//x &= ~(1 << n);���� ����� ������������� ��������� ����:
//x ^= (1 << n);���� ����� ��������� ��������� ���:
//unsigned char x = (1 << 2) | (1 << 3) | (1 << 7);
//if (x & (1 << 2)) {  /* �� ������ ��� ������� ������� */ }
//if (x & (1 << 3)) {  /* � ������ ��� ������� ������� */ }
//if (x & (1 << 7)) {  /* � ������� ��� ������� ������� */ }

//************************//
   // ����������� ���������� ������� ������ �� �������
//************************// 
ISR  (TIMER0_COMP_vect)     //���������� ������� T0
 { 
		 Bit_action_ModbasRtu &=~ (1<<0);	  // ��������� �� �������� ������� ����������                  
		 TCCR0=0b00001000;  // ������ ����������
	     if ( Bit_action_ModbasRtu & (1<<1) ) // ����������� ������ ��� ���
	      {
		    Bit_action_ModbasRtu &=~ (1<<1);	
			PORTB &=~(1<<0);  
			PORTB &=~(1<<1);	
			Bit_action_ModbasRtu |= (1<<2);   // ������ ������� ����� ������������
	      }  		  
 }
//************************//
   //��������� ������ 
//************************//   
ISR(USART_RXC_vect)         // ���������� ������ ����� ������
 {
     unsigned char TempModbas = UDR;// �������� �������� ������	
     unsigned char D = Bit_action_ModbasRtu;
	TCNT0 = 0;
	if (!(D & (1<<2)))   //������ �� ������� � �� ��������������
	 {
		if (!(D & (1<<0))) // 
		 {
			D |= (1<<0);         // ������ ���� ������
			if (TempModbas == address_Device  )     //��������� � ������ ���������� �� �������- ��,���.
			 {
				D |= (1<<1);	    // ��������� ��������� ����� ������
				PORTB |=(1<<0);                     // ��������� ��� ����� ������ �����������				
			 }
			quantity_Data_ModbasRtu = 0;                       
			TCCR0=0b00001100; // ��������� ������ ��� ����������� ���������� ������ ������
		 }
		if ( D & (1<<1))
		 {
		   Danie_Rx_ModbasRtu[quantity_Data_ModbasRtu] = TempModbas; // ��������� �������� ������
		   quantity_Data_ModbasRtu ++; //���������� �������� ������
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


ISR(USART_UDRE_vect)        //������� ������ �� �������� ����
 {	
   if (quantity_Data_ModbasRtu >= Temp_ModbasRtu )                                                                                                                                                                                                                                       
    {	
	  UDR = Danie_Rx_ModbasRtu[Temp_ModbasRtu++];	  	
    }
   else
    {
	  UCSRB &=~(1 << UDRIE);  //���������� �� ����������� �������� ������ �� �������� ��������� 
	  UCSRB |= (1 << TXCIE);	
	}	 	 	 	   	 	  	
 } 
 //************************//
 // ��������� ��������� ��������
 //************************//
 void Loading_settings_modbasRtu()
 {
	 DDRD |=(1<<1);	  //������ USART (TXD)
	 PORTD |= (1<<0);
	 DDRD |=(1<<2);
	 
	 UBRRH = 0;
	 
	 UBRRL = 103;           // ((8)-115200  (16)-57600  (25)-38400 ���, (51)-19200 ���, (103)-9600 ���.
	 
	 OCR0= 70;             //10-115200 15-57600   20-38400 ���, 40-19200 ���, 70 - 9600 ��� ;

	 
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
// CRC16 Modbus RTU ���������� ����������� ����� 
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
    // ���������� ���� ����
 //******************************//
 int ModbasRtu_Register_address(unsigned char Li)
 {
	 register char Hi= Li - 1;
	 return  Danie_Rx_ModbasRtu[Hi] * 256+ Danie_Rx_ModbasRtu[Li]; // ��������� ������ �������� ����� � �������
 }
//*****************************//
   //�������� ����������� ����� � ��������� ������� ������
//*****************************//
char Data_integrity()
 { 
   register unsigned int Temp22;
   register unsigned char Temp33; 
   quantity_Data_ModbasRtu = quantity_Data_ModbasRtu - 2;        // ������� ����������� ����� �� ��������
   Temp22 = crc_chk(Danie_Rx_ModbasRtu,quantity_Data_ModbasRtu); // ��������� ����������� �����
   Temp33 = Temp22;                                              // �������� ������� ���� � ������������ �����
   if ( Danie_Rx_ModbasRtu[quantity_Data_ModbasRtu] == Temp33  ) // ���������� � ������� ������� ���� ����������� ����
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
   //������ � ����������� ������� � �������� 
 //***********************************//
 //***********************************//
char _Bin_input_Output( register unsigned char NUMBER, register unsigned char state,volatile unsigned char *Masiv, volatile unsigned char Sd ) 
  {
	volatile unsigned char Temp = 0,Temp1 = 0;	
	while (NUMBER >= 8) 
	 {
	   NUMBER = NUMBER - 8;	// 
	   Temp ++; // ���������� � ����� �������� ����� �������� ���� ������� ���
	 }
	Temp1 = Masiv [ Temp ];
	if (Sd == 10 ) //����������� ���� ����� �������� ��� 
	 {
	   if ( state == On) 
	    Temp1 |=(1<<NUMBER);
	   else
	    Temp1 &=~(1<<NUMBER);
	   Masiv [ Temp ] =  Temp1; 	
	 } 
	else  // ����������� ���� ����� ��������� ��������� ����
	 {
		if ( Temp1 & (1<<NUMBER) )
		 NUMBER = 1;
		else
		 NUMBER = 0;		 		 
	 }	
	 return NUMBER; // ���������� ��������� ����������� ����
  }   

//************************************	
 //....................................
  //��������� ����������� ������ ������� 0x05
 //.................................... 
 //************************************ 	
 void Changing_Discrete_Output(void)
   {
	  register unsigned int address;
	  address = ModbasRtu_Register_address(3); //����� �������� � ��������� ���������� ������	  
	  if (  address >  NUMBER_Binary_Output ) // �������� ��� ������ �� ��������� ����������
	   {
		  Error_modbasRtu (0x02); // �� ���������� ������  
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
    //������ ����������� ������ � ����� ������� 0x01,0x02
  //.................................	
  //********************************
 void Reading_Discrete_Output(unsigned char *Massiv, register unsigned char Number_)
  {
	volatile unsigned int address,Number_bits;
	register unsigned char Temp = 0,Danie,Temp2 = 0,address2 = 0,Temp3 = 2;
	address = ModbasRtu_Register_address(3); // ����� �������� � �������� ���������� ������	  
	if (  address > Number_  )    // �������� ��� ������ �� ��������� ����������
	 {
		Error_modbasRtu (0x02); // �� ���������� ������ 
	 }
	 else
	 {
		Number_bits =  ModbasRtu_Register_address(5); //���������� ��� ������� ����� ��������
		while (address >= 8) // ������ ����� ������ ������� � ������� ������ ��������� ������
		 {
		   address = address - 8;	// address - �� ���������� �������������� �������� ��� � �������� ����� �������� ����������  
		   Temp ++;  //����� ����� � ������� � �������� ���������� ���������� ���������
		 }
		Danie = Massiv [ Temp ];	// ������� ������ 	 
		//----------------
		  //������� ������� � ��������� ������ ��� ��������
		//----------------  
		while ( Number_bits > 0) // �������� ��� ��� ���� ������� ���������
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
				Danie = Massiv [ Temp ];	// ������� ������ 
			 }
		 }
		if ( address2 > 0 )
		 {
		   Temp3 ++;
		   Danie_Rx_ModbasRtu[Temp3] = Temp2;	
		 }
		//----------------
		//----------------	
		Danie_Rx_ModbasRtu[2] = Temp3 - 2; // ���������� ���������� ���� (��� ����� ������� � ���� �������)
		Temp3 ++;
		check_sum ( Temp3); //���������� ����������� ����� ��� �������� ������		 		 
	 }	  	    
  }	
 //********************************
  //.................................
    //������ ����������� ����� � ������? ������� 0x04,0x03
  //.................................	
  //******************************** 
 void Read_analog_input(unsigned char *Massiv, register unsigned char Number_, unsigned char Vt)
 {
	volatile unsigned int address,Number_bits,Danie;
	volatile unsigned  char Adress = 4;
	address = ModbasRtu_Register_address(3); // ������ �������� � �������� ���������� ������	  
	if (  address >  Number_ )     // �������� ��� ������ �� ��������� ����������
	 {
		Error_modbasRtu (0x02); // �� ���������� ������ 
	 }
	 else
	 {		   
		Number_bits =  ModbasRtu_Register_address(5); //���������� INT ���� ������� ����� �������� (������� � ������� )
		Danie_Rx_ModbasRtu[2] = Number_bits * 2; // ���������� ���� ���������� ��� ����� ��������� 
		Adress = 3;
		while (Number_bits > 0 ) 
		 {
			if ( Vt == 1 ) // ����������� ��� ���������, ���� ��� �����
			{
			  Danie = Danie_ModbasRtu_analog_input[ address ];	
			}
			else
		    {
			  Danie = Danie_ModbasRtu_analog_Output[ address ];		
			}			 
			address++;
			Massiv = &Danie;  
			Danie_Rx_ModbasRtu[Adress ++] = Massiv[1];	// ������� ������� ����
			Danie_Rx_ModbasRtu[Adress++] = Massiv [0];  // ������� ������� ����	
			Number_bits = Number_bits - 1 ;	 
		 }
		check_sum ( Adress); //���������� ����������� ����� ��� �������� ������				
	 }	
 }
 //************************************//
 //..............................
      //  ������ ����������� ������? ������� 0x06
 //..............................
 //************************************//
 void analog_output_recording(void)
  {
	register int address;  
	address = ModbasRtu_Register_address(3);   // ������ �������� � �������� ���������� ������ 
	if (  address >  NUMBER_ANALOG_Output )    // �������� ��� ������ �� ��������� ����������
	 {
		Error_modbasRtu (0x02); // �� ���������� ������ 
	 }
	 else
	 {
	   Danie_ModbasRtu_analog_Output [address] = ModbasRtu_Register_address(5); // ������ ������� ����� �������� 
	 } 	  
  }
  //************************************//
  //-----------------------------------
                   //������
  //-----------------------------------
  //************************************//
 void Error_modbasRtu (volatile unsigned char Temp_Error)
  {
	Danie_Rx_ModbasRtu[1] |= (1<<7); 
	Danie_Rx_ModbasRtu[2] = Temp_Error; // ��� ������
	check_sum (3); //���������� ����������� ����� ��� �������� ������						
  }
  //************************************//
     //��������� ����� ����������� �����
  //************************************//
 void check_sum ( register unsigned char Adress)
  {
	register unsigned int RC;  
	RC = crc_chk(Danie_Rx_ModbasRtu,Adress); // ��������� ����������� �����
	Danie_Rx_ModbasRtu[Adress] = RC ; // ������� ���� ����������� ������
	Adress++;
	Danie_Rx_ModbasRtu[Adress] = RC >> 8; // ������� ���� ����������� �����
	quantity_Data_ModbasRtu = Adress;  
  }
  //***********************************//
  //***********************************//
  //��������� �������� � ����� �� �������� ������� ��� ��������� ����� �� �������
  //***********************************//
  //***********************************//
  void modbasRtu_Answer()
  {
	  Temp_ModbasRtu = 0;
	  switch (Danie_Rx_ModbasRtu[1])
	  {
		  case 1:
		         Reading_Discrete_Output(Danie_ModbasRtu_Binary_Output,NUMBER_Binary_Output); // Modbus RTU ������ ����������� ������? ������� 0x01
		         break;
		  case 2:
		         Reading_Discrete_Output(Danie_ModbasRtu_Binary_input,NUMBER_Binary_inputs);   // Modbus RTU ������ ����������� �����? ������� 0x02
		         break;
		  case 3:
		         Read_analog_input( Danie_ModbasRtu_analog_Output,NUMBER_ANALOG_Output,0);     // Modbus RTU �� ������ ����������� ������ ������� 0x03
		  break;
		  case 4:
		         Read_analog_input(Danie_ModbasRtu_analog_input,NUMBER_ANALOG_INPUT,1);        // Modbus RTU �� ������ ����������� �����? ������� 0x04
		         break;
		  case 5:
		         Changing_Discrete_Output(); // Modbus RTU �� ������ ����������� ������? ������� 0x05
		         break;
		  case 6:  
		         analog_output_recording();// Modbus RTU �� ������ ����������� ������? ������� 0x06				 
		         break;
		  case 15: // Modbus RTU �� ������ ���������� ���������� �������? ������� 0x0F
		         asm("nop");
		//  break;
		  case 16:
		         asm("nop");
		       // Modbus RTU �� ������ ���������� ���������� �������? ������� 0x10
		 // break;
		  default: // ������� �� �������������
		        Error_modbasRtu (0x01); //�������� ��� ������� �� ����� ���� ���������. 
		  break;
	  }	
	 PORTD |= (1<<2);    
	 UCSRB |=(1 << UDRIE); // ����� ���������� ���� ��� ������	 
	 Bit_action_ModbasRtu &=~ (1<<2); // ������ ���������� ����� ��������� ���������
  }
  //*************************//
     //������������ ��� ��������� ��������
  //*************************//
 char read_digital_inputs( volatile unsigned char Temp1 ) // ��������� ��� ������
  {
	  return  _Bin_input_Output (Temp1,On,Danie_ModbasRtu_Binary_input,discrete_output_reading);   // ������� ���������  ������ �� ��������� �������
  }
void change_digital_inputs( volatile unsigned char Temp1,volatile unsigned char Temp2 ) // �������� ��� ������
  {
	  _Bin_input_Output (Temp1,Temp2,Danie_ModbasRtu_Binary_input,Change_output);
  }
char read_digital_Output( volatile unsigned char Temp1 ) // ��������� ��� �������
  {
	  return _Bin_input_Output (Temp1,On,Danie_ModbasRtu_Binary_Output,discrete_output_reading);   // ������� ���������  ������ �� ��������� �������
  }
void change_digital_Output( volatile unsigned char Temp1,volatile unsigned char Temp2 ) // �������� ��� �������
  {
	  _Bin_input_Output (Temp1,Temp2,Danie_ModbasRtu_Binary_Output,Change_output);
  } 
void change_analogue_Output (volatile unsigned char nomer, int Danie) // �������� �������� ���������� ������� 
  {
	  Danie_ModbasRtu_analog_Output [ nomer ] = Danie; 	  	  
  }
void change_analogue_input (volatile unsigned char nomer, int Danie)  // �������� �������� ���������� ������
  {
	  Danie_ModbasRtu_analog_input [ nomer ] = Danie; 	  	  
  }   
 int read_analogue_Output (volatile unsigned char nomer) // ������� �������� ���������� �������
  {
	 return  Danie_ModbasRtu_analog_Output [ nomer ];
  }
 int read_analogue_input (volatile unsigned char nomer)  // ������� �������� ���������� ������
  {
	 return  Danie_ModbasRtu_analog_input [ nomer ];
  }
  //*****************************//
 void modbasRtu_Slave( void )
   {
    if (Bit_action_ModbasRtu & (1<<2))      // ������ �������
    {
	    if ( Data_integrity() == 0 )         // �������� �������� ������ �� ����������� ����� 1- ����, 0-������
	    {
		    PORTB |=(1<<1);	                  // ����� ������ ���������
		    Error_modbasRtu (0x04);	          // ������������������� ������ ����� �����, ���� ������� ���������� �������� ��������� ������������� ��������.
		    Bit_action_ModbasRtu &=~ (1<<2);  // ������ ���������� ����� ��������� ���������
	    }
	    else
	    {
		    modbasRtu_Answer(  );              // ��������� ������� � ��������� �����, �� ���������� ���� ������
	    }
    }  
  }
   // ������ ������ � ����������	
	  //******************************//
	  //******************************// 
	     /*
		   
           change_digital_inputs(23,On);  // �������� ���������� ���� ���� 23 �� 1 (On-1,Off-0)  
	       BV = read_digital_inputs(23);  // ������� ��������� ����������� ����� 23 (���������� char)
	      
	       change_digital_Output(20,On);  // �������� ���������� ����� ���� 20 �� 1 (On-1,Off-0)  
	       BV = read_digital_Output(20);  // ������� ��������� ����������� ������ 20 (���������� char)
		   
		   change_analogue_Output(1,567); // �������� �������� 567, ������� ����������� ������, (������ ������� 1) 
		   change_analogue_input (2,300); // �������� �������� 300, �������� ����������� �����,( ������ ������� 2) 
		   
		   BV = read_analogue_Output (1); // ������� �������� ������� ����������� ������ (���������� int)
		   BV = read_analogue_input (2);  // ������� �������� ������� ����������� ����� (���������� int)
		   
	    */   