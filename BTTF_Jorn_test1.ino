
//****************************************
//* Groningen is Cyberspace              *
//* (c) 2018 PAX|Media - Jorn de Vreede  *
//*                                      *
//* Input voor deze ino gebaseerd op:    *
//* Credits: John Guarnero 11/04/2015    *
//****************************************

    // include libraries:
    #include <Wire.h>                  // I2C communications
    #include <RTClib.h>                 // RealTimeCLock
    #include <EEPROM.h>
    #include "Adafruit_LEDBackpack.h"
    #include "Adafruit_GFX.h"   
    #include <Button.h>                // Push the button deedee :)
    #include <avr/sleep.h>                  
    
    volatile boolean buttonPressed;    // set when button pressed                       
 
    #define DS1307_ADDRESS 0x68 //default address of the 1307 RTC module
 
    // button defines
    Button bMenu = Button(2,PULLUP);
    Button bUp = Button(9,PULLUP);
    Button bDown = Button(10,PULLUP);
    float pressLength_milliSeconds = 0;
      
    // display shizzle
    Adafruit_7segment matrix = Adafruit_7segment();     
    // Remember if the colon was drawn on the display so it can be blinked
    // on and off every second.
    bool blinkColon = false;
    uint8_t brightness = 3;        
    bool intro = true;
   
    //clock
    RTC_DS1307 RTC;     // Establish clock object
    DateTime Clock;     // Holds current clock time DateTime future (now + TimeSpan(7,12,30,6));
    int DST;            // DST yes/no
    byte one;           //variable used to point to time setting for minutes in DS1307
    byte two;           //variable used to point to time setting for minutes in DS1307 
    byte hourval_set;   // holds the hour set time
    byte minuteval_set; // holds the minute set time
    int x = 0;          
  
    // Variables for updating time values in menu
    int hourupg;
    int minupg;
    int yearupg;
    int monthupg;
    int dayupg;
    
    // what menu is active?
    int menu = 0;

// interrupt service routine in sleep mode
void wake(){
      sleep_disable ();         // first thing after waking from sleep:
}  // end of wake
    
    // interrupt service routine when awake and button pressed
void buttonDown(){
      buttonPressed = true;
}  // end of buttonDown
    
void sleepNow(){
      set_sleep_mode (SLEEP_MODE_PWR_DOWN);   
      sleep_enable ();          // enables the sleep bit in the mcucr register
      attachInterrupt (digitalPinToInterrupt (2), wake, LOW); 
      sleep_mode ();            // here the device is actually put to sleep!!
      detachInterrupt (digitalPinToInterrupt (2));     // stop LOW interrupt
}  // end of sleepNow

void setup() {
      // DST setting in EEPROM
      DST = EEPROM.get(0, DST);
      if(DST != 0 && DST != 1){     
        DST = 1;  
        EEPROM.put(0, DST);
      }       
      // init clock
      RTC.begin();
  
      if (! RTC.isrunning()) {
         // Set the date and time at compile time
        RTC.adjust(DateTime(__DATE__, __TIME__));
      }
      one = 0x01;     
      two = 0x02;
} // end of setup

  
void loop(){
    // DST fix
   DateTime now = RTC.now();   
   if (now.dayOfTheWeek() == 0 && now.month() == 3 && now.day() >= 8 && now.day() <= 16 && now.hour() == 2 && now.minute() == 0 && now.second() == 0 && DST == 0){       
      RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour()+1, now.minute(), now.second()));       
      DST = 1;       
      EEPROM.put(0, DST);     
    }     
    else if(now.dayOfTheWeek() == 0 && now.month() == 11 && now.day() >= 1 && now.day() <= 8 && now.hour() == 2 && now.minute() == 0 && now.second() == 0 && DST == 1){       
      RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour()-1, now.minute(), now.second()));       
      DST = 0;       
      EEPROM.put(0, DST);
    }

    Clock = RTC.now();                 // get the RTC time
  
   // first time since wake? Show intro!
   if(intro){
    showIntro(); 
   } 
   
   // check if you press the SET button and increase the menu index
   if(bMenu.isPressed() || buttonPressed){

       //Record *roughly* the tenths of seconds the button in being held down
        while (bMenu.isPressed()){ 
              delay(50);  //if you want more resolution, lower this number 
              pressLength_milliSeconds = pressLength_milliSeconds + 50;   
        }//close while
        
        if (pressLength_milliSeconds >= 2000){
              clearAll();  // Clear displays
              intro = true;  // reset intro for wakeup
              sleepNow (); // go to sleep
        } 
        else if(pressLength_milliSeconds >= 50){
              menu = menu + 1;   
         }//close if options
   }
   mainRoutine();
   pressLength_milliSeconds = 0;
   EIFR = 1;      // cancel any existing falling interrupt (interrupt 0)
   attachInterrupt (digitalPinToInterrupt(2), buttonDown, FALLING);   // ready for button press
   buttonPressed = false;
} // end loop



