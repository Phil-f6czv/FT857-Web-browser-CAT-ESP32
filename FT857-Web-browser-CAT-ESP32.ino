
/*
 *
 * Created : 2020.07.09 FT857-Web-browser-CAT-ESP32 V1.00
  Author   : Ph Lonc, F6CZV

FT857D Display

  This sketch implements thanks to the FT-857D library :
  - a display the parameters of a Yaesu FT-857D radio on the on-board TTGO screen,
  - a webserver to display the same parameters on a web client but also to send commands to the FT-857D.
  The on-board display was first implemented on an Arduino Nano and ported on the ESP2 TTGO T-display MCU.
  The web server and the web client are specific to the ESP32.

 On the ESP32 board TTGO T-Display the following data are displayed :
  Inclusion of a color TFT screen on SPI bus to display :
  - frequency and mode,
  - RX/TX status,
  - S Meter value,
  - VFO A / B status,
  - Keyer and Break-in status if mode is CW or CWR,
  - DSP configuration,
  - SPLIT status.
  The VFO status, Keyer, Break-in, DSP configuration and SPLIT status are read from the radio EEPROM.
  The SoftwareSerial library was replaced by the HardwareSerial library available for ESP32. ESP32 has 3 UARTs (0 to 2) but only UART 2 (Serial2) is usable without problem.
  As on development boards pin 16 and 17 dedicated by default to Serial2 are either not shown or are used for other purpose
  it is possible to affect pin 25, 26 and 27 for Rx/tx.
  As it is a graphical TFT screen, screen must be cleared before any write to avoid scories of previous writes.
  The tft.fillRect(x,y,width,height, color) function was used for that.

  A web display was added and some CAT commands can be executed from the web page (toogle VFO, SPLIT, frequency input, mode modification , clarifier on/off).

  As the web client source code is now the most important part, all the related code is now in dedicated files (index.html, style.css).
  With that split of files it is possible to modify the web client without recompling the ESP32 C++ source code : one only need to upload the html and css to the ESP32 flash memory.
  This is done by the "ESP32 Sketch Data Upload" utility which is an add-on of the Arduino IDE.
  All the files including images used by the web client are stored and managed in the flash memory using the SPIFFS library.


  Serial link : attach your radio's CAT interface to the following ESP32 TTGO T-Display pins :

    Radio CAT port GND -> T-Display GND
    Radio CAT port TX  -> T-Display pin 27
    Radio CAT port RX  -> T-Display pin 26

  If you are unable to see someting on the screen try reversing pins 26 and 27.
  It may be necessary to power down the radio and to reset the MCU prior to powering the radio back on.

LIMITATION OF LIABILITY :
 This source code is provided "as-is". It may contain bugs.
Any damages resulting from its use is done under the sole responsibilty of the user/developper and beyond my responsibility.

This work is derived from the FT857 library implemented for Arduino by James Buck, VE3BUX (Web: http://www.ve3bux.com). A huge thanks to him.

I would like to also thank Rui Santos and Sara Santos for their comprehensive tutorials and books on ESP32 at https://randomnerdtutorials.com.



  ------------------


*/
// Include the file management library SPIFFS
#include <SPIFFS.h>


// include the Wi-Fi and webserver libraries
#include <WiFi.h>
#include "ESPAsyncWebServer.h"

// include the librairies needed for the TFT screen management - SPI bus
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>

// include the librairies for the FT-857D CAT management
#include <HardwareSerial.h> // replace the softwareserial library used for Arduino
#include "FT857D-ESP32.h"    // Customized library of the FT857D CAT controls

// TFT declarations for the use on an SPI bus

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

#define TFT_MOSI            19
#define TFT_SCLK            18
#define TFT_CS              5
#define TFT_DC              16
#define TFT_RST             23

#define TFT_BL          4  // Display backlight control pin
#define ADC_EN          14
#define ADC_PIN         34
TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library - TTGO screen of 135 pixels x 240 pixels
int btnDisp = true;

// Variables for the FT-857D CAT and parameters display

