/*
  FT857D-ESP32.h		ESP32 library for controlling a Yaesu FT857D
			radio via CAT commands.

 Version:  0.1 for Arduino
 Created:  2012.08.16
Released:  2012.08.17
  Author:  James Buck, VE3BUX
     Web:  http://www.ve3bux.com

Version:  CAT-FT857D-ESP32 2.0
 Created:  2020.05.01
Released:  2020.07.09
  Author:  Philippe Lonc, F6CZV
      
LIMITATION OF LIABILITY :
 This source code is provided "as-is". It may contain bugs. 
Any damages resulting from its use is done under the sole responsibility of the user/developper
 and beyond my responsibility.

version 1.1 
- char getVFO(), char* getmode() are now String getVFO() and String getMode()
-  setMode(String mode), squelch(String mode), rptrOffset(String ofst) and squelchFreq(unsigned int freq, String sqlType) parameters are now a String (were char*)


CAT commands for FT-857D radio taken from the FT-857D Manual (page 66):
	
http://www.yaesu.co.uk/files/FT-857D_Operating%20Manual.pdf

All CAT commands to the radio should be sent as 5-byte blocks. The commands are generally in the form of: 

	{P1,P2,P3,P4,CMD}
		where P1-P4 are parameters
		and   CMD is the command code (ie. set mode)

----Lock On / Off------------------------------------------------
	{0x00,0x00,0x00,0x00,CAT_LOCK_ON}
	{0x00,0x00,0x00,0x00,CAT_LOCK_OFF}

	Toggle the lock as: {0x00,0x00,0x00,0x00,CMD}
	CMD byte:
		0x00 = lock on				= CAT_LOCK_ON
		0x80 = lock off				= CAT_LOCK_OFF

	Eg: {0x00,0x00,0x00,0x00,0x00} enables the lock

----PTT On / Off-------------------------------------------------
	{0x00,0x00,0x00,0x00,CAT_PTT_ON}
	{0x00,0x00,0x00,0x00,CAT_PTT_OFF}

	Toggle PTT as: {0x00,0x00,0x00,0x00,CMD}
	CMD byte:
		0x08 = PTT on				= CAT_PTT_ON
		0x88 = PTT off				= CAT_PTT_OFF

	Eg: {0x00,0x00,0x00,0x00,0x08} causes radio to TX

----Set Frequency------------------------------------------------
	{0x00,0x00,0x00,0x00,CAT_FREQ_SET}

	Tune the radio to a frequency as: {P1,P2,P3,P4,0x01}
	Parameters:
		P1-P4 = frequency as: aa,bb,cc,dd
		
	Eg: {0x01,0x40,0x07,0x00,0x01} tunes to 14.070MHz
	Eg2:{0x14,0x43,0x90,0x00,0x01} tunes to 144.390MHz

----Operating Mode-----------------------------------------------
	{0x00,0x00,0x00,0x00,CAT_MODE_SET}

	Set radio mode as: {P1,0x00,0x00,0x00,0x07}
	P1 Byte:
	0x00 = LSB					= CAT_MODE_LSB
	0x01 = USB					= CAT_MODE_USB
  	0x02 = CW					= CAT_MODE_CW
  	0x03 = CW-R					= CAT_MODE_CWR
  	0x04 = AM					= CAT_MODE_AM
  	0x08 = FM					= CAT_MODE_FM
  	0x0A = DIG					= CAT_MODE_DIG
  	0x0C = PKT					= CAT_MODE_PKT
  	0x88 = FM-N					= CAT_MODE_FMN

	Eg: {0x00,0x00,0x00,0x00,0x07} sets the radio to LSB

----Clar On / Off------------------------------------------------
	{0x00,0x00,0x00,0x00,CAT_CLAR_ON}
	{0x00,0x00,0x00,0x00,CAT_CLAR_OFF}

	Toggle clarifier as: {0x00,0x00,0x00,0x00,CMD}
	CMD byte:
		0x05 = CLAR on				= CAT_CLAR_ON
		0x85 = CLAR off				= CAT_CLAR_OFF

	Eg: {0x00,0x00,0x00,0x00,0x05} turns on the clarifier

----Clar Frequency-----------------------------------------------
	{0x00,0x00,0x00,0x00,CAT_CLAR_SET}

	Set clarifier frequency as: {P1,0x00,P3,P4,0xF5}
	P1 byte:
		0x00 = toggle between + and - offset
	P3,P4 bytes:
			= frequency as: aa,bb

	Eg: {0x00,0x12,0x34,0xF5} sets the clarifier to 12.34kHz
	Eg2:{0x00,0x06,0x50,0xF5} sets the clarifier to -6.5kHz

----VFO A/B------------------------------------------------------
	{0x00,0x00,0x00,0x00,CAT_VFO_AB}

	Toggle between VFO A and B as: {0x00,0x00,0x00,0x00,CMD}
	CMD byte:
		0x81 = chg  between A & B		= CAT_VFO_AB

	Eg: {0x00,0x00,0x00,0x00,0x81} change from VFO A to VFO B

----Split On / Off-----------------------------------------------
	{0x00,0x00,0x00,0x00,CAT_SPLIT_ON}
	{0x00,0x00,0x00,0x00,CAT_SPLIT_OFF}

	Toggle Split operation as: {0x00,0x00,0x00,0x00,CMD}
	CMD byte:
		0x02 = split on				= CAT_SPLIT_ON
		0x82 = split off				= CAT_SPLIT_OFF

	Eg: {0x00,0x00,0x00,0x00,0x02} enables split mode operation

----Repeater Offset---------------------------------------------
	{P1,0x00,0x00,0x00,CAT_RPTR_OFFSET_CMD}

	Set repeater offset as: {P1,0x00,0x00,0x00,0x09}
	P1 byte:
		0x09 = negative shift			= CAT_RPTR_OFFSET_N
		0x49 = positive shift			= CAT_RPTR_OFFSET_P
		0x89 = simplex				= CAT_RPTR_OFFSET_S

	Eg: {0x49,0x00,0x00,0x00,0x09} sets positive repeater shift

----Repeater Offset Frequency-----------------------------------
	{0x00,0x00,0x00,0x00,CAT_RPTR_FREQ_SET}

	Set repeater offset frequency as: {P1,P2,P3,P4,0xF9}
	Parameters:
		P1-P4 = frequency as: aa,bb,cc,dd
		
	Eg: {0x05,0x43,0x21,0x00,0xF9} sets offset to 5.4321MHz
	Eg2:{0x00,0x60,0x00,0x00,0xF9} sets offset to 0.600MHz
	
----CTCSS / DCS Mode--------------------------------------------
	{P1,0x00,0x00,0x00,CAT_SQL_CMD}

	Select CTCSS / DCS mode as: {P1,0x00,0x00,0x00,0x0A}
	P1 Byte:
	0x0A = DCS on					= CAT_SQL_DCS
	0x0B = DCS decoder on 			= CAT_SQL_DCS_DECD
  	0x0C = DCS encoder on 			= CAT_SQL_DCS_ENCD
  	0x2A = CTCSS on					= CAT_SQL_CTCSS
  	0x3A = CTCSS decoder on			= CAT_SQL_CTCSS_DECD
  	0x4A = CTCSS encoder on			= CAT_SQL_CTCSS_ENCD
  	0x8A = off						= CAT_SQL_OFF

	Eg: {0x2A,0x00,0x00,0x00,0x0A} enables CTCSS tones on TX&RX

----CTCSS Tone--------------------------------------------------
	{0x00,0x00,0x00,0x00,CAT_SQL_CTCSS_SET}

	Set CTCSS tones as: {P1,P2,P3,P4,0x0B}
	Parameters:
		P1-P2 = CTCSS tone on TX as: aa,bb
		P3-P4 = CTCSS tone on RX as: cc,dd
		
	Eg: {0x08,0x85,0x10,0x00,0x0B} CTCSS TX = 88.5Hz, RX = 100Hz
	Eg2:{0x10,0x00,0x10,0x00,0x0B} CTCSS TX&RX = 100Hz
	
----DCS Code----------------------------------------------------
	{0x00,0x00,0x00,0x00,CAT_SQL_DCS_SET}

	Set DCS codes as: {P1,P2,P3,P4,0x0C}
	Parameters:
		P1-P2 = DCS code on TX as: aa,bb
		P3-P4 = DCS code on RX as: cc,dd

	Eg: {0x00,0x23,0x03,0x71,0x0C} DCS TX = 023, RX = 371
	
----Read RX Status (Squelch, CTCSS/DCS, Disc. Ctr, S-Meter)------
	{0x00,0x00,0x00,0x00,CAT_RX_DATA_CMD}

	Read the RX status as: {0x00,0x00,0x00,0x00,E7}

	The radio will then output 1 byte of data to report the various parameters:

	{D1}

	Data:
		bit 7			i = 0 = squelch off (signal present)
					i = 1 = squelch off (no signal)
		bit 6			j = 0 = CTCSS/DCS matched
					j = 1 = CTCSS/DCS unmatched
					CTSS/DCS OFF : 0
		bit 5			k = 0 = discriminator centered
					k = 1 = discriminator offcentered
		bit 4			l = dummy data
		bits 0-3 = S-meter data

	Eg: {0x00,0x00,0x00,0x00,0xE7} gets the radio status

----Read TX Status (PTT, SWR Hi/Lo, Split, PO Meter)------------
	{0x00,0x00,0x00,0x00,CAT_TX_DATA_CMD}

	Read the TX status as: {0x00,0x00,0x00,0x00,F7}

	The radio will then output 1 byte of data to report the various parameters:

	{D1}

	Data:
		bit 7			0 = PTT off
					1 = PTT on
		bit 6			0 = HI SWR off
					1 = HI SWR on
		bit 5			0 = split on
					1 = split off
		bit 4			dummy data
		bits 0-3		PO meter data

----Read Frequency / Mode Status---------------------------------
	{0x00,0x00,0x00,0x00,CAT_RX_FREQ_CMD}

	Read the frequency and mode as: {0x00,0x00,0x00,0x00,03}

	The radio will then output 5 bytes of data to report the 	current frequency and mode:

	{D1,D2,D3,D4,D5}

	Data:
		D1-D4 = frequency
		D5 = mode (same as set mode)

	Eg: {0x00,0x00,0x00,0x00,0x03} gets the current freq & mode

----Read VFO Status---------------------------------

	{MSB_ADD_VFO_status,LSB_ADD_VFO_status,0x00,0x00,CAT_EEPROM_READ_CMD}

	Read the VFO status as: {0x00,0x68,0x00,0x00,BB}

	The radio will then output 2 bytes of data to report the VFO status (A or B):

	{D1,D2}

	Data:
		bit 0 of D1 : 0- VFO A / 1 - VFO B

	D2 is discarded		

	Eg: {0x00,0x68,0x00,0x00,0xBB} gets the VFO status from EEPROM
	
	Result from radio is:
		{0x81,0x80} which means VFO B active
	
----Read the CW and MTR configurations---------------------------------

	{MSB_ADD_CW_CONF,LSB_ADD_CW_CONF,0x00,0x00,CAT_EEPROM_READ_CMD}

	Read the MTR / CW configurations as: {0x00,0x6B,0x00,0x00,BB}

	The radio will then output 2 bytes of data to report the Meter configuration, 
	the KYR status and the BK status:

	{D1,D2}

	Data:
		bits 0 and 1 of D1 : MTR configuration (MFi) 00=PWR, 01=ALC, 10=SWR, 11=MOD
		bit 4 of D1 : KYR (MFj) 0=OFF - 1=ON
		bit 5 of D1 : BK  (MFj) 0=OFF - 1=ON

	D2 is discarded
		

	Eg: {0x00,0x6B,0x00,0x00,0xBB} gets the CW configuration from EEPROM
	
	Result from radio is:
			     7654-3210
		{0x62,0x13} (0110-0010) which means Break-in on, Keyer OFF, MTR SWR

----Read the AGC, DBF, DNF and DNR configurations---------------------------------

	{MSB_ADD_DSP_CONF,LSB_ADD_DSP_CONF,0x00,0x00,CAT_EEPROM_READ_CMD}

	Read the AGC, DBF, DNF and DNR configurations as: {0x00,0xA8,0x00,0x00,BB}

	The radio will then output 2 bytes of data to report the AGC, DBF, DNF
	 and DNR configurations :

	{D1,D2}

	Data:
		bit 0 of D1 : DNF (MFp) 0=OFF - 1=ON
		bit 1 of D1 : DNR (MFp) 0=OFF - 1=ON
		bits 3-2 of D1 : DBF (MFp) 00=OFF - 11=ON
		bit 5 of D1 : AGC (MFl) 0=OFF - 1=ON

	D2 is discarded
		

	Eg: {0x00,0xA8,0x00,0x00,0xBB} gets the AGC and DSP configurations from EEPROM
	
	Result from radio is:
			     7654-3210
		{0x23,0x70} (0010-0011) which means AGC ON, DBF OFF, DNR ON, DNF ON

----Read the SPLIT status---------------------------------

	{MSB_ADD_SPLIT_CONF,LSB_ADD_SPLIT_CONF,0x00,0x00,CAT_EEPROM_READ_CMD}

	Read the SPLIT status as: {0x00,0x8D,0x00,0x00,BB}

	The radio will then output 2 bytes of data to report the SPLIT status :

	{D1,D2}

	Data:
		bit 7 of D1 : SPLIT (MFa) 0=OFF - 1=ON

	D2 is discarded
		

	Eg: {0x00,0x8D,0x00,0x00,0xBB} gets the SPLIT status from EEPROM
	
	Result from radio is:
			     7654-3210
		{0x83,0x10} (1000-0011) which means SPLIT ON

----------------------------------------------------------------
*/

