/* ATM90E26 Energy Monitor Demo Application
    The MIT License (MIT)
  Copyright (c) 2016 whatnick and Ryzee
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

// the sensor communicates using SPI, so include the library:
#include <SPI.h>
////////////////////////////////////SD and RTC//////////////////////////////////////////////////////////////////////
#include <SD.h>
#include "RTClib.h"


RTC_PCF8523 rtc;    //Pcf8523 RTC that we are using in Adalogger ( Website I have searched)

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

const int chipSelect = 9;  //Check the ChipSelect and select accordingly.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /*******************
 * WEMOS SPI Pins:
 * SCLK - D5
 * MISO - D6
 * MOSI - D7
 * SS   - D8
 *******************/
 
#include "energyic_SPI.h"

ATM90E26_SPI eic;

void setup() 
{
  /* Initialize the serial port to host */
  Serial.begin(115200);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  /*Initialise the ATM90E26 + SPI port */
  eic.InitEnergyIC();

////////////////////////////////SD CARD and RTC//////////////////////////////////

Serial.print("Initializing SD card...");
// see if the card is present and can be initialized:
if (!SD.begin(chipSelect)) 
{
  Serial.println("Card failed, or not present");
  // don't do anything more:
  return;
}
Serial.println("card initialized.");

if (! rtc.begin()) 
{
  Serial.println("Couldn't find RTC");
  while (1);
}
if (! rtc.initialized()) 
{
   Serial.println("RTC is NOT running!");
   rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}
//////////////////////////////////////////////////////////////////////////////

}

void loop() 
{
  /*Repeatedly fetch some values from the ATM90E26 */
 
  Serial.print("Voltage:");
  Serial.println(eic.GetLineVoltage());
  yield();
  Serial.print("Current:");
  Serial.println(eic.GetLineCurrent());
  yield();
  Serial.print("Real Power:");
  Serial.println(eic.GetActivePower(),HEX);
  yield();
  Serial.print("Power Factor:");
  Serial.println(eic.GetPowerFactor());
  yield();
  Serial.print("Freq:");
  Serial.println(eic.GetFrequency());
  delay(1000);

  float id = ESP.getChipId();
  float Vrms1 = eic.GetLineVoltage();
  float Crms1 = eic.GetLineCurrent();
  float realPower1 = eic.GetActivePower() ;
  float powerFactor1 = eic.GetPowerFactor();
  float freq = eic.GetFrequency();

  String aa = String(id);
  String bb = String(Vrms1);
  String cc = String(Crms1);
  String dd = String(realPower1);
  String ee = String(powerFactor1);
  String ff = String(freq);


  /////////////////////////////////////SD Card and RTC/////////////////////////////
DateTime now = rtc.now();  

int y=now.year();
String yer = String(y);    //converting each values into string
int m=now.month();
String mnth = String(m);
int d=now.day();
String dy = String(d);
int h=now.hour();
String hr = String(h);
int mi = now.minute();
String mint = String(mi);
int s = now.second();
String secn = String(s);
int milli = millis();
int ms = milli % 1000;
String millisec = String(ms);
String date = "Date";
String Time = "Time";

    
Serial.println(yer+"/"+mnth+"/"+dy+"---"+hr+":"+mint+":"+secn+":"+millisec+ "---" + "Id:"+aa+ "Voltage :" +bb+"Current:"+cc+ "Real Power:"+dd+"Power Factor:"+ee+"Freq:"+ff );   // String addiction, seperation using "/"

File dataFile = SD.open("FINAL_Data.csv", FILE_WRITE);

if (dataFile) 
{
dataFile.println(date+","+Time+","+"Id"+","+"Voltage"+","+"Current"+","+"Real Power"+","+"Power Factor"+","+"Frequency");
dataFile.println(yer+"/"+mnth+"/"+dy+","+hr+":"+mint+":"+secn+":"+millisec+ "," +aa+ ","  +bb+ "," +cc+"," +dd+"," +ee+","+ff );   // String addiction, seperation using "/" and "," is to seperate in SD card
 
dataFile.close();

Serial.println(yer+"/"+mnth+"/"+dy+"---"+hr+":"+mint+":"+secn+":"+millisec+ "---" + "Id:"+aa+ "Voltage :" +bb+"Current:"+cc+ "Real Power:"+dd+"Power Factor:"+ee+"Freq:"+ff );   // String addiction, seperation using "/"

}
    // if the file isn't open, show up an error:
else 
{
  Serial.println("error opening FINAL_Data.csv");
}

/////////////////////////////////////////////////////////////////////////////////
  
}