FT857D radio; // instanciate the FT857D class i.e. define "radio" so that we may pass CAT commands
bool On_Air = false;
String RTStatus;
bool PTT = false;
bool Split = false;
String SPLStatus;
bool Keyer;
String KYRStatus;
bool Break_In;
String BKStatus;
byte MeterConf;
String VFO1;
String prevVFO = " ";
bool DNF = false;
String DNFStatus;
bool DNR = false;
String DNRStatus;
bool DBF = false;
String DBFStatus;
bool AGC = false;
bool Clar = false;
String ClarStatus;
String blank = "      ";
String radmode = "   ";
String reqmode = "LSB";
String prevmode = "   ";
String Smeter;
String prevSmeter = "     ";
int dly = 500;            // delay for x milliseconds between commands
String Wfrequency;
String reqfreq;
String deltafreq;
unsigned long frequency_int;

// Two possibilities for WiFi network :
// - the ESP32 is connected to a WiFi Access Point,
// - the ESP32 is the Access Point.


// Initialization of the WiFi network credentials - case ESP32 connected to an Access Point
/* const char* ssid     = "XXXXXXXXX";
const char* password = "yyyyyyyyyyyyyyyy"; */

// Initialization of the WiFi network credentials - case ESP32 is an Access Point. On my ESP32 IP V4 address is 192.168.4.1
const char* ssid     = "ESP32essaiAP";
const char* password = "Esp32-accesspoint2-port&&32";

// Instanciation of the aynchronous web server (port number 80)
AsyncWebServer server(80);

// This function supplies the values of the placeholders in the HTML code (%VAR%)with the effective radio parameters values
//
String processor(const String& var){

  if (var == "VFO") {
    return VFO1;}

  if (var == "SMETER") {
      return Smeter;}

  if (var == "RXTX") {
      return RTStatus;}

  if (var == "SPLIT") {
      return SPLStatus;}

  if (var == "MODE") {
      return radmode;}

  if (var == "FREQ") {
      return Wfrequency;}

  if (var == "BK") {
      return BKStatus;}

  if (var == "KYR") {
      return KYRStatus;}

  if (var == "DNF") {
      return DNFStatus;}

  if (var == "DNR") {
      return DNRStatus;}

  if (var == "DBF") {
      return DBFStatus;}

  if (var == "CLAR") {
      return ClarStatus;}
}