#ifndef CAT_h
#define CAT_h

#include <Arduino.h>  // ESP32 library
#include <HardwareSerial.h> // ESP32 SoftwareSerial was replaced by HardwareSerial

// New constants added by F6CZV

#define LSB_ADD_VFO_status		0x68
#define MSB_ADD_VFO_status		0x00
#define LSB_ADD_CW_MTR_CONF		0x6B
#define MSB_ADD_CW_MTR_CONF		0x00
#define LSB_ADD_AGC_DSP_CONF		0xA8
#define MSB_ADD_AGC_DSP_CONF		0x00
#define LSB_ADD_SPLIT_STATUS		0x8D
#define MSB_ADD_SPLIT_STATUS		0x00

#define CAT_EEPROM_READ_CMD		0xBB

// end of new constants

#define CAT_LOCK_ON			0x00
#define CAT_LOCK_OFF			0x80
#define CAT_PTT_ON			0x08
#define CAT_PTT_OFF			0x88
#define CAT_FREQ_SET			0x01
#define CAT_MODE_SET			0x07
#define CAT_MODE_LSB			0x00
#define CAT_MODE_USB			0x01
#define CAT_MODE_CW			0x02
#define CAT_MODE_CWR			0x03
#define CAT_MODE_AM			0x04
#define CAT_MODE_FM			0x08
#define CAT_MODE_DIG			0x0A
#define CAT_MODE_PKT			0x0C
#define CAT_MODE_FMN			0x88
#define CAT_MODE_WFM			0x06
#define CAT_CLAR_ON			0x05
#define CAT_CLAR_OFF			0x85
#define CAT_CLAR_SET			0xF5
#define CAT_VFO_AB			0x81
#define CAT_SPLIT_ON			0x02
#define CAT_SPLIT_OFF			0x82
#define CAT_RPTR_OFFSET_CMD		0x09
#define CAT_RPTR_OFFSET_N		0x09 // -
#define CAT_RPTR_OFFSET_P		0x49 // +
#define CAT_RPTR_OFFSET_S		0x89 // simlex
#define CAT_RPTR_FREQ_SET		0xF9
#define CAT_SQL_CMD			0x0A //{P1 ,0x00,0x00,0x00,0x0A}
#define CAT_SQL_DCS			0x0A // all values below are P1
#define CAT_SQL_DCS_DECD		0x0B // only useful in "split"
#define CAT_SQL_DCS_ENCD		0x0C
#define CAT_SQL_CTCSS			0x2A
#define CAT_SQL_CTCSS_DECD		0x3A
#define CAT_SQL_CTCSS_ENCD		0x4A
#define CAT_SQL_OFF			0x8A
#define CAT_SQL_CTCSS_SET		0x0B
#define CAT_SQL_DCS_SET			0x0C
#define CAT_RX_DATA_CMD			0xE7
#define CAT_TX_DATA_CMD			0xF7
#define CAT_RX_FREQ_CMD			0x03
#define CAT_NULL_DATA			0x00