// ************************* All functions *********************

// main routine that determines what we should do
void mainRoutine(){
  // in which subroutine should we go?
  if (menu==0){
     
     //refresh screen every ten seconds 
     if (x > 10){
      x = 0;
     }
     if(x == 0){
       //rood scherm 
       DisplayDateTime(0, RTC.now());
       //groen scherm
       DisplayDateTime(1, (RTC.now() + TimeSpan (1461, 0, 0, 0)) ); // nothing going on, show time with 4 years up (1461 days that is)
     }
     
     blinkRoutine(); //this takes a second
     x = x +1;
    }   
  
  if (menu==1){
    DisplaySetHour();
    }
    
  if (menu==2){
     DisplaySetMinute();
    }
    
  if (menu==3){
    DisplaySetYear();
    }
    
  if (menu==4){
    DisplaySetMonth();
    }
    
  if (menu==5){
    DisplaySetDay();
    }
  
  if (menu==6){
    StoreAgg(); 
    delay(500);
    menu=0;
    }
  delay(100); 
}

// small routine to blink the dots between hours/min 
void blinkRoutine(){
      // Green time blink on
      matrix.begin(0x72);
        matrix.drawColon(blinkColon);
        matrix.setBrightness(brightness);
        matrix.writeDisplay();

      blinkColon = !blinkColon;

      // Red time blink inverted, and flips every loop :)
     matrix.begin(0x75);
        matrix.drawColon(blinkColon);
        matrix.setBrightness(brightness);
        matrix.writeDisplay();
      delay(990);
}


//********************** DISPLAY DATE / TIME  *********************************
 
