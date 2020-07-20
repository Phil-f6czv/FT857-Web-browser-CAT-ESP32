/*
  FT857D-ESP32.cpp	ESP32 library for controlling a Yaesu FT857D
			radio via CAT commands.
 
 Version:  0.1 for Arduino
 Created:  2012.08.16
Released:  2012.08.17
  Author:  James Buck, VE3BUX
     Web:  http://www.ve3bux.com

Version:  CAT-FT857-ESP32 V2.0
 Created:  2020.05.01
Released:  2020.07.09
  Author:  Philippe Lonc, F6CZV

Version 1.1 : 
  - char getVFO() is now String get VFO(),
  - setMode(String mode), squelch(String mode), rptrOffset(String ofst) and squelchFreq(unsigned int freq, String sqlType) parameters are now a String (were char*)

LIMITATION OF LIABILITY :
 This source code is provided "as-is". It may contain bugs. 
Any damages resulting from its use is done under the sole responsibility of the user/developper
 and beyond my responsibility.


*/

#include <Arduino.h> // ESP32
#include <HardwareSerial.h> // ESP32 : SoftwareSerial (Arduino) was replaced by HardwareSerial
#include "FT857D-ESP32.h"

// define hardware serial port here:
extern HardwareSerial rigCat(2); //  UART 3 of ESP32. 

#define dlyTime 5	// delay (in ms) after serial writes

FT857D::FT857D(){ }	// nothing to do when first instanced

//********************************************************************

// Setup software serial with user defined input
// from the Arduino sketch (function, though very slow)
/* void FT857D::setSerial(SoftwareSerial portInfo) {
  rigCat = portInfo;
} 
function put in comments by F6CZV - a priori not used */

//********************************************************************

// similar to Serial.begin(baud)
  void FT857D::begin(int baud) {
  rigCat.begin(baud, SERIAL_8N2, 26, 27); // speed - 8 bits - No parity - 2 stop bits - pin Rx - pin Tx  
}

//********************************************************************

// lock or unlock the radio
void FT857D::lock(boolean toggle) {
	if (toggle == true) singleCmd(CAT_LOCK_ON);
	if (toggle == false) singleCmd(CAT_LOCK_OFF);
}

//********************************************************************

// set or release the virtual PTT button
void FT857D::PTT(boolean toggle) {
	if (toggle == true) singleCmd(CAT_PTT_ON);
	if (toggle == false) singleCmd(CAT_PTT_OFF);
}

//********************************************************************

// set radio frequency directly (as a long integer)
void FT857D::setFreq(long freq) {
	byte rigFreq[5] = {0x00,0x00,0x00,0x00,0x00};
rigFreq[4] = CAT_FREQ_SET; // command byte

	unsigned char tempWord[4];
	converted = to_bcd_be(tempWord, freq, 8);

	for (byte i=0; i<4; i++){
		rigFreq[i] = converted[i];
	}

sendCmd(rigFreq,5);
	getByte();
}

//********************************************************************

// set radio mode using human friendly terms (ie. USB)
void FT857D::setMode(String mode) {
	byte rigMode[5] = {0x00,0x00,0x00,0x00,0x00};
rigMode[0] = CAT_MODE_USB; // default to USB mode
rigMode[4] = CAT_MODE_SET; // command byte

if (mode == "LSB") 	rigMode[0] = CAT_MODE_LSB;
if (mode == "USB")  	rigMode[0] = CAT_MODE_USB;
if (mode == "CW") 	rigMode[0] = CAT_MODE_CW;
if (mode == "CWR")      rigMode[0] = CAT_MODE_CWR;
if (mode == "AM") 	rigMode[0] = CAT_MODE_AM;
if (mode == "FM") 	rigMode[0] = CAT_MODE_FM;
if (mode == "DIG")	rigMode[0] = CAT_MODE_DIG;
if (mode == "PKT") 	rigMode[0] = CAT_MODE_PKT;
if (mode == "FMN") 	rigMode[0] = CAT_MODE_FMN;

sendCmd(rigMode,5);
	getByte();
}

//********************************************************************

