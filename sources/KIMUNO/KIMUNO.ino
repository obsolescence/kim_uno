#include "Arduino.h"
#include <EEPROM.h>
#include <stdint.h>
#include <util/delay.h>
#include "x.h"

//#define CH2_SPEAKERPIN 9
#define WREG_OFFSET 0x0360
#define AVRX		// define on for Arduino, off for normal PC C compilers. 
                        // in kimuno.ino, this only relates to some code segments that are shared with PC version

extern uint8_t RAM[1024]; 		// main 1KB RAM		 0x000-0x3FF
extern uint16_t pc;  // 6502 pc
extern uint8_t SSTmode;
extern uint8_t useKeyboardLed; // 0 to use Serial port or 1 for HEX digits.
uint8_t curkey = 0;
uint8_t eepromProtect=1;  // default is to write-protect EEPROM
int blitzMode=1;  // microchess status variable. 1 speeds up chess moves (and dumbs down play)
uint8_t keyboardMode=0;  // start with keyboard in KIM-1 mode. 1: calculator mode

char threeHex[3][2];        // LED display

byte aCols[8] = { A5, 2,3,4,5,6,7,8 }; // note col A5 is the extra one linked to DP
byte aRows[3] = { 9,10,11 };
byte ledSelect[8] =  { 12, 13, A0, A1, A2, A3, A7, A4 }; // note that A6 and A7 are not used at present. Can delete them.
byte ledSelect7[8] = { 12, 13, A0, A1, A4, A2, A3, A7 }; // note that A6 and A7 are not used at present. Can delete them.

byte dig[19] = {
// bits     6543210
// digits   abcdefg
          0b01111110,//0  
          0b00110000,//1
          0b01101101,//2
          0b01111001,//3
          0b00110011,//4
          0b01011011,//5
          0b01011111,//6
          0b01110000,//7
          0b01111111,//8
          0b01111011,//9
          0b01110111,//a
          0b00011111,//b
          0b01001110,//c
          0b00111101,//d
          0b01001111,//e
          0b01000111,//f
          0b00000001, //g printed as -
          0b00001000, //h printed as _
          0b00000000  //i printed as <space>
};



extern "C" {
  // ---------- in cpu.c ------------------------------
  uint16_t getpc();
  uint8_t getop();
  void exec6502(int32_t tickcount);
  void reset6502();
  void nmi6502();
  void initKIM(void);
  void loadTestProgram(void);
  extern void driveLEDs();
  extern void driveCalcLEDs(uint8_t reg);
//  extern void scanKeys(); 
void scanKeys(); 


//  extern uint8_t getRegister(uint8_t reg, uint8_t *inVal);
//  extern void setRegister(uint8_t reg, double regVal, uint8_t *inVal);
  
  // ---------- called from cpu.c ----------------------
  void serout(uint8_t val)    { Serial.write(val);  }
  void serouthex(uint8_t val) { Serial.print(val, HEX); }
  uint8_t getAkey()            { return(curkey);  }
  void clearkey()             { curkey = 0; }
  void printhex(uint16_t val) { Serial.print(val, HEX); Serial.println(); }



  // getKIMkey() translates ASCII keypresses to codes the KIM ROM expects.
  // note that, inefficiently, the KIM Uno board's key codes are first translated to ASCII, then routed through
  // here just like key presses from the ASCII serial port are. That's inefficient but left like this
  // for hysterical raisins.
  
  
  uint8_t getKIMkey() {
    //Serial.print('#');  Serial.print(curkey);  Serial.print('#');

if (curkey==0)
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
    // curkey==ctrlR for hard reset (/RST) (KIM's RS key) is handled in main loop!
    // curkey==ctrlT for ST key (/NMI) is handled in main loop!
    return(curkey); // any other key, should not be hit but ignored by KIM
  }
  
  uint8_t eepromread(uint16_t eepromaddress) {
    EEPROM.read(eepromaddress);
  }
  void eepromwrite(uint16_t eepromaddress, uint8_t bytevalue) {
    if (eepromProtect==0)
      EEPROM.write(eepromaddress, bytevalue);
    else
    {  Serial.println(F("ERROR: EEPROM STATE IS WRITE-PROTECTED. HIT '>' TO TOGGLE WRITE PROTECT"));
      Serial.println(freeRam());
    }
  }
}

