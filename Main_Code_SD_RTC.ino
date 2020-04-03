#define FS_NO_GLOBALS
#include <FS.h>   
                                      
#include <DNSServer.h>                         
#include <ESP8266WebServer.h>   

//////////////////SD Card and RTC/////////////////////
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"


RTC_PCF8523 rtc;    //Pcf8523 RTC that we are using in Adalogger ( Website I have searched)

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

const int chipSelect = 15;


///////////////////////////////////////////////////////
    
#include <WiFiManager.h>              

bool shouldSaveConfig = false;           

#include <ArduinoJson.h>                   
#include <Arduino.h>                           
#include <Stream.h>                        
    
#include <ESP8266WiFi.h>                                                                                      
#include <ESP8266WiFiMulti.h>

#include <Wire.h>                                        
#include <energyic_UART.h>                        
#if !defined(ARDUINO_ARCH_SAMD)        
#include <SoftwareSerial.h>                       
#else
#endif


struct clientData {
int Vrms1[8];
int Crms1[8];
int id[8];                                                                  
int realPower1[8];
int powerFactor[8];
int freq1[8];
};

//AWS
#include "sha256.h"                        
#include "Utils.h"                            

//WEBSockets
#include <Hash.h>                                                         
#include <WebSocketsClient.h>

//MQTT PAHO
#include <SPI.h>
#include <IPStack.h>
#include <Countdown.h>
#include <MQTTClient.h>

//AWS MQTT Websocket
#include "Client.h"
#include "AWSWebSocketClient.h"
#include "CircularByteBuffer.h"


#if defined(ESP8266)
SoftwareSerial ATMSerial(13, 14);
#endif

#ifdef AVR_ATmega32U4 //32u4 board
SoftwareSerial ATMSerial(11, 13);                                                    
#endif

#if defined(ARDUINO_ARCH_SAMD)
#include "wiring_private.h"                                                             
#define PIN_SerialATM_RX       12ul
#define PIN_SerialATM_TX       11ul                                             
#define PAD_SerialATM_RX       (SERCOM_RX_PAD_3)
#define PAD_SerialATM_TX       (UART_TX_PAD_0)

Uart ATMSerial(&sercom1, PIN_SerialATM_RX, PIN_SerialATM_TX, PAD_SerialATM_RX, PAD_SerialATM_TX);
#endif

ATM90E26_UART eic(&ATMSerial);

void saveConfigCallback () {
Serial.println("Should save config");                            
shouldSaveConfig = true;
}


char auth[36] = "AKIA45IPCKAKD3J7OKID";
char server[50] = "a2geapajmwonm2-ats.iot.us-east-2.amazonaws.com";

//char wifi_ssid[]       = "Devil's Home";
//char wifi_password[]   = "password@ml";
char aws_endpoint[]    = "a2geapajmwonm2-ats.iot.us-east-2.amazonaws.com";
char aws_key[]         = "AKIA45IPCKAKHZSKN7MB";
char aws_secret[]      = "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";  
char aws_region[]      = "us-east-2";
const char* aws_topic  = "$aws/things/minion_new/shadow/update";
int port = 443;

#define DEBUG_PRINT 1      //default debugging process

const int maxMQTTpackageSize = 512;   

const int maxMQTTMessageHandlers = 1;    
ESP8266WiFiMulti WiFiMulti;

AWSWebSocketClient awsWSclient(256,5000);   

IPStack ipstack(awsWSclient);

MQTT::Client<IPStack, Countdown, maxMQTTpackageSize, maxMQTTMessageHandlers> *client = NULL;

long connection = 0;

char* generateClientID () {
char* cID = new char[23]();
for (int i=0; i<22; i+=1)
cID[i]=(char)random(1, 256);
return cID;   }

int arrivedcount = 0;

void messageArrived(MQTT::MessageData& md)
{
MQTT::Message &message = md.message;

if (DEBUG_PRINT) {                                                                     
Serial.print("Message ");
Serial.print(++arrivedcount);
Serial.print(" arrived: qos ");
Serial.print(message.qos);
Serial.print(", retained ");
Serial.print(message.retained);
Serial.print(", dup ");
Serial.print(message.dup);
Serial.print(", packetid ");
Serial.println(message.id);
Serial.print("Payload ");
char* msg = new char[message.payloadlen+1]();
memcpy (msg,message.payload,message.payloadlen);
Serial.println(msg);

Serial.print("Initial string value: ");
Serial.println(msg);


StaticJsonBuffer<300> JSONBuffer;   

JsonObject& parsed = JSONBuffer.parseObject(msg); 
if (!parsed.success()) {   
Serial.println("Parsing failed");
delay(5000);
return;
}

const char* Vrms1 = parsed["state"]["reported"]["Vrms1"];
const char* Crms1 = parsed["state"]["reported"]["Crms1"];


int convVrms=atoi(Vrms1);            
Serial.print("The Voltage is ");
Serial.println(convVrms);

int convCrms=atoi(Crms1);            
Serial.print("The Current is ");
Serial.println(convCrms);
}
}

