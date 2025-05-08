/* RS232 LCD SERIAL TESTER
**HARDWARE:**
- Arduino Uno
- LCD Button shield
- RS232 serial shield (see connection)

**LIBRARY:**
- LiquidCrystal https://github.com/arduino-libraries/LiquidCrystal

**LCD:**
- 2x16 character lcd display.
- Serial Data prints on line 1. Menu prints on line 2.

**MENUS:**
- Menu 1 = Baud rate
- Menu 2 = Send counter on/off
- Menu 3 = Send Preset string
- Menu 4 = Record on/off
- Menu 5 = playback
- Menu 6 = Display non chars on/off
- Menu 7 = RX bytes counter (bytes received)

**CONNECTION:**
- RX PIN 0, TX PIN 1.


*/

//EEPROM STRING
#include <EEPROM.h>
int addr = 0;// the current address in the EEPROM (i.e. which byte we're going to write to next)
boolean rec = false; //rec on/off controll

//LCD
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7); // pins for lcd display
const int lcdBacklightPin = 10;
unsigned long lastLight = 0; //lcd led


//KEYS
long baud[] = {300, 1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, 115200, 256000};
byte baudIndex = 4; // default index 4(9600)
//int adc_key_val[5] = {50, 200, 400, 600, 800 }; //ANALOG VALUES FROM BUTTON SHIELD
int adc_key_val[5] = {50, 250, 450, 650, 850 }; // OTHER ANALOG VALUES, thanks to Tim.
byte NUM_KEYS = 5;
int adc_key_in;
byte key = -1;
byte oldkey = -1;
unsigned long lastHold = millis();
boolean holding = true; //sperr

//AUTOSEND
long previousMillis = 0;        // store last time x was updated
unsigned int testCounter = 0;
boolean sendCounterState = false;

//MENU
const byte menuTotalItems = 8; //number of total items in menu
byte menuSelectedItem = 1;
unsigned int rxByteCounter = 0;
boolean displayNonChars = false;
boolean echoState = false;

void menuUp() {
  menuSelectedItem++;
  if (menuSelectedItem > menuTotalItems) menuSelectedItem = 1; //reset
  updateMenuSelection();
}

void menuDown() {
  menuSelectedItem--;
  if (menuSelectedItem < 1) menuSelectedItem = menuTotalItems; //reset
  updateMenuSelection();
}

void msgSendt() {
  GOTOlcdLine2();
  lcd.print("Sendt           ");
  delay(100);
}

//RIGHT BUTTON,CHANGE MENU ITEM
void menuItemRightClick() {
  switch (menuSelectedItem) {

    case 1://CHANGE BAUD
      baudIndex++; 
      if (baudIndex > 11) baudIndex = 0;   //reset baudIndex
      //baudIndex % 11;
      Serial.end() ;
      delay(150);
      GOTOlcdLine2();
      Serial.begin(baud[baudIndex]);
      break;

    case 2: // FLIP COUNTER STATE
      sendCounterState = !sendCounterState;
      break;


    case 3: // SEND PRESET
      intermec();
      msgSendt();
      break;


    case 4: //FLIP REC STATE
      if (rec == true) { //DO
        //STOPP
        rec = false;
        //rec end:
        EEPROM.write(addr, 255);
        Serial.println(F("Rec Stoppped"));
      }
      else {
        //START
        addr = 0;
        rec = true;
        Serial.println(F("Rec Started   "));
      }
      break;

    case 5: //START PLAYBACK
      //Serial.println(F("Sending recorded:"));
      playback();
      break;
    
    case 6: //FLIP displayNonChars STATE
      displayNonChars=!displayNonChars;
      break;

    case 7: //CLEAR RX COUNTER
      rxByteCounter=0;
      break;
      
    case 8: //FLIP echoState
      echoState=!echoState;
      break;
      
  }
  updateMenuSelection();
}

//UP DOWN MENU ITEM CYCLE 
void updateMenuSelection() {
  GOTOlcdLine2();
  switch (menuSelectedItem) {


    case 1: //BAUD
      lcd.print("Baud = ");
      lcd.print(baud[baudIndex]);
      lcd.print("      ");
      Serial.print("Baudrate =  ");
      Serial.println(baud[baudIndex]);
      break;


    case 2: //COUNTER
      lcd.print("Send count = ");
      if (sendCounterState == true) {
        lcd.print("On ");
        
      }
      else {
        lcd.print("Off");
      }
      break;

    case 3: //PRESET
      lcd.print("Send Preset      ");
      break;


    case 4: //RECORD
      lcd.print("Record = ");
      if (rec == true) lcd.print("On    ");
      else lcd.print("Off   ");
      break;


    case 5: //playbackK
      lcd.print("Playback        ");
      break;

    case 6: //displayNonChars
      lcd.print("Non char = ");
      if (displayNonChars == true) lcd.print("On   ");
      else lcd.print("Off  ");
      break;
    
    case 7: //display rx bytes
      lcd.print("RX bytes: ");
      lcd.print(rxByteCounter);
      lcd.print("     ");
      break;

    case 8: //echo (return all bytes)
      lcd.print("Echo = ");
      if (echoState == true) lcd.print("On   ");
      else lcd.print("Off  ");
      break;
      
  }
  

  GOTOlcdLine1(); //goto serial rx line when done
}