void setup() {
    Serial.begin(115200); // serial link to the PC for debugging purposes
    radio.begin(38400); // serial link to the FT-857

    // screen initialization
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(0, 0);
    tft.setTextDatum(MC_DATUM);
    if (TFT_BL > 0) { // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
         pinMode(TFT_BL, OUTPUT); // Set backlight pin to output mode
         digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
    }


  // Connect to Wi-Fi network with SSID and password - case connected to an Access Point
  /*
     Serial.println(ssid);
     WiFi.begin(ssid, password);
     tft.setRotation(1); // rotation F6CZV
     tft.setCursor(0, 0); // F6CZV
     while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            tft.print("."); // à vérfier
          }
     tft.setCursor(0, 0); // F6CZV
     tft.fillScreen(TFT_BLACK);
     tft.setTextSize(2);
     // display the server IP address on the TTGO screen
     tft.println("IP address: "); // F6CZV
     tft.println(WiFi.localIP()); // F6CZV */

// Start the Wi-Fi network with SSID and password - case ESP32 is an Access Point

     WiFi.softAP(ssid, password);
     IPAddress IP = WiFi.softAPIP();
     Serial.print("AP IP address: ");
     Serial.println(IP);
     tft.setRotation(1); // rotation of the screen
     tft.setCursor(0, 0); //
     tft.fillScreen(TFT_BLACK);
     tft.setTextSize(2);

     // display the server IP address on the TTGO screen
     tft.println("IP address: "); // F6CZV
     tft.println(IP); // F6CZV

     delay(3000); // display the IP address during 3s
     tft.fillRect(0,0,240,135, TFT_BLUE); // erase the screen display
     tft.setTextSize(3);
     tft.setCursor(0, 35); // (x colonne , y ligne) - F6CZV
     tft.print("VFO"); // F6CZV

   // Web server request management :

   // Upload to the web client of the necessary files from the SPIFFS file mgt:
   // - HTML file,
   // - css file,
   // - javascript jogdial.js function to manage the VFO dial
   // - the images of the FT-857D, dial, knob and tx LED.

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/index.html", String(), false, processor);
   });

   server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/style.css","text/css");
  });

  server.on("/jogDial.js", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/jogDial.js","application/javascript");
  });

  server.on("/dial.png", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/dial.png","image/png");
  });

  server.on("/knob.png", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/knob.png","image/png");
  });

   server.on("/FT857D2", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/FT857D2.jpg", "image/jpeg");
   });

   server.on("/redLED", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/redLED.jpg", "image/jpeg");
   });

   server.on("/run", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/run.gif", "image/gif");
   });

   // for each request the value to be displayed on the web page is supplied
   // the values are stored in global variables.
   // If the variable type is not a char * the variable must be converted
   // from String to char * by the c_str()function
   //
   server.on("/vfo", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", VFO1.c_str()); // VFO A or B
   });
   server.on("/smeter", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send_P(200, "text/plain", Smeter.c_str()); // Smeter value
    });
   server.on("/rxtx", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send_P(200, "text/plain", RTStatus.c_str()); // Rx / Tx indication
    });
    server.on("/split", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send_P(200, "text/plain", SPLStatus.c_str()); // Split
    });
    server.on("/mode", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send_P(200, "text/plain",radmode.c_str()); // radio mode
    });
    server.on("/freq", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send_P(200, "text/plain", Wfrequency.c_str()); // frequency
    });

    server.on("/kyr", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send_P(200, "text/plain", KYRStatus.c_str()); // keyer status
    });

    server.on("/bk", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send_P(200, "text/plain", BKStatus.c_str()); // Break-In status
    });

    server.on("/dbf", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send_P(200, "text/plain", DBFStatus.c_str());
    });

    server.on("/dnr", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send_P(200, "text/plain", DNRStatus.c_str());
    });

    server.on("/dnf", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send_P(200, "text/plain", DNFStatus.c_str());
    });
    server.on("/clar", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send_P(200, "text/plain", ClarStatus.c_str());
    });

    // action following the click on the Toggle VFO button
    // a confirmation of good execution - request->send(200 ...) is mandatory to avoid repetitions of the request
    // by the client web page
    //
    server.on("/ToggleVFO", HTTP_GET, [](AsyncWebServerRequest *request){
     radio.switchVFO();
     request->send(200, "text/plain", "OK");
    });

    // action following the click on the Toggle split button
    // a confirmation of good execution - request->send(200 ...) is mandatory to avoid repetitions of the request
    // by the client web page
    //
    server.on("/Togglesplit", HTTP_GET, [](AsyncWebServerRequest *request){
     if (Split) {radio.split(false);}
     else
     {radio.split(true);}
     request->send(200, "text/plain", "OK");
    });

    // action following the selection of the radio mode on the mode form
    // a confirmation of good execution - request->send(200 ...) is mandatory to avoid repetitions of the request
    // by the client web page
    //
    server.on("/setmode", HTTP_GET, [](AsyncWebServerRequest *request){
      reqmode = request->getParam("Fmode")->value();
      // Serial.println(reqmode);
     if (reqmode != radmode) {radio.setMode(reqmode);}
     request->send(200, "text/plain", "OK");
    });
    //
    // action following the input of the frequency on the frequency form
    //
    server.on("/setfreq", HTTP_GET, [](AsyncWebServerRequest *request){
      reqfreq = request->getParam("FFreq")->value() + "00";
      // Serial.println(reqfreq);
     radio.setFreq(reqfreq.toInt());
     request->send(200, "text/plain", "OK");
    });
    //
    // request to update the frequency if the VFO dial was rotated
    //
    server.on("/updatefrequency", HTTP_GET, [](AsyncWebServerRequest *request){
      deltafreq = request->getParam(0)->value();
     // Serial.println(reqfreq);
     radio.setFreq(frequency_int + deltafreq.toInt());
     request->send(200, "text/plain", "OK");
    });



    // action following the click on the Clarifier button. The clarifier indication is only displayed on the web page.
    // As there is no CAT command to read the clarifier status, this status is managed by the software and may be different
    // from the real situation before the sending of the first command
    // a confirmation of good execution - request->send(200 ...) is mandatory to avoid repetitions of the request
    // by the client web page
    //
    server.on("/Toggleclar", HTTP_GET, [](AsyncWebServerRequest *request){
     if (Clar) {radio.clar(false);
                Clar = false; ClarStatus = " ";}
     else
     {radio.clar(true);
      Clar = true;ClarStatus = "-";}
     request->send(200, "text/plain", "OK");
    });

  // Start web server
   server.begin();

  //start the file manager SPIFFS
  if (!SPIFFS.begin(true)) {Serial.println("SPIFFS non démarré");}
}