bool connect () {
if (client == NULL) {
client = new MQTT::Client<IPStack, Countdown, maxMQTTpackageSize, maxMQTTMessageHandlers>(ipstack);
} else {
if (client->isConnected ()) {
client->disconnect ();
}
delete client;
client = new MQTT::Client<IPStack, Countdown, maxMQTTpackageSize, maxMQTTMessageHandlers>(ipstack);
}

delay (1000);
if (DEBUG_PRINT) {
Serial.print (millis ());
Serial.print (" - conn: ");
Serial.print (++connection);
Serial.print (" - (");
Serial.print (ESP.getFreeHeap ());
Serial.println (")");
}

int rc = ipstack.connect(aws_endpoint, port);
if (rc != 1)
{
if (DEBUG_PRINT) {
Serial.println("error connection to the websocket server");
}
return false;
} else {
if (DEBUG_PRINT) {
Serial.println("websocket layer connected");
}
}

if (DEBUG_PRINT) {
Serial.println("MQTT connecting");
}

MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
data.MQTTVersion = 3;
char* clientID = generateClientID ();
data.clientID.cstring = clientID;
rc = client->connect(data);
delete[] clientID;
if (rc != 0)
{
if (DEBUG_PRINT) {
Serial.print("error connection to MQTT server");
Serial.println(rc);
return false;
}
}
if (DEBUG_PRINT) {
Serial.println("MQTT connected");
}
return true;
}
void subscribe () {
int rc = client->subscribe(aws_topic, MQTT::QOS0, messageArrived);
if (rc != 0) {
if (DEBUG_PRINT) {
Serial.print("rc from MQTT subscribe is ");
Serial.println(rc);
}
return;
}
if (DEBUG_PRINT) {
Serial.println("MQTT subscribed");
}
}


void setup() {
Serial.begin (115200);



WiFiManagerParameter custom_ts_token("ts", "AKIA45IPCKAKD3J7OKID", auth, 33);
WiFiManagerParameter custom_server("serv", "a2geapajmwonm2-ats.iot.us-east-2.amazonaws.com", server, 50);

WiFiManager wifiManager;
wifiManager.setDebugOutput(false);

wifiManager.setSaveConfigCallback(saveConfigCallback);

wifiManager.addParameter(&custom_ts_token);
wifiManager.addParameter(&custom_server);

wifiManager.autoConnect("Minion", "password");

Serial.println("connected...yeey :");
Serial.print("Key:");
Serial.println(auth);
Serial.print("Server:");
Serial.println(server);

strcpy(auth, custom_ts_token.getValue());
strcpy(server, custom_server.getValue());

awsWSclient.setAWSRegion(aws_region);
awsWSclient.setAWSDomain(aws_endpoint);
awsWSclient.setAWSKeyID(aws_key);
awsWSclient.setAWSSecretKey(aws_secret);
awsWSclient.setUseSSL(true);






}



void loop() 
{
unsigned short s_status = eic.GetSysStatus();
if(s_status == 0xFFFF)
{
#if defined(ESP8266)
//Read failed reset ESP, this is heavy
ESP.restart();
#endif
}

float id = ESP.getChipId();
float Vrms1 = eic.GetLineVoltage();
float Crms1 = eic.GetLineCurrent();
float realPower1 = eic.GetActivePower();
float powerFactor1 = eic.GetPowerFactor();
float freq1 = eic.GetFrequency();



String aa = String(id);
String bb = String(Vrms1);
String cc = String(Crms1);
String dd = String(realPower1);
String ee = String(powerFactor1);
String ff = String(freq1);

String values = "{\"state\":{\"reported\":{\"Id\":"+aa+",\"Voltage\":"+bb+",\"Current\":"+cc+",\"Active Power\":"+dd+",\"Power Factor\":"+ee+",\"Frequency\":"+ff+"}}}";             



const char *publish_message = values.c_str();

if (awsWSclient.connected ()) {
client->yield();
MQTT::Message message;
char buf[1000];            
strcpy(buf, publish_message);
message.qos = MQTT::QOS0;
message.retained = false;
message.dup = false;
message.payload = (void*)buf;
message.payloadlen = strlen(buf)+1;
int rc = client->publish(aws_topic, message);
} else {
connect ();
}

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
    
Serial.println(yer+"/"+mnth+"/"+dy+"---"+hr+":"+mint+":"+secn+":"+millisec+ "---" + "Id:"+aa+ "Voltage :" +bb+"Current:"+cc+ "Active Power:"+dd+"Power Factor:"+ee+"Frequency:"+ff );   // String addiction, seperation using "/"

//Serial.println(yer+"/"+mnth+"/"+dy+"---"+hr+":"+mint+":"+secn+":"+millisec+ "---" + "Id:"+aa+",\"Voltage\":"+bb+",\"Current\":"+cc+",\"Active Power\":"+dd+",\"Power Factor\":"+ee+",\"Frequency\":"+ff  );   // String addiction, seperation using "/"

    
//    float ACCurrentValue = readACCurrentValue(); //read AC Current Value
//    float voltageVirtualValue = readVoltageValue();
//    String AC_Current = "";
//    String VoltageRMS = "";
//    AC_Current += String(ACCurrentValue);
//    VoltageRMS += String(voltageVirtualValue);
//    AC_Current += String(ACCurrentValue);
//    VoltageRMS += String(voltageVirtualValue);
//    AC_Current += String(ACCurrentValue);
//    VoltageRMS += String(voltageVirtualValue);

File dataFile = SD.open("Final_Data.txt", FILE_WRITE);
if (dataFile) 
{
dataFile.println(yer+"/"+mnth+"/"+dy+"---"+hr+":"+mint+":"+secn + "---" + "Id:"+aa+",\"Voltage\":"+bb+",\"Current\":"+cc+",\"Active Power\":"+dd+",\"Power Factor\":"+ee+",\"Frequency\":"+ff  );   // String addiction, seperation using "/"
dataFile.close();
   
    // print to the serial port too for cross verify. 
   // Serial.println();
    //Serial.println(voltageVirtualValue);
    
}
  
  // if the file isn't open, show up an error:
else 
{
  Serial.println("error opening Final_Data.txt");
}
   // Current_Value += ACCurrentValue;
delay(1000);
  //}
/////////////////////////////////////////////////////////////////////////////////







}