void DisplayDateTime(int Screen, DateTime Clock){
  
      int yearval, yearvalGrunn, monthval, dayval, hourval, minuteval, secondval; // holds the time
      int waarde;
      
      yearval = Clock.year();            // Clock Year
      //yearvalGrunn = yearval + 4;        // Grunn years 
      monthval = Clock.month();          // Clock Month
      dayval = Clock.day();              // Clock Day

      hourval = Clock.hour();           // Clock Hour: 2 hours later
      minuteval = Clock.minute();       // Clock Minute: 30min later
      secondval = Clock.second();        // Clock Second
      
      // upgradevariabelen laden met huidige tijd
      hourupg = hourval;
      minupg = minuteval;
      yearupg = yearval;
      monthupg = monthval;
      dayupg = dayval;
                   
  if(Screen==0){
  // Display 1Rood: maand en dag
  matrix.begin(0x70);
      matrix.writeDigitNum(3, monthval / 10);
      matrix.writeDigitNum(4, monthval % 10, true);
      matrix.writeDigitNum(0, dayval / 10);
      matrix.writeDigitNum(1, dayval % 10, true);
      matrix.writeDigitRaw(2, 0x1);
      matrix.setBrightness(brightness);
      matrix.writeDisplay();
  
  // Display 2Rood: jaar randstad
  matrix.begin(0x71);      
      matrix.writeDigitNum(0, (yearval / 1000) % 10);
      matrix.writeDigitNum(1, (yearval /  100) % 10);
      matrix.writeDigitNum(3, (yearval /   10) % 10);
      matrix.writeDigitNum(4,  yearval % 10);
      matrix.writeDigitRaw(2, 0x1);
      matrix.setBrightness(brightness);
      matrix.writeDisplay();
   
  // Display 3Rood: tijd
  matrix.begin(0x72);
     matrix.writeDigitNum(0, hourval / 10);
     matrix.writeDigitNum(1, hourval % 10);
     matrix.writeDigitNum(3, minuteval / 10);
     matrix.writeDigitNum(4, minuteval % 10);
     matrix.setBrightness(brightness);
     matrix.writeDisplay();
  }

  if(Screen==1){  
  // Display 1Groen: maand en dag
  matrix.begin(0x73);
      matrix.writeDigitNum(3, monthval / 10);
      matrix.writeDigitNum(4, monthval % 10, true);
      matrix.writeDigitNum(0, dayval / 10);
      matrix.writeDigitNum(1, dayval % 10, true);
      matrix.writeDigitRaw(2, 0x1);
      matrix.setBrightness(brightness);
      matrix.writeDisplay();
      
  // Display 2Groen: Jaartal in Groningen
  matrix.begin(0x74);
      matrix.writeDigitNum(0, (yearval / 1000) % 10);
      matrix.writeDigitNum(1, (yearval /  100) % 10);
      matrix.writeDigitNum(3, (yearval /   10) % 10);
      matrix.writeDigitNum(4,  yearval % 10);
      matrix.writeDigitRaw(2, 0x1);
      matrix.setBrightness(brightness);
      matrix.writeDisplay();
  
  // Display 3Groen: Uren en minuten
  matrix.begin(0x75);
     matrix.writeDigitNum(0, hourval / 10);
     matrix.writeDigitNum(1, hourval % 10);
     matrix.writeDigitNum(3, minuteval / 10);
     matrix.writeDigitNum(4, minuteval % 10);
     //matrix.writeDigitRaw(2, 0x2); //center colon
     matrix.setBrightness(brightness);
     matrix.writeDisplay();
     delay(10);
  } // end if Screen
     
} // end DisplayDateTime


void DisplaySetHour(){ 
      // time setting
      DateTime now = RTC.now();
      if(bUp.isPressed()){
        if(hourupg==23)
        {
          hourupg=0;
        }
        else{
          hourupg=hourupg+1;
        }
      }
      if(bDown.isPressed()){
        if(hourupg==0){
          hourupg=23;
        }
        else{
          hourupg=hourupg-1;
        }
      }
       // Display 1,2 uit, 3 alleen uur tonen
      matrix.begin(0x70);
      clearDisplay();
      matrix.begin(0x71);
      clearDisplay();
      
      matrix.begin(0x72);
         matrix.writeDigitNum(0, hourupg / 10);
         matrix.writeDigitNum(1, hourupg % 10);
         matrix.writeDigitRaw(3,0); 
         matrix.writeDigitRaw(4,0);
         matrix.setBrightness(brightness);
         matrix.writeDisplay();
      delay(20);
}

void DisplaySetMinute(){
      // Setting the minutes
      if(bUp.isPressed()){
        if (minupg==59){
          minupg=0;
        }
        else{
          minupg=minupg+1;
        }
      }
      if(bDown.isPressed()){
        if (minupg==0){
          minupg=59;
        }
        else{
          minupg=minupg-1;
        }
      }
       // Display 1,2 uit, 3 alleen minuten tonen
      matrix.begin(0x70);
      clearDisplay();
      matrix.begin(0x71);
      clearDisplay();
      
      matrix.begin(0x72);
         matrix.writeDigitRaw(0,0);
         matrix.writeDigitRaw(1,0);
         matrix.writeDigitNum(3, minupg / 10);
         matrix.writeDigitNum(4, minupg % 10);
         matrix.setBrightness(brightness);
         matrix.writeDisplay();
      delay(20);
}
  
