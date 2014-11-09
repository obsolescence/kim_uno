#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <conio.h>
#include <stdint.h>


//#define DEBUGUNO


#pragma warning(disable : 4996) // MS VC2008 does not like unsigned char -> signed char converts.

#ifdef DEBUGUNO
//--------------- debug
FILE *fpx;
void debug_setup();
extern uint16_t pc;
// --------------
#endif

//#define WREG_OFFSET 0x03DF
#define WREG_OFFSET 0x0360

uint8_t curkey = 0;
char threeHex[3][2]; // buffer for KIM's onboard LEDs
uint8_t eepromProtect=0;  // redundant - used for Eeprom write protection on Arduino.
int blitzMode=0;  // microchess status variable. 1 speeds up chess moves (and dumb down play)
//int keyboardState = 0; // set to 3 to simulate GO keypress after entering calculator Enter Ops

extern void exec6502(int32_t tickcount);
extern void reset6502(void);
extern void nmi6502(void);
extern void initKIM(void);
extern void loadTestProgram(void);

void serout(uint8_t val);
void serouthex(uint8_t val);
uint8_t getAkey(void);
void clearkey(void);
void printhex(uint16_t val);
void setup (void);


// Calculator functions:
/*void enterCalcValue(uint8_t reg);
void viewCalcValue(uint8_t reg);
void enterCalcOp();
*/
extern uint8_t enterflt(uint8_t reg);
extern uint8_t showflt(uint8_t reg);
extern uint8_t enteroperation(void);



extern uint8_t RAM[1024]; 		// main 1KB RAM		 0x000-0x3FF
extern uint8_t SSTmode;
extern uint8_t useKeyboardLed;
extern uint16_t pc;


void serout(uint8_t val) { printf("%c",val); }
void serouthex(uint8_t val) { printf("%x",val);}
uint8_t getAkey() { return(curkey); }

uint8_t getKIMkey() {
//	printf("..................................%d", curkey);

	if (curkey==0)
//		if (keyboardState==3) // simulte a GO keypress after Ctrl-V for entering calc operation
//		{   curkey=7; keyboardState=0;	}
//		else
			return (0xFF);	//0xFF: illegal keycode 
	
	if ((curkey>='0') & (curkey <='9'))
		return (curkey-'0');
	if ((curkey>='A') & (curkey <='F'))
		return(curkey-'A'+10);
	if ((curkey>='a') & (curkey <='f'))
		return(curkey-'a'+10);

	if (curkey==1) // ctrlA
		return(0x10); // AD address mode
	if (curkey==4) // ctrlD
		return(0x11); // DA data mode
	if (curkey=='+') // +
		return(0x12); // step
	if (curkey==7) // ctrlG
		return(0x13); // GO
	if (curkey==16) // ctrlP
		return(0x14); // PC mode

    return(curkey); // any other key, should not be hit but ignored by KIM
}

void clearkey() { curkey = 0; }
void printhex(uint16_t val) { printf("%x\r\n", val); }

void setup () 
{
	reset6502();
	initKIM(); // Enters 1c00 in KIM vectors 17FA and 17FE. Might consider doing 17FC as well????????
	loadTestProgram();
}


int main( int argc, char* argv[] )
{
//	char slot;

	setup();
	#ifdef DEBUGUNO
	debug_setup();
	fpx = NULL;
	#endif

	do {
#ifdef DEBUGUNO
		exec6502(1); //do 1 6502 instructions

		if((pc==0xC000) && (fpx==NULL))
				fpx = fopen("c:\\temp27\\debug_uno.csv", "w"); 

#else
		exec6502(100); //do 100 6502 instructions
#endif
		if(_kbhit()) {
            curkey = _getch();

			if (curkey==18) // CtrlR = RS key = hardware reset (RST)
			{	reset6502(); clearkey(); 
				printf("reset pressed\r\n"); 
			}
			if (curkey==20) // CtrlT = ST key = throw an NMI to stop execution of user program
			{	nmi6502(); clearkey(); 
				printf("ST key pressed\r\n"); 
			}
			if (curkey==']') // switch on SST
			{	SSTmode = 1; clearkey();
				printf("                                      SST ON "); 
			}
			if (curkey=='[') // switch off SST
			{	SSTmode = 0; clearkey();
				printf("                                      SST OFF"); 
			}

			if (curkey==9) { //TAB
		    	if (useKeyboardLed==0) 
  		    	{ useKeyboardLed=1;    printf("\r                    Keyboard/Hex Digits Mode "); }
      			else 
      			{ useKeyboardLed=0;    printf("\r                        Serial Terminal Mode         ");}
      			reset6502();
  			}
   
  			if (curkey=='>') // eeprom is read-only
  			{	if (eepromProtect==0) {
          		eepromProtect = 1; // redundant in PC version: 0x400-0x7FF is not write protected, it's just RAM
           		printf("\r                                   Eeprom N/A     "); 
        	} 
			else {
            	eepromProtect = 0;
          		printf("\r                                   Eeprom N/A     "); 
        	}
        	clearkey();
  			}

/*			if (curkey=='/') {
				slot = _getch() - '0';
				if (slot<0 || slot>9)
					printf("\r                        Error - load slots 0-9       ");
				else {
				
				}
			}
*/
			if (curkey==22) {		// A Ctrl V
//				enterCalcOp();
				printf("Input register values: \n");
				showflt(0);	showflt(1);	showflt(2);	
				enteroperation();
			}
			if (curkey==23) {
//				enterCalcValue(0); // W1 Ctrl W
				enterflt(0);
			}
			if (curkey==24) {
//				enterCalcValue(1); // W2 Ctrl X
				enterflt(1);
			}
			if (curkey==25) {
//				enterCalcValue(2); // W3 Ctrl Y
				enterflt(2);
			}
			if (curkey==26) {
//				enterCalcValue(3); // W3 Ctrl Z
				enterflt(3);
			}
 		}
#ifdef DEBUGUNO
//	} while (pc!=0xD01D);
	} while ((pc!=0x1c00) && (curkey!=26));
#else
	} while (1);