void loop(){

  // the loop requests the data from the radio, displays it on the TTGO tft screen and updates the global variables
  //
  displayVFO();
  displaySMeter();
  displayRXTX();
  displaySplit_status();
  displayMode();
  displayFreq(frequency_int);
  displayDSP();

  // the status of the keyer and Break-in options are only displayed if the mode is CW or CWR
  //
  if ((radmode == "CW ") || (radmode == "CWR")) {displayCWConf();}
  else {tft.fillRect(160, 110, 75, 15, TFT_BLUE);
        KYRStatus = "   ";
        BKStatus = "  ";
    }
   delay(dly);
}

void displayDSP() {
radio.getAGC_DSP_Conf(AGC,DBF,DNR,DNF); // FT857D-ESP32 library function
tft.setCursor(0,110);
tft.setTextSize(2);
if (DBF) {tft.print("DBF");DBFStatus = "DBF";} else
                      {tft.fillRect(0, 110, 35, 15, TFT_BLUE); // x, y, width, height, color
                       DBFStatus = "   ";}
tft.setCursor(40,110);
if (DNF) {tft.print("DNF");DNFStatus = "DNF";} else
                      {tft.fillRect(40, 110, 35, 15, TFT_BLUE); // x, y, width, height, color
                       DNFStatus = "   ";}
tft.setCursor(80,110);
if (DNR) {tft.print("DNR");DNRStatus = "DNR";} else
                      {tft.fillRect(80, 110, 35, 15, TFT_BLUE); // x, y, width, height, color
                      DNRStatus = "   ";}
}