void DisplaySetYear(){
    // setting the year
      if(bUp.isPressed()){    
        yearupg=yearupg+1;
      }
      if(bDown.isPressed()){
        yearupg=yearupg-1;
      }
        // Display 2Rood: jaar randstad
        matrix.begin(0x70);
          clearDisplay();
        matrix.begin(0x72);
          clearDisplay(); 
    
      
        matrix.begin(0x71);      
          matrix.writeDigitNum(0, (yearupg / 1000) % 10);
          matrix.writeDigitNum(1, (yearupg /  100) % 10);
          matrix.writeDigitNum(3, (yearupg /   10) % 10);
          matrix.writeDigitNum(4,  yearupg % 10);
          matrix.setBrightness(brightness);
          matrix.writeDisplay();
        delay(20);
}

void DisplaySetMonth(){
    // Setting the month
      if(bUp.isPressed()){
        if (monthupg==12){
          monthupg=1;
        }
        else{
          monthupg=monthupg+1;
        }
      }
      if(bDown.isPressed()){
        if (monthupg==1){
          monthupg=12;
        }
        else{
          monthupg=monthupg-1;
        }
      }
       matrix.begin(0x71);
         clearDisplay();
       matrix.begin(0x72);
         clearDisplay();
      
     matrix.begin(0x70);
          matrix.writeDigitNum(3, monthupg / 10);
          matrix.writeDigitNum(4, monthupg % 10);
          matrix.writeDigitRaw(0,0);
          matrix.writeDigitRaw(1,0);
          matrix.writeDigitRaw(2, 0x1);
          matrix.setBrightness(brightness);
          matrix.writeDisplay();
      delay(20);
}

void DisplaySetDay(){
    // Setting the day
      if(bUp.isPressed()){
        if (dayupg==31){
          dayupg=1;
        }
        else{
          dayupg=dayupg+1;
        }
      }
      if(bDown.isPressed()){
        if (dayupg==1){
          dayupg=31;
        }
        else{
          dayupg=dayupg-1;
        }
      }
      matrix.begin(0x71);
      clearDisplay();
      matrix.begin(0x72);
      clearDisplay();
      
       matrix.begin(0x70);
          matrix.writeDigitRaw(3,0);
          matrix.writeDigitRaw(4,0);
          matrix.writeDigitNum(0, dayupg / 10);
          matrix.writeDigitNum(1, dayupg % 10);
          matrix.setBrightness(brightness);
          matrix.writeDisplay();
      delay(20);
}

void StoreAgg(){
        // Saving the new date and time, blink 3 times  
        for(int i = 0; i < 6; i++){
        if ( (i % 2) == 0){
          matrix.begin(0x70);
              matrix.writeDigitNum(3, monthupg / 10);
              matrix.writeDigitNum(4, monthupg % 10, true);
              matrix.writeDigitNum(0, dayupg / 10);
              matrix.writeDigitNum(1, dayupg % 10, true);
              matrix.writeDigitRaw(2, 0x1);
              matrix.setBrightness(brightness);
              matrix.writeDisplay();
          matrix.begin(0x71);      
              matrix.writeDigitNum(0, (yearupg / 1000) % 10);
              matrix.writeDigitNum(1, (yearupg /  100) % 10);
              matrix.writeDigitNum(3, (yearupg /   10) % 10);
              matrix.writeDigitNum(4,  yearupg % 10);
              matrix.writeDigitRaw(2, 0x1);
              matrix.setBrightness(brightness);
              matrix.writeDisplay();
           matrix.begin(0x72);
             matrix.writeDigitNum(0, hourupg / 10);
             matrix.writeDigitNum(1, hourupg % 10);
             matrix.writeDigitNum(3, minupg / 10);
             matrix.writeDigitNum(4, minupg % 10);
             matrix.setBrightness(brightness);
             matrix.writeDisplay();
             delay(200);
        } // end if even
            if ((i % 2) == 1){
               matrix.begin(0x70);
                clearDisplay();
               matrix.begin(0x71);
                clearDisplay();   
               matrix.begin(0x72);
               clearDisplay(); 
               delay(200);   
              } // end if odd
        } // end for
      RTC.adjust(DateTime(yearupg,monthupg,dayupg,hourupg,minupg,0));
}

