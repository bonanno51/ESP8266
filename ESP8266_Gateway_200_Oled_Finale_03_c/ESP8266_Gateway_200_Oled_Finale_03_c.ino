/**************************************************************************
    G. Bonanno
    Souliss - Gateway for Expressif ESP8266
    
    IDE 1.6.4
    ESP8266 version 1.6.4-673-g8cd3696
    Souliss version 7.0.5
    
    Per ESP-12 
    
    Pushetta
    DHT-11
    OLED

    pin 0
    pin 2    DHT-11
    pin 5    SDA
    pin 4    SCL
    pin 12   led yellow
    pin 13   led red
    pin 14   
    pin 16
    
    Indirizzi IP in uso:
    192.168.1.200  Gateway WIFI
    192.168.1.201  Amplificatore
    192.168.1.202  Lampada di sale
    192.168.1.203  Camera da letto
    192.168.1.204  Motion Sensor
    192.168.1.205
    
    Indirizzi UART in uso:
    CE01           Gateway UART
    CE02           Soggiorno
    CE03

    a) Aggiunto WiFi.mode(WIFI_STA); in Setup

***************************************************************************/

// Configure the framework
#include "bconf/MCU_ESP8266.h"              // Load the code directly on the ESP8266
//#include "conf/Gateway.h" // The main node is the Gateway, we have just one node
#include "conf/Gateway_wPersistence.h"
#include "conf/usart.h"
 
// Define the WiFi name and password
#define WIFICONF_INSKETCH
#define WiFi_SSID               ""
#define WiFi_Password           ""

#include <Wire.h>
#include <stdio.h>
#include <OLED_SSD1306.h>
#include <ESP8266WiFi.h>
#include "Souliss.h"
#include <ntp.h>
#include <WiFiUdp.h>
#include <Time.h>
time_t getNTPtime(void);
NTP NTPclient;
#define CET +1

int c = 0;  // ciclo di allarme

// #define OLED_ADDRESS  0x3c  //OLED I2C bus address
#define SCROLL_WORKS 1
#include "font2.h"
OLED_SSD1306 oled;

extern "C" {
#include "user_interface.h"
}

// ---------  Pushetta  - Registrazione su www.pushetta.com
char APIKEY[] = ""; // Put here your API key
char CHANNEL[] = "";                          // and here your channel name
WiFiClient client;
char serverName[] = "api.pushetta.com";
// ---------  Pushetta fine

#define ANTITHEFT      0
#define TEMPERATURE    1
#define HUMIDITY       3
//
// pins definitions
#define DHTPIN         2      // DHT-11 pin
#define OLED_SDA       5      // Oled display
#define OLED_SCL       4      // Oled display
#define LedY           12     // pin Led Yellow
#define LedR           13     // pin Led Red
//
uint8_t ip_address[4]  = {192, 168, 1, 200};
uint8_t subnet_mask[4] = {255, 255, 255, 0};
uint8_t ip_gateway[4]  = {192, 168, 1, 1};
//
#define Gateway_address 200    // Gateway ESP8266 
#define Peer_address1   201    // Amplificatore
#define Peer_address2   202    // Lampada di sale ESP8266
#define Peer_address3   203    // Camera da letto ESP8266
#define Peer_address4   204    // Motion Sensor   ESP8266
#define Peer_address5   205    // free
#define myvNet_esp8266  ip_address[3]
#define myvNet_subnet   0xFF00
#define myvNet_supern   Gateway_address
#define Gateway_UART    0xCE01  // Gateway UART
#define Peer_UART       0xCE02  // Wall Switch Soggiorno
#define Peer_UART_CE03  0xCE03  // free

// Include and Configure DHT11 SENSOR
#include "DHT.h"
#define DHTTYPE DHT11   // DHT 11 
DHT dht(DHTPIN, DHTTYPE, 15);

//Function for sending the request to Pushetta
void sendToPushetta(char channel[], String text) {
  client.stop();
  if (client.connect(serverName, 80))
  {
    client.print("POST /api/pushes/");
    client.print(channel);
    client.println("/ HTTP/1.1");
    client.print("Host: ");
    client.println(serverName);
    client.print("Authorization: Token ");
    client.println(APIKEY);
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(text.length() + 46);
    client.println();
    client.print("{ \"body\" : \"");
    client.print(text);
    client.println("\", \"message_type\" : \"text/plain\" }");
    client.println();
  }
}