void setup() {
  backlight(); // controls backlight. see link in function.
  Serial.begin(baud[baudIndex]);
  Serial.println(F("RS232 Serial data tester."));
  Serial.println(F("Smartrak Serial data emulator."));
  Serial.println(F("Use arrow up, down, right to scroll menu."));

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Handheld serial");
  lcd.setCursor(0, 1);
  lcd.print("data tester");
  delay(3000);
  lcd.clear();
  lcd.print("by Mac");
  delay(2000);
  lcd.clear();
  lcd.print("rx pin = 0");
  lcd.setCursor(0, 1);
  lcd.print("tx pin = 1");
  delay(2000);
  lcd.clear();
  lcd.print("Menu = arrow up,");
  lcd.setCursor(0, 1);
  lcd.print("down, right.");
  delay(2000);
  lcd.clear();
  lcd.print("v18 Ready");

  updateMenuSelection();
}

// count bytes, and update menu now if in the menu
void updateMenu6RxBytes(){
      if(menuSelectedItem==7){
      lcd.setCursor(0, 1);
      lcd.print("RX bytes: ");
      lcd.print(rxByteCounter);
      lcd.print("     ");
      }
}

void loop(){

  // SERIAL READ
  if (Serial.available()) { 
    boolean freshdata = true;
    //delay(100);// wait a bit for the entire message to arrive
    //LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
    lastLight = millis(); 
    // read all the available characters
    while (Serial.available() > 0) { 
      char inchar = Serial.read();
      rxByteCounter+=1;

      // clear line 1 when data starts
      if(freshdata){
        GOTOlcdLine1();
        lcd.print("                ");//clear old data on line 1
        freshdata=false;
        GOTOlcdLine1();
      }
 
      // display data, with non chars if selected
      if (displayNonChars) lcd.write(inchar);
      else if (inchar >= 31) lcd.write(inchar); //write only ascii chars to lcd
      
      //echo data back to sender
      if (echoState) Serial.write(inchar);
      


      // RECORD TO EEPROM
      //***********************************************************
      if (rec == true) {
        //SAVE CHAR TO EEPROM
        if (addr <= 1023) {
          EEPROM.write(addr, inchar); 
          addr = addr + 1; 
          GOTOlcdLine1();
          lcd.print("Saved ");
          lcd.print(addr);
          lcd.print(" bytes   ");
          // GOTOlcdLine1();
        }
        else { //FULL
          GOTOlcdLine1();
          lcd.print("EEPROM full      ");
        }
      }
    }// serial end
    
    //SERIAL END, SAVE END TO EEPROM
    //**************************************************
    updateMenu6RxBytes(); //THIS WILL UPDATE COUNTER WHEN RX MESSAGE IS DONE
    GOTOlcdLine1();
    //ADD LINE END 13 Og 10 BYTE TO EEPROM AT SERIAL END.
    if (rec == true) {
      if (addr <= 1023) {
        //EEPROM.write(addr, 13); 
        //addr+=1; 
        //EEPROM.write(addr, 10); 
        //addr+=1; 
        GOTOlcdLine1();
        lcd.print("Saved ");
        lcd.print(addr);
        lcd.print(" bytes   ");
      }
      else { //FULL
        GOTOlcdLine1();
        lcd.print("EEPROM full      ");
      }
    }

  }//IF SERIAL END

  //*************************************************************
  readAnalog();
  delay (20);  //20ms debounce timer. Need this to sort messages on screen.

  sendCount();
  backlight();
}


void sendCount() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > 1000) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    //DO
    //AUTO TEST STRENG UT
    if (sendCounterState == true) {
      Serial.print(F("Count Test nr."));
      Serial.println(testCounter);
      testCounter++;
    }

  }
}


//READ BUTTONS
void readAnalog() {
  adc_key_in = analogRead(0);    // read the value from the sensor
  key = get_key(adc_key_in);  // convert into key press.

 if (key != oldkey) {  // new keypress is detected
  oldkey=key;
  if (key >= 0)  {   // keypress is detected
    // ONE SHOT EVENTS ON KLICK DOWN
    lastLight = millis(); 
    backlight();
    doKeyDown();
    delay(100);  // 200ms button debounce time
  }
}  

//  // BUTTON HOLD, NOT IN USE..
//  else {
//    //RESET HOLD VED KNAPP AV
//    unsigned long currentMillis = millis();
//    lastHold = currentMillis; 
//    holding = false; 
//  }
}


void doKeyDown() {

  //UP KEY (1)
  if (key == 1) { 
    menuDown();
  }

  //DOWN KEY (2)
  if (key == 2) { 
    menuUp();
  }

  //LEFT
  //   if (key == 3){ 

  //        }

  //RIGHT
  if (key == 0) {
    menuItemRightClick();
  }

}