// turn the clarifier on or off
void FT857D::clar(boolean toggle) {
	if (toggle == true) singleCmd(CAT_CLAR_ON);
	if (toggle == false) singleCmd(CAT_CLAR_OFF);
}

//********************************************************************

// set the clarifier frequency
void FT857D::clarFreq(long freq) {
	// will come back to this later
}

//********************************************************************

// switch between VFO A and VFO B
void FT857D::switchVFO() {
	singleCmd(CAT_VFO_AB);
}

//********************************************************************

// turn split operation on or off
void FT857D::split(boolean toggle) {
	if (toggle == true) singleCmd(CAT_SPLIT_ON);
	if (toggle == false) singleCmd(CAT_SPLIT_OFF);
}

//********************************************************************

// control repeater offset direction
void FT857D::rptrOffset(String ofst) {
	byte rigOfst[5] = {0x00,0x00,0x00,0x00,0x00};
	rigOfst[0] = CAT_RPTR_OFFSET_S; // default to simplex
	rigOfst[4] = CAT_RPTR_OFFSET_CMD; // command byte

if (ofst == "-") rigOfst[0] = CAT_RPTR_OFFSET_N;
if (ofst == "+") rigOfst[0] = CAT_RPTR_OFFSET_P;
if (ofst == "s") rigOfst[0] = CAT_RPTR_OFFSET_S;

sendCmd(rigOfst,5);
	getByte();
}

//********************************************************************

void FT857D::rptrOffsetFreq(long freq) {
	byte offsetFreq[5] = {0x00,0x00,0x00,0x00,0x00};
	offsetFreq[4] = CAT_RPTR_FREQ_SET; // command byte

freq = (freq * 100); // convert the incoming value to kHz

	unsigned char tempWord[4];
	converted = to_bcd_be(tempWord, freq, 8);

	for (byte i=0; i<4; i++){
		offsetFreq[i] = converted[i];
	}

	sendCmd(offsetFreq,5);
	getByte();
}

//********************************************************************

// enable or disable various CTCSS and DCS squelch options
void FT857D::squelch(String mode) {
	byte rigSql[5] = {0x00,0x00,0x00,0x00,0x00};
	rigSql[0] = CAT_MODE_USB; // default to USB mode
	rigSql[4] = CAT_SQL_CMD; // command byte

if (mode == "DCS")	rigSql[0]= CAT_SQL_DCS;
if (mode == "DDC")	rigSql[0]= CAT_SQL_DCS_DECD;
if (mode == "DEN") 	rigSql[0]= CAT_SQL_DCS_ENCD;
if (mode == "TSQ") 	rigSql[0]= CAT_SQL_CTCSS;
if (mode == "TDC")	rigSql[0]=CAT_SQL_CTCSS_DECD;
if (mode == "TEN") 	rigSql[0]=CAT_SQL_CTCSS_ENCD;
if (mode == "OFF") 	rigSql[0]= CAT_SQL_OFF;

	sendCmd(rigSql,5);
	getByte();
}

//********************************************************************

void FT857D::squelchFreq(unsigned int freq, String sqlType) {
	byte rigSqlFreq[5] = {0x00,0x00,0x00,0x00,0x00};
	if (sqlType == "C") rigSqlFreq[4] = CAT_SQL_CTCSS_SET;
	if (sqlType == "D") rigSqlFreq[4] = CAT_SQL_DCS_SET;
	
	byte freq_bcd[2];
	to_bcd_be(freq_bcd, (long)  freq, 4);

	for (byte i=0; i<4; i++){
		rigSqlFreq[i] = freq_bcd[i];
	}
	sendCmd(rigSqlFreq,5);
	getByte();
}

//********************************************************************


    String FT857D::getMode() {
	unsigned long l = getFreqMode();
	return mode;
}

//********************************************************************

// get the frequency and the current mode
// if called as getFreqMode() return only the frequency