#endif


}


// CRBond calculator IO functions


#ifdef DEBUGUNO
void debug_setup()
{
	uint8_t	i;
	uint8_t bootcode[] = 
	{	0xA9, 0x40, 
		0x8D, 0xE1, 0x03, 
		0xA9, 0x20, 
		0x8D, 0xE9, 0x03, 
		0xA9, 0x02, 
		0x20, 0x00, 0xD0, 
		0x00};

	for (i=0;i<16;i++)
		RAM[i+0x0200]=bootcode[i];		// copy in bootcode from d000

	printf("Display of bootcode at 0200:\n");
	for (i=0;i<16;i++)
		printf("  %x ", RAM[i+0x0200]);
	for (i=0;i<16;i++)							// clear W1 register (4 will be entered by bootcode)
		RAM[i+WREG_OFFSET] = 0x00;

	printf("\n");
}

#endif



uint8_t enterflt(uint8_t reg)		// result code -1 = cancel, 0 = good
{
	uint8_t fltstr[32];					// display string
	uint8_t decpt = 0xFF, expt = 0xFF;	// pointers to start of mantissa & exponent in string
	uint8_t mntsign = 0, expsign = 0;	// 1 means negative for mantissa/exponent
	uint8_t strpos = 0, i, j;				// strpos is position counter in string
	uint8_t mntval = 0, expval = 0;		// parsed value of mantissa & exponent
	uint8_t carry = 0, addToExp;
	int digit;
        uint16_t offset;
        uint8_t done=0; 
        
	// init
        offset = WREG_OFFSET + 8*reg;
	for (i=0;i<8;i++)
        #ifdef AVRX
		fltstr[i]=65;
        #else
		fltstr[i]='_';
        #endif

	fltstr[8]=0x00;
	#ifndef AVRX
	printf("%s\r", fltstr);
	#endif

	// input loop ---------------------------------------------------------------
	do {
            #ifdef AVRX
            curkey=0;
            driveCalcLEDs(fltstr, decpt);  // xxxxxx decpt may be a problem
            scanKeys();
            if (curkey!=0) {
            #else
			if (_kbhit()) {//}
			curkey = _getch();
            #endif
                
			if (curkey=='+') {
				if (expt==0xFF) {			// not yet into exponent
					mntsign = 1; strpos=0;
                    #ifdef AVRX
			        fltstr[strpos++] = 64;
                    #else
					fltstr[strpos++] = '-';
                    #endif
				} else {						// minus sign relates to exponent
					expsign = 1; strpos=expt;
                    #ifdef AVRX
				    fltstr[strpos++] = 64;
                    #else
					fltstr[strpos++] = '-';
                    #endif
				}
			}
			if ((curkey>='0') && (curkey<='9')) {
				fltstr[strpos++] = curkey;
			}
			if (curkey=='B') {
				expt = strpos;
                #ifdef AVRX
				fltstr[strpos++] = 62;
                #else
				fltstr[strpos++] = 'E';
                #endif
			}
			if (curkey=='A') {
				decpt = strpos-1;
			}

                        #ifdef AVRX
			fltstr[strpos] = 65;
                        #else
			fltstr[strpos]=0x00;		// terminate into nice string
                        #endif
		}
		#ifndef AVRX
		printf("%s\r", fltstr);
		#endif
	} while ((strpos<8) && (curkey!=7) && (curkey!=19) && (1!=(curkey>='C') && (curkey<='F')));

	if (curkey==19)
		return(-1);						// cancel

	// parse into 8 byte fltpt65 format -----------------------------------------------------------------
	// Ugly, horrible code. But running out of Arduino memory means C library calls must be avoided.
	
	if (expt==0xFF)		// if no E was entered, let it start at end of string and be 0 length
		expt = strpos;
	if (decpt==0xFF)	// if no dec pt was entered, put it at end of mantissa, just before the E
		decpt = expt-1;
	addToExp = decpt - mntsign;	// normalise mantissa: how much to add to exp to have 1.2345 format

	// Exponent 3: parse and adjust exponent value to get normalised 1.23 format using addToExp
	if ((strpos-1)>expt)	// user at least entered 1 exp digit
		digit = (expsign==1?-1:1) * ((int) fltstr[strpos-1]-48) + (int) addToExp;	//expsign*-1: deal with negative exps
	else
		digit = (int) addToExp;			// user entered number without exp. So exp is 0.
	if (digit<0)
		digit = -(digit);		// do not want to use abs() function-arduino out of memory space :)
	RAM[offset+1] = (digit<=9?digit:digit-10);	// store adjusted exp digit
	addToExp = (digit<=9?0:1);					// simple carry mechanism: add could overflow to 2nd sigit

	// Exponent 2: same thing.
	if ((strpos-2)>expt)	// user entered a second exp digit
		digit = (expsign==1?-1:1) * ((int) fltstr[strpos-2]-48) + (int) addToExp;	//expsign*-1: deal with negative exps
	else
		digit = (int) addToExp;			// user entered number without exp. So exp is 0.
	if (digit<0)
		digit = -(digit);		// do not want to use abs() function-arduino out of memory space :)
	RAM[offset+1] |= (digit<=9?digit:digit-10)<<4;	// store adjusted exp digit in upper nibble
	addToExp = (digit<=9?0:1);					// simple carry mechanism: add could overflow to 2nd sigit
	
	// Exponent 1: same thing.
	if ((strpos-3)>expt)	// user entered a second exp digit
		digit = ((int) fltstr[strpos-3]-48) + (int) addToExp;	// there is no carry or add to exp in digit 3
	else
		digit = (int) addToExp;			// user entered number without exp. So exp is 0.
	if (digit<0)
		digit = -(digit);		// do not want to use abs() function-arduino out of memory space :)
	RAM[offset+0] = (digit<=9?digit:digit-10);	// store adjusted exp digit in lower nibble

	// Sign bits
	RAM[offset+0] |= ((mntsign<<7) | (expsign<<6));
//	printf("%u %u ", (RAM[offset+0] & 0xF0)>>4, RAM[offset+0] & 0x0F);
//	printf("%u %u \n", (RAM[offset+1] & 0xF0)>>4, RAM[offset+1] & 0x0F);

	// print mantissa
	j = mntsign;
	for (i=0;i<12;i++)
	{
		if (j<expt)
			RAM[offset+2+i] = (fltstr[j]-48)<<4;
		else
			RAM[offset+2+i] = 0;
		j++;

		if (j<expt)
			RAM[offset+2+i] |= (fltstr[j]-48);
		j++;

//		printf("%u %u ", (RAM[offset+2+i] & 0xF0)>>4, RAM[offset+2+i] & 0x0F);
	}
//	printf("\n");
	return (curkey);  // return value, if not -1, can be used to jump to next register value entry call
} // end function

uint8_t showflt(uint8_t reg)	// returns location of decimal point in string
{
	uint8_t fltstr[32];					// display string
	uint8_t mntsign = 0, expsign = 0;	// 1 means negative for mantissa/exponent
	uint8_t cnt, expt, i;	// decpt,
        uint16_t offset;
	int exp, decpt;

	// init
        offset = WREG_OFFSET + 8*reg;
	for (i=0;i<8;i++)
		fltstr[i]='_';  // no longer necessary I think
	// calculate exponent
	exp = (RAM[offset+1] & 0x0F) + 10*((RAM[offset+1] & 0xF0)>>4) + 100*(RAM[offset+0] & 0x0F);
	printf("\n\nexp = %d\n", exp);

	// determine maximum exponent value we can show as normal number without E
	mntsign = (RAM[offset+0] & 0x80)>>7;	// negative mantissa: 1
	expsign = (RAM[offset+0] & 0x40)>>6;	// negative exponent: 1
	decpt = (mntsign==0?0:1);					// dec point is after digit0 (+ values) or digit1 (- values)
	
	// with pos numbers, any E between 0 and 7 can be polished away. If there's a '-', one less
	if ((exp>0) && (exp<(7 - mntsign)) && (expsign==0))
	{	// yes, we can polish E away
		decpt +=exp;
		expt = 0;
	}
/*		else // negative exponent
		{	decpt -=exp;
			// need to ROR the string's digits or decpt gets to be negative!

			expt = 0;
		}
	}
*/	else						// we need to show exponent, how many digits?
	{	expt = 0;
		if (exp>0) 
		{	expt = 2; //1;
			fltstr[6] = (RAM[offset+1] & 0x0F) + 48;
		}
		if (exp>9) 
		{	expt = 3; //2;
			fltstr[5] = (uint8_t) ((RAM[offset+1] & 0xF0)>>4) + (uint8_t) 48;
		}
		if (exp>90) 
		{	expt = 4; //3;
			fltstr[4] = (RAM[offset+0] & 0x0F) + 48;
		}
                #ifdef AVRX
		fltstr[7-expt] = (expsign==1?64:62);
                #else
		fltstr[7-expt] = (expsign==1?'-':'E');
                #endif
	}

	// fill string with mantissa
	cnt=0;
	if (mntsign==1)
                #ifdef AVRX
		fltstr[0] = 64;
                #else
		fltstr[0]='-';
                #endif

	for (i=mntsign;i<(7-expt);i=i+2)
	{
		fltstr[i] = (uint8_t) ((RAM[offset + 2 + cnt ] & 0xF0)>>4) + (uint8_t) 48;
		printf(" %c ", (uint8_t) ((RAM[offset + 2 + cnt ] & 0xF0)>>4) + (uint8_t) 48);
		if ((i+1)<(7-expt)) // bug fix 20141007
		{fltstr[i+1] = (RAM[offset + 2 + cnt ] & 0x0F) + 48;
		printf(" %c   ", (RAM[offset + 2 + cnt ] & 0x0F) + 48);
		}
		cnt++;
	}
	fltstr[7]=0x00;	// string terminator

//	printf("\n\n%s\n", fltstr);

        #ifdef AVRX
        curkey=0;
        do { driveCalcLEDs(fltstr, decpt);          scanKeys();
        } while (curkey==0);
        if ((curkey<'C') || (curkey>'F'))
          curkey=0;  // to clear any keypresses before returning to KIM    
        #else
        // show number with dec point inserted
	for (i=0;i<=decpt;i++)
		printf("%c", fltstr[i]);
	printf(".");
	for (i=decpt+1;i<7;i++)
		printf("%c", fltstr[i]);
	printf("\n");
        #endif
        
	return decpt;						// pointers to start of mantissa & exponent in string
} // end function


uint8_t enteroperation(void)		// result code -1 = cancel, 0 = good
{
	uint8_t fltstr[8];					// display string
	uint8_t strpos = 4, i;				// strpos is position counter in string

	// init
        #ifdef AVRX
	for (i=0;i<8;i++)  fltstr[i]=66;
	fltstr[4]=65;  fltstr[5]=65;
        #else
	for (i=0;i<8;i++)  fltstr[i]=' ';
	fltstr[4]='_';  fltstr[5]='_';
        #endif

	// input loop ---------------------------------------------------------------
	do {
                #ifdef AVRX
                curkey=0;
                driveCalcLEDs(fltstr, 5);  scanKeys();
                if (curkey!=0) {
                #else
		if (_kbhit()) { // } this curly brace is there so Arduino IDE does not miscount when it does { } highlighting
			curkey = _getch();
                #endif
                
			if ((curkey>=48) && (curkey<=57))     //only allow dec digits
				fltstr[strpos++] = curkey;

                        #ifdef AVRX
			fltstr[strpos] = 65;
                        #else
			fltstr[strpos]=0x00;		// terminate into nice string
//			printf("> %s  ms%u es%u expt%u decpt%u\r", fltstr, mntsign, expsign, expt, decpt);
                        #endif
		}
	} while ((strpos<6) && (curkey!=7) && (curkey!=19));

	if (curkey==19)
		return(-1);						// cancel

        RAM[0x00F3]= (fltstr[4]-48)*10 + (fltstr[5]-48);  // operation to go into 6502 A register
        RAM[0x00EF] = (uint8_t) 0xE0;  RAM[0x00F0] = (uint8_t) 0x6F;                // set PC register to fltpt65 start address (0x6FE0, not 0x5000, we need to start with a JSR and end with a BRK)
        curkey = 16;  // PC
       
        return(RAM[0x00F3]);
}