void showIntro(){
      groningen();
      cyberspace();
      delay(5000);
      clearAll();
      DisplayDateTime(0, RTC.now());
      for(int y = 0; y < 146; y++){
        //groen scherm
        DisplayDateTime(1, (RTC.now() + TimeSpan ((y*10), 0, 0, 0)) ); 
        delay(20);  
      }     
     intro = false;
}

void groningen(){
  matrix.begin(0x73);
      clearDisplay();
      matrix.writeDigitRaw(0,B01101111); // g
      matrix.writeDigitRaw(1,B00110001); // r
      matrix.drawColon(false);
      matrix.writeDigitRaw(3,B00111111); // o
      matrix.writeDigitRaw(4,B00110111); // n
      matrix.setBrightness(brightness);
      matrix.writeDisplay();
  matrix.begin(0x74);
  clearDisplay();
      matrix.writeDigitRaw(0,B00110000); // i
      matrix.writeDigitRaw(1,B00110111); // n
      matrix.drawColon(false);
      matrix.writeDigitRaw(3,B01101111); // g
      matrix.writeDigitRaw(4,B01111001); // e
      matrix.setBrightness(brightness);
      matrix.writeDisplay();
  matrix.begin(0x75);
  clearDisplay();
      matrix.writeDigitRaw(0,B00110111); // n
      matrix.writeDigitRaw(1,B00000000); // 
      matrix.drawColon(false);
      matrix.writeDigitRaw(3,B00000110); // i
      matrix.writeDigitRaw(4,B01101101); // S
      matrix.setBrightness(brightness);
      matrix.writeDisplay();      
  delay(20);
}

void cyberspace(){
  matrix.begin(0x70);
  clearDisplay();
      matrix.writeDigitRaw(0,B00000000); // 
      matrix.writeDigitRaw(1,B00111001); // C
      matrix.drawColon(false);
      matrix.writeDigitRaw(3,B01101110); // y
      matrix.writeDigitRaw(4,B01111111); // B
      matrix.setBrightness(brightness);
     matrix.writeDisplay();
  matrix.begin(0x71);
      matrix.writeDigitRaw(0,B01111001); // e
      matrix.writeDigitRaw(1,B00110001); // r
      matrix.drawColon(false);
      matrix.writeDigitRaw(3,B01101101); // S
      matrix.writeDigitRaw(4,B01110011); // p
     matrix.setBrightness(brightness);
     matrix.writeDisplay();
  matrix.begin(0x72);
      matrix.writeDigitRaw(0,B01110111); // A
      matrix.writeDigitRaw(1,B00111001); // c
      matrix.drawColon(false);
      matrix.writeDigitRaw(3,B01111001); // e
      matrix.writeDigitRaw(4,B00000000); // 
      matrix.setBrightness(brightness);
      matrix.writeDisplay();      
  delay(20);
}    

void clearDisplay(){
     matrix.setBrightness(brightness);
     matrix.drawColon(false);
     matrix.writeDigitRaw(0,0);
     matrix.writeDigitRaw(1,0);
     matrix.writeDigitRaw(3,0);
     matrix.writeDigitRaw(4,0);
     matrix.writeDisplay();
     delay(10);
}

void clearAll(){
      matrix.begin(0x70);
      clearDisplay();
      matrix.begin(0x71);
      clearDisplay();   
      matrix.begin(0x72);
      clearDisplay();
      matrix.begin(0x73);
      clearDisplay();
      matrix.begin(0x74);
      clearDisplay();   
      matrix.begin(0x75);
      clearDisplay();  
}
            
byte decToBcd(byte val) { // Convert normal decimal numbers to binary coded decimal - used for DS1307
   return ( (val / 10 * 16) + (val % 10) );
}

byte bcdToDec(byte val)  { // Convert binary coded decimal to normal decimal numbers - used for DS1307
   return ( (val / 16 * 10) + (val % 16) );
}