void setup()
{
  Wire.begin( OLED_SDA, OLED_SCL );  // Wire.cpp modificato
  oled.Init();
  oled.ClearDisplay();
  oled.SendStrXY( "Souliss ESP8266", 0, 0 );

  Initialize();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  WiFi.mode(WIFI_STA);
  oled.SendStrXY( "WiFi connected", 1, 0 );
  oled.SendStrXY( "SSID=", 2, 0 );
  oled.SendStrXY( WiFi.SSID(), 2, 5 );

  // assign the address manually
  Souliss_SetIPAddress(ip_address, subnet_mask, ip_gateway);
  SetAsGateway(myvNet_esp8266);       // Set this node as gateway for SoulissApp
  SetAddress(Gateway_UART, myvNet_subnet, 0);

  SetAsPeerNode(Peer_address1, 1);    // Amplificatore
  SetAsPeerNode(Peer_address2, 2);    // Lampada di sale
  SetAsPeerNode(Peer_address3, 3);    // Camera da letto
  SetAsPeerNode(Peer_address4, 4);    // Motios Sensor
  SetAsPeerNode(Peer_UART, 5);        // Soggiorno
  SetAsPeerNode(Peer_UART_CE03, 6);   //
  SetAsPeerNode(Peer_address5, 7);    // 

  Set_T41(ANTITHEFT);

  pinMode(LedY, OUTPUT);
  pinMode(LedR, OUTPUT);

  dht.begin();
  Set_Temperature(TEMPERATURE);
  Set_Humidity(HUMIDITY);

  NTPclient.begin("time-a.timefreq.bldrdoc.gov", CET);
  setSyncInterval(SECS_PER_HOUR);
  setSyncProvider(getNTPtime);

  //print the local IP address
  char result[16];
  sprintf(result, "%03d.%03d.%03d.%03d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  oled.SendStrXY(result, 3, 0);
  delay(5000);
  oled.ClearDisplay();
  Draw_Souliss();

  oled.SendStrXY("Souliss   ", 0, 6);
  
}

void loop()
{
  // Here we start to play
  EXECUTEFAST() {

    UPDATEFAST();

    FAST_50ms() {
      
    }
    FAST_510ms()    {
      DateDisplay();
      ClockDisplay();
      // Input from anti-theft sensor
      //      LowDigIn(12, Souliss_T4n_Alarm, ANTITHEFT);

      // Execute the anti-theft logic
      Logic_T41(ANTITHEFT);
      //      if (data_changed) sendToPushetta(CHANNEL, "Allarme infrarosso soggiorno "); // send to Pushetta
      // Set the Led Yellow if the anti-theft is activated
      nDigOut(LedY, Souliss_T4n_Antitheft, ANTITHEFT);
      if (digitalRead(LedY) == 1 && digitalRead(LedR) == 0) PreAlarmON();
      if (digitalRead(LedY) == 0) AlarmOFF();
      // Set the Led Red if the alarm is raised
      DigOut(LedR, Souliss_T4n_InAlarm, ANTITHEFT);
      if (digitalRead(LedR) == 1 && digitalRead(LedY) == 1 )
      {
        AlarmON();
        if (c == 1) {
          sendToPushetta(CHANNEL, "Allarme infrarosso soggiorno "); // send to Pushetta
          c = 0;
        }
      }
    }

    FAST_910ms()    {

    }

    FAST_2110ms()
    {
      Logic_Temperature(TEMPERATURE);
      Logic_Humidity(HUMIDITY);
    }

    // Here we handle here the communication with Android
    FAST_GatewayComms();
  }

  EXECUTESLOW() {
    UPDATESLOW();

    SLOW_10s() {
      // Read temperature and humidity from DHT every 10 seconds
      float h = dht.readHumidity();
      // Read temperature as Celsius
      float t = dht.readTemperature();
      // Check if any reads failed and exit early (to try again).
      if (isnan(h) || isnan(t))  {
        oled.SendStrXY("Fail DHT sensor", 7, 0);
        //return;
      }

      // Print temperature on OLED display
      char mytemp[16];
      int d1 = t;
      float f1 = t - d1;
      int d2 = (int)(f1 * 10);
      sprintf(mytemp, "%d.%01d *C" , d1 , d2);
      oled.SendStrXY(mytemp, 7, 0);

      // Print umidity on OLED display
      char myumid[16];
      int d3 = h;
      float f2 = h - d3;
      int d4 = (int)(f2 * 10);
      sprintf(myumid, "%d.%01d %" , d3 , d4);
      oled.SendStrXY(myumid, 7, 10);

      Souliss_ImportAnalog(memory_map, TEMPERATURE, &t);
      Souliss_ImportAnalog(memory_map, HUMIDITY, &h);
    }
  }
}

time_t getNTPtime(void)
{
  return NTPclient.getNtpTime();
}

void AlarmON()
{
  char mytime[16];
  oled.SendStrXY("Alarm ACTIVE    ", 5, 0);
}

void AlarmOFF()
{
  char mytime[16];
  oled.SendStrXY("                ", 5, 0);
}

void PreAlarmON()
{
  c = 1;
  char mytime[16];
  oled.SendStrXY("Antitheft ARMED ", 5, 0);
}

void ClockDisplay()
{
  char mytime[16];
  sprintf(mytime, "%02d:%02d", hour(), minute());
  oled.SendStrXY(mytime, 2, 7);
}

void DateDisplay()
{
  char mydate[16];
  sprintf(mydate, "%02d/%02d/%4d", day(), month(), year());
  //  sprintf(mydate, "%02d/%02d/%4d %02d:%02d", day(), month(), year(), hour(), minute());
  oled.SendStrXY(mydate, 1, 5);
}

void Draw_Souliss(void) {
  unsigned char i, j;

  //  oled.DisplayOFF();
  // oled.ClearDisplay();
  for ( int i = 0; i < 8; i++ ) {
    oled.SetCursorXY( i, 0 );
    for ( int j = 0; j < 32; j++ ) {
      oled.SendChar( pgm_read_byte( souliss + j + i * 32 ) );
    }
  }
  oled.SendStrXY("                ", 4, 0);
  oled.SendStrXY("                ", 5, 0);
  oled.SendStrXY("                ", 6, 0);
  oled.SendStrXY("                ", 7, 0);
  //  oled.DisplayON();

}
