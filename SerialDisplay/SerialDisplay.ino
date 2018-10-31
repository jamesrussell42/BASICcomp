/*
  LiquidCrystal Library - Serial Input

 Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
 library works with all LCD displays that are compatible with the
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.

 This sketch displays text sent over the serial port
 (e.g. from the Serial Monitor) on an attached LCD.

 The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe
 modified 7 Nov 2016
 by Arturo Guadalupi

 This example code is in the public domain.

 http://www.arduino.cc/en/Tutorial/LiquidCrystalSerialDisplay

*/
int screenMem[80];
int cursorX = 0;
// include the library code:
#include <LiquidCrystal.h>

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

static void lcdChar(byte c) {

  if (c == 8) { //Backspace?
  
    if (cursorX > 0) {
  
      cursorX -= 1; //Go back one
      screenMem[60 + cursorX] = 32; //Erase it from memory
      doFrame(60 + cursorX); //Redraw screen up to that amount
      
    }
  
  }

  if (c != 13 and c != 10 and c != 8) { // Not a backspace or return, just a normal character
  
    screenMem[60 + cursorX] = c;
    cursorX += 1;
    if (cursorX < 20) {
    
      lcd.write(c);
    
    }
    
  }
  
  if (cursorX == 20 or c == 10) { // Did we hit Enter or go type past the end of a visible line?
  
    // Lines are interlaced as described here: http://www.element14.com/community/message/81507/l/re-basic-pocket-computer-display-modification#81507
    for (int xg = 0 ; xg < 20 ; xg++) {

      screenMem[0 + xg] = screenMem[40 + xg];
      screenMem[40 + xg] = screenMem[20 + xg];    
      screenMem[20 + xg] = screenMem[60 + xg];
      screenMem[60 + xg] = 32;
    
    
    }
  
    cursorX = 0;
    
    doFrame(60);  

  }


}

static void doFrame(byte amount) {

  lcd.clear();
  lcd.noCursor();
    
  for (int xg = 0 ; xg < amount ; xg++) {
  
    lcd.write(screenMem[xg]);
  }

  lcd.cursor();

}

void setup() {
Serial.begin(9600); // opens serial port
    lcd.begin(20, 4);

  for (int xg = 0 ; xg < 80 ; xg++) {
    screenMem[xg] = 32;
  }

  lcd.blink();
  
  doFrame(80);
}

void loop() {
  // when characters arrive over the serial port...
  if (Serial.available()) {
    // wait a bit for the entire message to arrive
    //delay(10);
    // read all the available characters
    while (Serial.available() > 0) {
      // display each character to the LCD
      lcdChar(Serial.read());
    }
  }
}