void setup () {
  Serial.begin (9600);
  Serial.println ();

  setupUno();

  reset6502();
  initKIM(); // Enters 1c00 in KIM vectors 17FA and 17FE. Might consider doing 17FC as well????????
  loadTestProgram();
  
  Serial.print(F("v23oct14 freeMemory()=")); // just a little check, to avoid running out of RAM!
  Serial.println(freeRam());
}

void loop () {
  int y;

  exec6502(100); //do 100 6502 instructions
  
  if (Serial.available())
  { curkey = Serial.read() & 0x7F;
    interpretkeys();
  }
    
  scanKeys();  
  if (xkeyPressed()!=0) //KIM Uno board input?
    interpretkeys();

  //driveLEDs(); // doing that here would cause a massive slowdown but keeps display on at all times
}


// check for out of RAM
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

// translate keyboard commands to KIM-I keycodes or emulator actions
void interpretkeys()
{    
  // round 1: keys that always have the same meaning
  switch (curkey) {
    case 18:  // CtrlR = RS key = hardware reset (RST)
      reset6502(); clearkey(); Serial.print("RSet\n"); break;
    case 20: // CtrlT = ST key = throw an NMI to stop execution of user program
      nmi6502(); clearkey(); Serial.print("STop\n"); break;
    case '[': // SST off
      SSTmode = 0; clearkey();
      Serial.print(F("                                      SST OFF         "));
      break;
    case ']': // SST on
      SSTmode = 1; clearkey();
      Serial.print(F("                                      SST ON          ")); 
      break;
    case 9: // TAB pressed, toggle between serial port and onboard keyboard/display
      if (useKeyboardLed==0) 
      { useKeyboardLed=1;    Serial.print(F("                    Keyboard/Hex Digits Mode ")); }
      else 
      { useKeyboardLed=0;    Serial.print(F("                        Serial Terminal Mode         "));}
      reset6502();  clearkey();  break;
    case '>': // Toggle write protect on eeprom
      if (eepromProtect==0) {
        eepromProtect = 1; 
        Serial.print(F("                                      Eeprom R/O     ")); 
      } else {
        eepromProtect = 0;
        Serial.print(F("                                      Eeprom R/W     ")); 
        delay(20);
      }
      clearkey(); break;
  }
  
  // round 2: keys in Calculator Mode
  if (keyboardMode==1) {  // in case of Calculator Mode
    switch (curkey) {
      case 'C': enterflt((uint8_t)0);  if (!((curkey>='C') && (curkey<='F')))  break;  // if statement allows roll-through to next reg entry
      case 'D': enterflt((uint8_t)1);  if (!((curkey>='C') && (curkey<='F')))  break;
      case 'F': enteroperation();  break;
      case 'E': enterflt((uint8_t)2);  break;

      case   1: showflt(0);  break; // AD
      case   4: showflt(1);  break; // DA
      case  16: showflt(2);  break; // PC
    }
  }
}
  
// =================================================================================================
// KIM Uno Board functions are bolted on from here
// =================================================================================================

void setupUno() {
  int i;
  // --------- initialse for scanning keyboard matrix -----------------
  // set columns to input with pullups
  for (i=0;i<8;i++)
  {  pinMode(aCols[i], INPUT);           // set pin to input
     digitalWrite(aCols[i], HIGH);       // turn on pullup resistors  
  }
  // set rows to output, and set them High to be in Neutral position
  for (i=0;i<3;i++)
  {  pinMode(aRows[i], OUTPUT);           // set pin to output
     digitalWrite(aRows[i], HIGH);       // set to high
  } 
  // --------- clear display buffer ------------------------------------
  for (i=0;i<3;i++)
  { threeHex[i][0]=threeHex[i][0]=0; }
  Serial.println(F("Ready"));
}