unsigned long FT857D::getFreqMode() {
	byte rigGetFreq[5] = {0x00,0x00,0x00,0x00,0x00};
	rigGetFreq[4] = CAT_RX_FREQ_CMD; // command byte
	byte chars[4];
	long timeout = millis();
	long elapsed = 0;

//	rigCat.flush();	// clear the RX buffer which helps prevent
				// any crap data from making it through

	sendCmd(rigGetFreq, 5);
	
	while (rigCat.available() < 5 && elapsed < 2000) {
		elapsed = millis() - timeout;
     	;}
	
	for (int j = 0; j < 4; j++) {
		chars[j] = rigCat.read();  
   	}
      /*  in V0.1 mode was a byte. The returned mode is now a user-friendly string directly displayable - F6CZV */

        byte modeint;
        modeint = rigCat.read(); // F6CZV
       
      switch (modeint)
       {
        case 0xFC:
           mode = "PKT";
           break;
       
	case CAT_MODE_LSB:
           mode = "LSB";
           break;

	case CAT_MODE_USB:
            mode = "USB";
            break;

	case CAT_MODE_CW:
            mode = "CW ";
            break;

	case CAT_MODE_FM:
            mode = "FM ";
           break;

	case CAT_MODE_WFM:
            mode = "WFM";
           break;

	case CAT_MODE_CWR:
            mode = "CWR";
           break;

	case CAT_MODE_AM:
            mode = "AM ";
           break;

	case CAT_MODE_FMN:
            mode = "FMN";
           break;

	case CAT_MODE_DIG:
            mode = "DIG";
           break;

	default:
            mode = "UNK";
            break;
          }
       // Serial.println(modeint);
	freq = from_bcd_be(chars, 8);
	return freq;
}

//********************************************************************




//********************************************************************

// determine if the radio is in TX state
// unless the radio is actively TX, the result is always
// 0x255 so any value other than 0x255 means TX !

bool FT857D::chkTx() {                         // was boolean F6CZV
	byte rigTXState[5] = {0x00,0x00,0x00,0x00,0x00};
	rigTXState[4] = CAT_TX_DATA_CMD;
	
	sendCmd(rigTXState, 5);

	byte reply = getByte();
	
	if (reply == 255) { // was ==0 F6CZV
		return false;
	}
	else {
		return true;
	}
}

//********************************************************************

// get the S Meter value from the radio F6CZV
// 

String FT857D::getSMeter() {   
	byte rigTXState[5] = {0x00,0x00,0x00,0x00,0x00};
	rigTXState[4] = CAT_RX_DATA_CMD;
	String SMeterl;
	byte reply;
	
	sendCmd(rigTXState, 5);	
	
	reply = getByte();
	
	reply = reply & 0x0f;

	if (reply < 10) {SMeterl = "S" + String(reply);
	}
	else {

	 switch (reply)
       {
        case 10:
           SMeterl = "S9+10";
           break;
       
	case 11:
           SMeterl = "S9+20";
           break;

	case 12:
           SMeterl = "S9+30";
            break;

	case 13:
            SMeterl = "S9+40";
            break;

	case 14:
            SMeterl = "S9+50";
           break;

	case 15:
            SMeterl = "S9+60";
           break;

	default:
            SMeterl = "UNK  ";
            break;
          }
	}
        
	return SMeterl;
}


//********************************************************************

// get the VFO status from the radio F6CZV
// 

 String FT857D::getVFO() {   
	byte rigTXState[5] = {0x00,0x00,0x00,0x00,0x00};
	rigTXState[4] = CAT_EEPROM_READ_CMD;
	rigTXState[0] = MSB_ADD_VFO_status;
	rigTXState[1] = LSB_ADD_VFO_status;
	String VFO;
	VFO = ' ';
	
	sendCmd(rigTXState, 5);

	byte reply = getByte();
	
	if (reply == 0x80) {VFO = 'a';}

	else {VFO = 'b';}

        reply = getByte(); // byte discarded
	return VFO;
}

//********************************************************************

// get the MTR, Keyer and Break-In configurations from the radio F6CZV
// 

   void FT857D::getCW_MTR_Conf(byte &MTR,bool &KYR,bool &BK) {   
	byte rigTXState[5] = {0x00,0x00,0x00,0x00,0x00};
	rigTXState[4] = CAT_EEPROM_READ_CMD;
	rigTXState[0] = MSB_ADD_CW_MTR_CONF;
	rigTXState[1] = LSB_ADD_CW_MTR_CONF;
	
	sendCmd(rigTXState, 5);

	byte reply = getByte();
	MTR = reply & 0x03;
	KYR = reply & 0x10;
	BK = reply & 0x20;

        reply = getByte(); // byte discarded

}

