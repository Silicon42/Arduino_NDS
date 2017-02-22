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
#define Unused1 0x0020		//DO NOT USE, blinks during programming and resets

// the following buttons are mapped to PORTC bit 0..5
#define BtnLeft 0x0040
#define BtnRight 0x0080
#define BtnStart 0x0100
#define BtnSelect 0x0200
#define BtnDown 0x0400
#define Unused2 0x0800

#define NoBtns 0x0000		//used to release all buttons using the output command

// The following are values associated with commands
#define CmdMask 0xf000		//bitwise anded to entries in the command list to view only the command portion
#define DataMask 0x0fff		//bitwise anded to entries in the command list to view only the data portion
#define BtnMask 0x001f
#define CmdEOF 0x0000		//reserved for unused entries in array and the end of the array
#define CmdInput 0x1000		//waits for the state of the buttons to match that of the data
#define CmdOutput 0x2000	//sets the state of the buttons to match that of the data, does not override manual presses
#define CmdDelay 0x3000		//delay in milliseconds
#define CmdLongDelay 0x4000	//delay in seconds
#define CmdBtnRepeat 0x5000	//uses the data of the entry immediately after it for the buttons then skips that command
#define CmdSetData 0x6000	//sets the data of the next command to that of setData which is set from the menu

#define PROGRAMS 2	//number of command lists in use

unsigned short* cmdListArray[PROGRAMS];
unsigned char prg_ndx =0;	//for which command list is being used

unsigned short* cmdList = NULL;
unsigned char cmd_ndx = 0;		//for which command is currently being executed

/*
We first have to wait until the game is in the reset position, then we start after 
the A button is released (A must be released first for the timings to be right). 
Next, we wait a bit and press the A button 3 times to get past the intro and title 
screens and wait again to let the the first half of the dead clock battery message 
go by, press A, and wait again, and press a to start the game.
*/
unsigned short  cmdList1[22] = 
{CmdInput|(BtnA|BtnB|BtnStart|BtnSelect), CmdInput|(BtnB|BtnStart|BtnSelect), CmdDelay|2000, CmdBtnRepeat|3, CmdOutput|BtnA, CmdOutput|BtnA, 
CmdDelay|1000, CmdOutput|NoBtns, CmdOutput|BtnA, CmdDelay|1000, CmdOutput|NoBtns, CmdOutput|BtnA, CmdBtnRepeat|40, CmdOutput|BtnUp, CmdEOF|0x0001};  //the array must be terminated with CmdEOF

unsigned short  cmdList2[22] = {CmdSetData, CmdDelay|0, CmdEOF|0x0002};


void setup()
{
    // put your setup code here, to run once:
    // initialize the digital pin as an output.
    // Pin 13 has an LED connected on most Arduino boards:
	
	cmdListArray[0] = cmdList1;
	cmdListArray[1] = cmdList2;
	
	PORTB = 0x00;		//NDS buttons
	DDRB = 0;		//0 = input, 1 = output(pressed)

	PORTC = 0x00;
	DDRC = 0;
	
	digitalWrite(ExtBtn1, HIGH);		//external buttons
	digitalWrite(ExtBtn2, HIGH);
	digitalWrite(ExtBtn3, HIGH);
	digitalWrite(ExtBtn4, HIGH);
	
	Serial.begin(57600);         // lower the BAUD rate if this runs on 8mhz without an x-tal as per fuse settings
	Serial.println("Ready");
}