extern "C" {
void driveLEDs()
{ 
  int led, col, ledNo, currentBit, bitOn;
  int byt,i;

  // 1. initialse for driving the 6 (now 8) 7segment LEDs
  // ledSelect pins drive common anode for [all segments] in [one of 6 LEDs]
  for (led=0;led<7;led++)
  { pinMode(ledSelect[led], OUTPUT);  // set led pins to output
    digitalWrite(ledSelect[led], LOW); // LOW = not lit
  }
  // 2. switch column pins to output mode
  // column pins are the cathode for the LED segments
  // lame code to cycle through the 3 bytes of 2 digits each = 6 leds
  for (byt=0;byt<3;byt++)
    for (i=0;i<2;i++)
    {
      ledNo=byt*2+i;
      for (col=0;col<8;col++)  
      {  pinMode(aCols[col], OUTPUT);           // set pin to output
         //currentBit = (1<<(6-col));             // isolate the current bit in loop
         currentBit = (1<<(7-col));             // isolate the current bit in loop
         bitOn = (currentBit&dig[threeHex[byt][i]])==0;  
         digitalWrite(aCols[col], bitOn);       // set the bit
      }
      digitalWrite(ledSelect[ledNo], HIGH); // Light this LED 
      delay(2);
      digitalWrite(ledSelect[ledNo], LOW); // unLight this LED
    }
} // end of function
} // end of C segment

void driveCalcLEDs(uint8_t *numberStr, uint8_t decpt)
{ 
  uint8_t led, col, ledNo, currentBit, bitOn;
  uint8_t digit,i;
    
  // 1. initialse for driving the 6 segment LEDs
  for (led=0;led<7;led++)
  { pinMode(ledSelect7[led], OUTPUT);  // set led pins to output
    digitalWrite(ledSelect7[led], LOW); // LOW = not lit
  }
  // 2. switch column pins to output mode
  for (digit=0;digit<7;digit++)
  {  for (col=0;col<8;col++)  
    {  pinMode(aCols[col], OUTPUT);           // set pin to output
       currentBit = (1<<(7-col));             // isolate the current bit in loop
       bitOn = (currentBit&dig[numberStr[digit]-48])==0;  
       if (col==0 && digit==decpt) // show decimal point here?
         bitOn=0; // 0 being light led.
       digitalWrite(aCols[col], bitOn);       // set the bit
    }
    digitalWrite(ledSelect7[digit], HIGH); // Light this LED 
    delay(2);
    digitalWrite(ledSelect7[digit], LOW); // unLight this LED
  }
} // end of function
//} // end of C segment


uint8_t parseChar(uint8_t n) //  parse keycode to return its ASCII code
{
  uint8_t c;
    
  // KIM-I keys
  switch (n-1) {              //KIM Uno keyscan codes to ASCII codes used by emulator
    case	7	: c = 	'0' ;  break;	//        note: these are n-1 numbers!
    case	6	: c = 	'1';  break;	// 
    case	5	: c = 	'2';  break;	// 
    case	4	: c = 	'3';  break;	//
    case	3	: c = 	'4';  break;	//
    case	2	: c = 	'5';  break;	//
    case	1	: c = 	'6';  break;	//
    case	0	: c = 	20;  break;	// ST
    
    case	15	: c = 	'7' ;  break;	//
    case	14	: c = 	'8';  break;	//
    case	13	: c = 	'9';  break;	//
    case	12	: c = 	'A';  break;	//
    case	11	: c =   'B';  break;	//
    case	10	: c =   'C';  break;	//
    case	9	: c =   'D';  break;	//
    case	8	: c =   18;  break;	// RS
    
    case	23	: c =  'E';  break;     //
    case	22	: c =  'F';  break;     //
    case	21	: c = 	1;   break;     // AD
    case	20	: c = 	4;   break;     // DA
    case	19	: c = 	'+'; break;     // + 
    case	18	: c = 	7;   break;	// GO
    case	17	: c =   16;  break;	// PC
    case	16	: c = (SSTmode==0?']':'[');  break; // 	SST toggle
  }
  return c;
}

