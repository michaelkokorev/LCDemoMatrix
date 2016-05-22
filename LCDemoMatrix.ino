//We always have to include the library
#include "fonts.h"
#include "LedControlX.h"

#include <Wire.h>  
#include "RTClib.h"

#define BUFFER_SIZE 100
#define VERSION     "3.0"

RTC_DS1307 rtc;
/*
 Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
 pin 8 is connected to the DataIn 
 pin 10 is connected to the CLK 
 pin 9 is connected to LOAD 
 We have only a single MAX72XX.
 */
  int NumDevices = 4;
  LedControlX lc = LedControlX(8,10,9, NumDevices);

  unsigned long previousMillis = 0;
  bool bInvert = true;
  char strTime[4];
  char strTime_old[4]= {0, 0, 0, 0};
  int iPosX[4] = {24, 18, 7, 1};
  
// Buffer for incoming data
  char serial_buffer[BUFFER_SIZE];
  int buffer_position;

void setup() {
  while (!Serial) ; // wait until Arduino Serial Monitor opens

  Serial.begin(57600);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
   for(int dev = 0; dev < lc.getDeviceCount(); dev ++) {   
    lc.shutdown(dev, false);
    /* Set the brightness to a medium values */
    lc.setIntensity(dev, 1);
    /* and clear the display */
    lc.clearDisplay(dev);
   }
}

void loop()
{
  // Wait for incoming data on serial port
  if (Serial.available() > 0) {
    // Read the incoming character
    char incoming_char = Serial.read();
    
    // End of line?
    if(incoming_char == '\n') {
      // Parse the command
      ParseIncomingComand();
    }
    // Carriage return, do nothing
    else if(incoming_char == '\r');
    // Normal character
    else {
      // Buffer full, we need to reset it
      if(buffer_position == BUFFER_SIZE - 1) buffer_position = 0;

      // Store the character in the buffer and move the index
      serial_buffer[buffer_position] = incoming_char;
      buffer_position++;      
    }
  }
  
  digitalClockDisplay();
}

void digitalClockDisplay(){
  if(millis() - previousMillis >= 1000) {
    previousMillis = millis();
    bInvert = !bInvert;

    lc.buildChar(bInvert ? ':' : ' ', 13, 0, 1, digit6x8future);

    DateTime now = rtc.now();
    
    strTime[0] = char(now.minute() % 10 + 48);
    
    if(strTime[0] != strTime_old[0]){      
      strTime[1] = char(now.minute() / 10 + 48);
      strTime[2] = char(now.hour() % 10 + 48);
      strTime[3] = char(now.hour() / 10 + 48);
   
      for(int posY = 7; posY >= 0; posY --) {
        for(int i = 0; i < 4; i++) {
          if(strTime[i] != strTime_old[i]){
            lc.buildChar(strTime[i], iPosX[i], posY, 1, digit6x8future);
            lc.buildChar(strTime_old[i], iPosX[i], posY - 8, 1, digit6x8future);
          }
        }
                
        lc.Update();
        delay(50);
      }

      for(int i = 0; i < 4; i++) strTime_old[i] = strTime[i];
    }
  
    lc.Update();
  }
}

void ParseIncomingComand(){
String strRet = "Error: ";

    switch(serial_buffer[0]){
        case '#':                                                // '##' - Test conektion
          if(serial_buffer[1] == '#') strRet = "!!";
          break;
        case '?':{                                               // '?' Get information
            switch(serial_buffer[1]){
              case 'V':                                          // '?V' Get Number Version
                strRet = VERSION;
                break;
              case 'T':{                                         // '?T' Get RTC Time
                  DateTime now = rtc.now();
                  char chStr[20];
                  sprintf(chStr, "%02d.%02d.%d %02d:%02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());
                  strRet = chStr;
                }
                break;
            }
          }
          break;
        case '!':{                                             // '! ' Set data
           switch(serial_buffer[1]){
             case 'T':{                                       // '!T' Set Current Time
                String time_string = String(serial_buffer);
                int day = time_string.substring(2, 4).toInt();
                int month = time_string.substring(4, 6).toInt();        
                int year = time_string.substring(6, 10).toInt();
                int hour = time_string.substring(10, 12).toInt();
                int minute = time_string.substring(12, 14).toInt();
                int second = time_string.substring(14, 16).toInt();
                DateTime set_time = DateTime(year, month, day, hour, minute, second);
                rtc.adjust(set_time);
                strRet = "OK";
              }
              break;
             
           }
           }
          break;
    }      
    
    Serial.println(strRet);
      
    resetBuffer();
}

void resetBuffer(){
  // Reset the buffer
  for(int i = 0; i < BUFFER_SIZE; i++){
    serial_buffer[i] = 0;
  }
  
  buffer_position = 0;
}