void loop()
{
	char str[20];
	unsigned char i;
    // put your main code here, to run repeatedly:
	while(1)
	{
		unsigned short setData;
		
		if(cmdList == NULL)
		{
			DDRB = 0;	//resets button positions to unpressed so they aren't pressed by the arduino outside of a program
			DDRC = 0;
			
			if(!digitalRead(ExtBtn3) )
			{
				Serial.println("Start!");
				cmdList = cmdListArray[prg_ndx];
				cmd_ndx = 0;
			}
			
			if(!digitalRead(ExtBtn4) )
			{
				if(prg_ndx <= PROGRAMS -2)
					prg_ndx++;
					
				else
					prg_ndx = 0;
					
				Serial.println(prg_ndx);
			}
			
			if(!digitalRead(ExtBtn2) )
			{
				char tmp[5];
				while(Serial.read() != -1)
				{}
				Serial.println("Enter a Set Data value:");
				i=0;
				while (i < 5)
				{
					char x;
					if( (x = Serial.read() ) == -1)
						continue;
					if( x == '\r')
					{
						tmp[i] = 0;
						break;
					}
					else
					{
						tmp[i] = x;
					}
					
					i++;
				}
				if(i >= 5)
				{
					tmp[0] = 0;
					Serial.println("Too many characters");
				}
				
				Serial.println(tmp);
				setData = atoi(tmp) & DataMask;
			}
			
			delay(100);
		}
		
		else
		{
			switch ( cmdList[cmd_ndx] & CmdMask )
			{

			case CmdInput:		//waits for the state of the buttons to match that of the data
				Serial.println("waiting for input");

				while( !( (~PINC & BtnMask) == ( (cmdList[cmd_ndx]>>6) & BtnMask)  && (~PINB & BtnMask) == (cmdList[cmd_ndx] & BtnMask) ) )		//input logic is inverted thus ~PINB/~PINC is necessary
				{
					delay(50);
					unsigned char a = ((~PINB & BtnMask));
					sprintf( str,"c=%x ---  B=%x   ~b%x\n",PINC & BtnMask, PINB & BtnMask , a);
					Serial.print(str);
					
					if(!digitalRead(ExtBtn1) )	//allows aborting in the middle of this command instead of waiting for button combination first
						break;
				}
				break;
				
			case CmdOutput:		//sets the state of the buttons to match that of the data, does not override manual presses
				Serial.println("output");
				DDRB = cmdList[cmd_ndx] & BtnMask;
				DDRC = (cmdList[cmd_ndx]>>6) & BtnMask;
				delay(50);
				
				break;
				
			case CmdDelay:		//delay in milliseconds
				Serial.println("delay");
				Serial.println(cmdList[cmd_ndx] & DataMask);
				delay(cmdList[cmd_ndx] & DataMask);
				
				break;
				
			case CmdLongDelay:	//delay in seconds
				Serial.println("long delay");
				Serial.println(cmdList[cmd_ndx] & DataMask);
				delay( (cmdList[cmd_ndx] & DataMask) *1000);
				
				break;
			
			case CmdBtnRepeat:	//uses the data of the entry immediately after it for the buttons then skips that command
			{
				if( (cmdList[cmd_ndx+1] & CmdMask) == CmdEOF)	//prevents this command from incrementing past the end of the array
					break;
				Serial.println("button repeat");
				i = (cmdList[cmd_ndx] & DataMask);
				unsigned char tempDDRB = DDRB;		//these allow toggling of only the buttons that the data specifies while leaving the rest alone
				unsigned char tempDDRC = DDRC;
				cmd_ndx++;	//increments command here so we are looking at button data of the next command and the next command gets skipped over instead of executing an additional time
				
				while(i--)
				{
					Serial.println(i);
					DDRB = tempDDRB | (cmdList[cmd_ndx] & BtnMask);
					DDRC = tempDDRC | ( (cmdList[cmd_ndx]>>6) & BtnMask);
					delay(50);
					DDRB = tempDDRB;	//returns buttons to the state that they entered in
					DDRC = tempDDRC;
					delay(50);
					
					if(!digitalRead(ExtBtn1) )	//allows aborting in the middle of this command instead of waiting for iterations to be done
						break;
				}
			}
				break;
			
			case CmdSetData:	//sets the data of the next command to that of setData which is set from the menu
				cmdList[cmd_ndx+1] = (cmdList[cmd_ndx+1] & CmdMask) | setData;
				Serial.println("data set");
				break;
			
			default:
				Serial.println("Done");
				Serial.println(cmdList[cmd_ndx] & DataMask);
				cmdList = 0;
			}
			
			if(!digitalRead(ExtBtn1) )
			{
				Serial.println("Stopped");
				cmdList = 0;
			}
			
			cmd_ndx++;
		}
	}
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