uint8_t xkeyPressed()    // just see if there's any keypress waiting
{ return (curkey==0?0:1); }


extern "C" {  // the extern C is to make function accessible from within cpu.c
void scanKeys() 
{
  int led,row,col, noKeysScanned;
  static int keyCode = -1, prevKey = 0;
  static unsigned long timeFirstPressed = 0;
    
  // 0. disable driving the 7segment LEDs -----------------
  for (led=0;led<8;led++)
  { pinMode(ledSelect[led], INPUT);  // set led pins to input
                                  // not really necessary, just to stop them
                                 // from driving either high or low.
    digitalWrite(ledSelect[led], HIGH); // Use builtin pullup resistors
  }
  // 1. initialise: set columns to input with pullups
  for (col=0;col<8;col++)
  {  pinMode(aCols[col], INPUT);           // set pin to input
     digitalWrite(aCols[col], HIGH);       // turn on pullup resistors  
  }
  // 2. perform scanning
  noKeysScanned=0;

  for (row=0; row<3; row++)
  { digitalWrite(aRows[row], LOW);       // activate this row     
    for (col=0;col<8;col++)
    { if (digitalRead(aCols[col])==LOW)  // key is pressed
      { keyCode = col+row*8+1;
        if (keyCode!=prevKey)
        {   //Serial.println();
            //Serial.print(" col: ");  Serial.print(col, DEC); 
            //Serial.print(" row: ");  Serial.print(row, DEC); 
            //Serial.print(" prevKey: ");  Serial.print(prevKey, DEC); 
            //Serial.print(" KeyCode: ");  Serial.println(keyCode, DEC); 
           prevKey = keyCode;
           curkey = parseChar(keyCode);
            //Serial.print(" curkey: ");  Serial.print(curkey, DEC); 
           timeFirstPressed=millis();  // 
        }
        else // if pressed for >1sec, it's a ModeShift key
        {
          if ((millis()-timeFirstPressed)>1000) // more than 1000 ms
          {
              if (keyCode==17) //it was the SST button
              {
                keyboardMode=(keyboardMode==0?1:0); // toggle
//                Serial.print(F("                                      Eeprom R/O     ")); 
                Serial.print(F("                                keyboardMode: "));  Serial.print(keyboardMode, DEC); 
                SSTmode=0;
                curkey=0;  // don't do anything else with this keypress
              }
              if (keyCode==9) // it was RS button
                curkey = '>';  // toggle eeprom write protect
              timeFirstPressed=millis(); // because otherwise you toggle right back!
          }          
        }
      }
      else
        noKeysScanned++;  // another row in which no keys were pressed
    }     
    digitalWrite(aRows[row], HIGH);       // de-activate this row
  }

  if (noKeysScanned==24)    // no keys detected in any row, 3 rows * 8 columns = 24. used to be 28.
    prevKey=0;        // allows you to enter same key twice 
} // end of function
} // end C segment

extern "C" {  // the extern C is to make function accessible from within cpu.c
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
			// temp protection against entering 0.025, which breaks this code.
                        // temp fix: do not allow entering a 0 after 0.
                          if ((curkey=='0') && (decpt==(strpos-1)) && (fltstr[strpos-1]=='0'))
                          {
                            // do nothing, that's the temp fix
                          }
                          else
                          // end of temp bug fix
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
} // end C segment

extern "C" {  // the extern C is to make function accessible from within cpu.c
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
//	printf("\n\nexp = %d\n", exp);

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
                { fltstr[i+1] = (RAM[offset + 2 + cnt ] & 0x0F) + 48;
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
} // end C segment


extern "C" {  // the extern C is to make function accessible from within cpu.c
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
} // end C segment