//********************************************************************

// get the AGC, DBF, DNF and DNR configurations from the radio F6CZV
// 

   void FT857D::getAGC_DSP_Conf(bool &AGC,bool &DBF,bool &DNR, bool &DNF) {   
	byte rigTXState[5] = {0x00,0x00,0x00,0x00,0x00};
	rigTXState[4] = CAT_EEPROM_READ_CMD;
	rigTXState[0] = MSB_ADD_AGC_DSP_CONF;
	rigTXState[1] = LSB_ADD_AGC_DSP_CONF;
	
	sendCmd(rigTXState, 5);

	byte reply = getByte();
	AGC = reply & 0x20;
	DBF = reply & 0x04; // only one bit is tested
	DNR = reply & 0x02;
	DNF = reply & 0x01;

        reply = getByte(); // byte discarded

}

//********************************************************************

// get the SPLIT status from the radio F6CZV
// 

   bool FT857D::getSPLIT_status() {   
	byte rigTXState[5] = {0x00,0x00,0x00,0x00,0x00};
	bool Status = false;
	rigTXState[4] = CAT_EEPROM_READ_CMD;
	rigTXState[0] = MSB_ADD_SPLIT_STATUS;
	rigTXState[1] = LSB_ADD_SPLIT_STATUS;
	
	sendCmd(rigTXState, 5);

	byte reply = getByte();
	Status = reply & 0x80;
	
        reply = getByte(); // byte discarded
	return Status;

}

//********************************************************************

// spit out any DEBUG data via this function
void FT857D::comError(char * string) {
 Serial.println("Communication Error!");
 Serial.println(string);
}

//********************************************************************

// gets a byte of input data from the radio
byte FT857D::getByte() {
	unsigned long startTime = millis();
	while (rigCat.available() < 1 && millis() < startTime + 2000) {
		;
	}
	byte radioReply = rigCat.read();
	return radioReply ;
}

//********************************************************************

// this is the function which actually does the 
// serial transaction to the radio
void FT857D::sendCmd(byte cmd[], byte len) {
	for (byte i=0; i<len; i++) {
		rigCat.write(cmd[i]);
	}
//	return getByte();	// should make this work more quickly
					// in a future update
}

//********************************************************************

// this function reduces total code-space by allowing for
// single byte commands to be issued (ie. all the toggles)
byte FT857D::singleCmd(int cmd) {
	byte outByte[5] = {0x00,0x00,0x00,0x00,0x00};
	outByte[4] = cmd;
	sendCmd(outByte, 5);
	return getByte();
}

//********************************************************************

// send a single byte of data (will be removed later)
void FT857D::sendByte(byte cmd) {
	rigCat.write(cmd);
}

void FT857D::flushRX() {
	rigCat.flush();
}

//********************************************************************

// GPL
// taken from hamlib work
unsigned long FT857D::from_bcd_be(const  byte bcd_data[], unsigned bcd_len)
{
	int i;
	long f = 0;

	for (i=0; i < bcd_len/2; i++) {
		f *= 10;
		f += bcd_data[i]>>4;
		f *= 10;
		f += bcd_data[i] & 0x0f;
	}
	if (bcd_len&1) {
		f *= 10;
		f += bcd_data[bcd_len/2]>>4;
	}
	return f;
}

//********************************************************************

// GPL
// taken from hamlib work
unsigned char * FT857D::to_bcd_be( unsigned char bcd_data[], unsigned long  freq, unsigned bcd_len)
{
	int i;
	unsigned char a;

	if (bcd_len&1) {
		bcd_data[bcd_len/2] &= 0x0f;
		bcd_data[bcd_len/2] |= (freq%10)<<4;
/* NB: low nibble is left uncleared */
		freq /= 10;
	}
	for (i=(bcd_len/2)-1; i >= 0; i--) {
		a = freq%10;
		freq /= 10;
		a |= (freq%10)<<4;
		freq /= 10;
		bcd_data[i] = a;
	}
	return bcd_data;
}