class FT857D
{
  public:
	FT857D();
//	void setSerial(SoftwareSerial portInfo); // a priori not used - F6CZV
	void begin(int baud);

	void lock(boolean toggle);
	void PTT(boolean toggle);
	void setFreq(long freq);
	void setMode(String mode);
	void clar(boolean toggle);
	void clarFreq(long freq);
	void switchVFO();
	void split(boolean toggle);
	void rptrOffset(String ofst);
	void rptrOffsetFreq(long freq);
	void squelch(String mode);
	void squelchFreq(unsigned int, String sqlType);
	String getMode(); // modified by F6CZV
	String getVFO(); // new function F6CZV
	unsigned long getFreqMode();
	bool chkTx(); // was boolean F6CZV
        String getSMeter(); // new function F6CZV
	void getCW_MTR_Conf(byte &MTR,bool &KYR,bool &BK); // new function F6CZV
	void getAGC_DSP_Conf(bool &AGC,bool &DBF,bool &DNR, bool &DNF); // new function F6CZV
	bool getSPLIT_status(); // new function F6CZV
	void flushRX();
	

  private:
	unsigned char * converted;		// holds the converted freq
	unsigned long freq;			// frequency data as a long
	unsigned char tempWord[4];		// temp value during conv.
	String mode; // F6CZV

	void sendCmd(byte cmd[], byte len);
	byte singleCmd(int cmd);		// simplifies small cmds
	byte getByte();

	void sendByte(byte cmd);
	unsigned long from_bcd_be(const byte bcd_data[], unsigned bcd_len);
	unsigned char * to_bcd_be( byte bcd_data[], unsigned long freq, unsigned bcd_len);
	void comError(char * string);
};

#endif