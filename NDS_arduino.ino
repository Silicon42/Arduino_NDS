#include "avr/io.h"
// The following buttons are the external (non NDS) buttons used to control the arduino
#define ExtBtn1 2
#define ExtBtn2 3
#define ExtBtn3 4
#define ExtBtn4 5
// The following buttons are mapped to PORTB bit 0..5
#define BtnA 0x0001
#define BtnB 0x0002
#define BtnX 0x0004
#define BtnY 0x0008
#define BtnUp 0x0010
#define BtnDown 0x0020
// the following buttons are mapped to PORTC bit 0..5
#define BtnLeft 0x0040
#define BtnRight 0x0080
#define BtnStart 0x0100
#define BtnSelect 0x0200
#define Unused1 0x0400    	//could be used for BtnL
#define Unused2 0x0800    	//could be used for BtnR
// The following are values associated with commands
#define CmdMask 0xf000
#define DataMask 0x0fff
#define CmdEOF 0x0000		//reserved for unused entries in array and the end of the array
#define CmdInput 0x1000
#define CmdOutput 0x2000
#define CmdDelay 0x3000
#define CmdLongDelay 0x4000
#define CmdBtnRepeat 0x5000	//uses the data of the entry immediately after it for the buttons then skips that command

const unsigned short int cmdList[22] = {CmdBtnRepeat|0x0006, CmdOutput|BtnDown, CmdBtnRepeat|0x0002, CmdEOF|0x0001};  //the array must be terminated with CmdEOF

void setup()
{
    // put your setup code here, to run once:
    // initialize the digital pin as an output.
    // Pin 13 has an LED connected on most Arduino boards:
	PORTB = 0xff;		//NDS buttons
	DDRB = 0;		//0 = input, 1 = output(pressed)
	PORTC = 0xff;
	DDRC = 0;
	
	digitalWrite(2, HIGH);		//external buttons
    digitalWrite(3, HIGH);
    digitalWrite(4, HIGH);
    digitalWrite(5, HIGH);
	
    Serial.begin(57600);         // lower the BAUD rate if this runs on 8mhz without an x-tal as per fuse settings
	
}

void loop()
{
  char str[55];
    // put your main code here, to run repeatedly:
	static unsigned int i = 0;

    switch ( cmdList[i] & CmdMask )
    {

    case CmdInput:
		Serial.println("waiting for input");

		while( !( (~PINC & 0x3f) == ( (cmdList[i]>>6) & 0x3f)  && (~PINB & 0x3f) == (cmdList[i] & 0x3f) ) )		//input logic is inverted thus ~PINB/~PINC is necessary
		{
			delay(255);
			unsigned char a = ((~PINB & 0x3f));
			sprintf( str,"c=%x ---  B=%x   ~b%x\n",PINC & 0x3f, PINB & 0x3f , a);
			Serial.print(str);

		}
	    break;
		
    case CmdOutput:
		Serial.println("output");
		DDRB = cmdList[i] & 0x3f;
		DDRC = (cmdList[i]>>6) & 0x3f;
		delay(20);		
				
		break;
		
	case CmdDelay:
		Serial.println("delay");
		delay(cmdList[i] & DataMask);
		
		break;
		
	case CmdLongDelay:
		Serial.println("delay Long");
		delay( (cmdList[i] & DataMask) *1000);
		
		break;
	
	case CmdBtnRepeat:
	{
		if( (cmdList[i+1] & CmdMask) == CmdEOF)	//prevents this command from incrementing past the end of the array
			break;
		Serial.println("button repeat");
		unsigned short int j = (cmdList[i] & DataMask);
		unsigned short int tempDDRB = DDRB;		//these allow toggling of only the buttons that the data specifies while leaving the rest alone
		unsigned short int tempDDRC = DDRC;
		i++;	//increments command here so we are looking at button data of the next command and the next command gets skipped over instead of executing an additional time
		
		while(j--)
		{
			Serial.println(j);
			DDRB = tempDDRB | (cmdList[i] & 0x3f);
			DDRC = tempDDRC | ( (cmdList[i]>>6) & 0x3f);
			delay(20);
			DDRB = tempDDRB;	//returns buttons to the state that they entered in
			DDRC = tempDDRC;
			delay(20);
		}
	}
		break;
	
	default:
		Serial.println("Done");
		Serial.println(cmdList[i] & DataMask);
		while(1)
		{
			delay(20);
		}
    }

	i++;
	
    /*
    if ( digitalRead( 14 ) == 0 )
    {
      Serial.println ( "p14 pressed");
      pinMode(5, OUTPUT);
      delay(20);
      pinMode(5, INPUT);
      while( !digitalRead( 14 ) )
      {
      }
      delay(5);
    }
    */
}