void GOTOlcdLine1() {
  lcd.setCursor(0, 0);
  //Serial.println(F("rad 1"));
}

void GOTOlcdLine2() {
  lcd.setCursor(0, 1);
  // Serial.println(F("rad 2"));
}

void intermec() {
  Serial.println(F("For Full description Visit https://www.innovatum.co.uk/media/1072/smartrak-data-output.pdf"));
  Serial.println(F(""));
  Serial.println(F("INNOVATUM SMARTRAK DATA OUTPUT STRING"));
  Serial.println(F(""));
  Serial.println(F("Characters             Description"));
  Serial.println(F(""));
  Serial.println(F("1 2                    Space characters"));
  Serial.println(F("3 4                    Day of month"));
  Serial.println(F("5                      Space character"));
  Serial.println(F("6 7 8                  Month of year"));
  Serial.println(F("9                      Space character"));
  Serial.println(F("10 11 12 13            Year"));
  Serial.println(F("14 15                  Hour"));
  Serial.println(F("14 15                  Hour"));
  Serial.println(F("16                     :character"));
  Serial.println(F("17 18                  Minutes"));
  Serial.println(F("19                     :character"));
  Serial.println(F("20 21                  Seconds"));
  Serial.println(F("22 23 24               Relative heading"));
  Serial.println(F("25                     Mode"));
  Serial.println(F("26                     Solution"));
  Serial.println(F("27 28 29 30            Signal strength & polarity"));
  Serial.println(F("31 32 33               Video overlay - NOT USED/LEGACY"));
  Serial.println(F("34 35                  Video overlay - NOT USED/LEGACY"));
  Serial.println(F("36                     Source type"));
  Serial.println(F("37 38 39 40 41         Horizontal displacement of target in metres"));
  Serial.println(F("42 43 44 45            Probable maximum error of horizontal displacement in metres"));
  Serial.println(F("46 47 48 49            Vertical displacement of target in metres"));
  Serial.println(F("50 51 52 53            Probable maximum error of vertical displacement in metres"));
  Serial.println(F("54 55 56 57 58         Vertical displacement / DOB"));
  Serial.println(F("59 60 61 62 63 64      Magnetization or current flowing in target"));
  Serial.println(F("65 66 66 67 69 69      Altitude"));
  Serial.println(F("70 71 72               Pitch"));
  Serial.println(F("73 74 75               Roll"));
  Serial.println(F("76 77 78               Absolute heading"));
  Serial.println(F("79 80                  Time split"));
  Serial.println(F("81                     Carriage Return Character"));
  Serial.println(F("82                     Line Feed Character"));
  Serial.println(F(""));
  Serial.println(F(""));
  Serial.println(F("A typical 80 character data string would therefore be as follows"));
  Serial.println(F(""));
  Serial.println(F("10 May 202215:33:27 2 13-4.4-6 250-0.150.051.980.05 1.01 -5146 0.31 1 -5 47 00"));
  Serial.println(F("10 May 202215:33:28 2 13-4.4-6 250-0.150.051.980.05 1.01 -5146 0.31 1 -5 47 00"));
  Serial.println(F("10 May 202215:33:29 2 13-4.4-6 250-0.150.051.980.05 1.01 -5146 0.31 1 -5 47 00"));
}



// Convert ADC value to key number
int get_key(unsigned int input) {
  int k;
  for (k = 0; k < NUM_KEYS; k++)
  {
    if (input < adc_key_val[k])
    {
      return k;
    }
  }
  if (k >= NUM_KEYS)k = -1;  // No valid key pressed
  return k;
}

// not sure what this is 10years later..
void backlight() {
  // // set the pin to input mode to turn the backlight on and to output and low to turn the LED off.
  // // http://arduino.cc/forum/index.php?topic=96747.0
  //    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
  unsigned long currentMillis = millis();
  if ((currentMillis - lastLight) >= 60000 ) { 
    //OFF
    pinMode(lcdBacklightPin, OUTPUT);
    digitalWrite(lcdBacklightPin, LOW);
  }
  else {
    //ON
    pinMode(lcdBacklightPin, INPUT);
  }
}




//playback EEPROM
void playback() {
  byte data = 0;
  //int address = 0;
  //lcd.setCursor(0, 0);

  GOTOlcdLine2();//plassering
  for (int address = 0; address <= 1023; address++) { 
    data = EEPROM.read(address);// read a byte from the current address of the EEPROM
    if (data == 255)break; 
    //  Serial.print(i);
    //  Serial.print("\t"); 
    //  Serial.print(data, DEC);
    //  Serial.print("--");
    Serial.write(data); 
    //  lcd.write(data); 
    //  Serial.print(data); 
    //  lcd.write(data);
  }
  // Serial.println();
  //lcd.write("                 ");
  //GOTOlcdLine1();
  lcd.print("Sendt       ");
  //delay(100);

}