void displayCWConf() {
radio.getCW_MTR_Conf(MeterConf,Keyer,Break_In); // FT857D-ESP32 library function
tft.setTextSize(2);
tft.setCursor(160,110); // (num colonne , num ligne)
  if (Keyer) {
     tft.print("KYR");
     KYRStatus = "KYR";}
     else {tft.fillRect(160, 110, 35, 15, TFT_BLUE); // x, y, width, height, color
     KYRStatus = "   ";}
  tft.setCursor(210,110); // (colonne , ligne)
  if (Break_In) {
    tft.print("BK");
    BKStatus = "BK";}
     else {tft.fillRect(210, 110, 25, 15, TFT_BLUE); // x, y, width, height, color
     BKStatus = "  ";}
}

  void displayVFO() { // F6CZV
  VFO1 = radio.getVFO(); // FT857D-ESP32 library function
  if (VFO1 != prevVFO) {
  tft.setTextSize(3);
  tft.fillRect(60, 35, 20, 25, TFT_BLUE); // x, y, width, height, color
  tft.setCursor(60,35); // (num colonne , num ligne)
  tft.print(VFO1);
  prevVFO = VFO1;}
  }

  void displaySMeter() { // F6CZV
  Smeter = radio.getSMeter(); // FT857D-ESP32 library function
  if (Smeter != prevSmeter) {
  tft.setTextSize(3);
  tft.setCursor(0,0);
  tft.fillRect(0, 0, 100, 25, TFT_BLUE); // x, y, width, height, color
  prevSmeter = Smeter;
  tft.print(Smeter);
  }
  }

  void displayRXTX() { // F6CZV
  tft.setTextSize(3);
  On_Air = radio.chkTx(); // FT857D-ESP32 library function
  if (On_Air) {
    tft.setCursor(200,0);
    tft.fillRect(195, 0, 45, 25, TFT_RED); // x, y, width, height, color
    tft.print("Tx");
    RTStatus = "Tx";}
  else
  {tft.setCursor(200,0);
  tft.fillRect(195, 0, 45, 25, TFT_BLUE); // x, y, width, height, color
  tft.print("Rx");
  RTStatus = "Rx";
  }
  }

  void displaySplit_status() { // F6CZV
  Split = radio.getSPLIT_status(); // FT857D-ESP32 library function
  tft.setTextSize(3);
  tft.setCursor(130,0);
  if (Split) {
    tft.print("SPL");
    SPLStatus = "SPL";}
  else
   {tft.fillRect(130, 0, 55, 25, TFT_BLUE); // x, y, width, height, color
   SPLStatus = "   ";}
  }

  String displayFreq(unsigned long &freq_int) { // F6CZV
  char frequency[9];
  String Sfrequency;
  byte shift = 0;
  unsigned long tempfreq;
  int n;
  tempfreq = radio.getFreqMode(); // FT857D-ESP32 library function
  freq_int = tempfreq;
  sprintf(frequency, "%lu", tempfreq);

  if (tempfreq < 10000000 & tempfreq >= 1000000) {
   shift = 1;
   }
   if (tempfreq < 1000000 & tempfreq >= 100000) {
   shift = 2;
   }
   if (tempfreq < 100000) {
   shift = 3;
   }
  tft.setTextSize(3);
  tft.setCursor(54,75); // (num colonne , num ligne)
  if (shift != 3) {tft.print(".");}

  for (n = 0; n < 3;n++) {
    tft.setCursor(70+n*17,75); // (num colonne , num ligne)
    tft.fillRect(70+n*17, 75, 17, 25, TFT_BLUE); // x, y, width, height, color
    tft.print(frequency[3+n-shift]);
    Sfrequency = Sfrequency + String(frequency[3+n-shift]);}

  tft.setCursor(120,75); // (num colonne , num ligne)
  tft.print(",");
  Sfrequency = Sfrequency + ",";

  for (n = 0; n < 2;n++) {
    tft.setCursor(135+n*17,75); // (num colonne , num ligne)
    tft.fillRect(135+n*17, 75, 17, 25, TFT_BLUE); // x, y, width, height, color
    tft.print(frequency[6+n-shift]);
    Sfrequency = Sfrequency + String(frequency[6+n-shift]);}

  if (shift != 3) {Sfrequency = "." + Sfrequency;}

  for (n = 2; n >= 0;n--) {
    tft.setCursor(5+n*17,75); // (num colonne , num ligne)
    tft.fillRect(5+n*17, 75, 17, 25, TFT_BLUE); // x, y, width, height, color
    if ((n-shift)>= 0) {tft.print(frequency[n-shift]);
                        Sfrequency = String(frequency[n-shift]) + Sfrequency;}
  }
  tft.setCursor(177,75); // (num colonne , num ligne)
  tft.print("kHz");
  Sfrequency = Sfrequency + " ";
  shift = 0 ;
  Wfrequency = Sfrequency;
  return Wfrequency;
  }

  void displayMode() {
  radmode = radio.getMode(); // FT857D-ESP32 library function
  if (radmode != prevmode) {
  tft.setTextSize(3);
  tft.setCursor(100,35); // (num colonne , num ligne)
  tft.fillRect(100, 35, 51, 25, TFT_BLUE); // x, y, width, height, color
  tft.print(radmode);
  prevmode = radmode;}
  }
