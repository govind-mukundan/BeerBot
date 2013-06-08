/*

 Description: INO for beerbot task #2 in Basecamp.
 Hardware: Arduino Leonardo
 Shields: SMS Shield -> https://www.sparkfun.com/products/9607
          GLCD       -> https://www.sparkfun.com/products/710
 Author: Govind Mukundan (govind dot mukundan at gmail dot com)
 Date: 12/Feb/2013
 Version: 1.2
 References: Basic intro to SM5100 -> http://tronixstuff.wordpress.com/tag/sm5100/
             Info about GLCD library and pinout --> http://playground.arduino.cc/Code/GLCDks0108
 History:
 1.2: Integrated GLCD support, and tested with Starhub+Singtel SIMs
 1.1: First Version, tested at home without Bot using SINGTEL SIM card
 
 
 */


#include <string.h>         //Used for string manipulations
#include <ctype.h>
#include <EEPROM.h>
#include <glcd.h>
#include "bitmaps/allBitmaps.h"       // all images in the bitmap dir
#include "fonts/SystemFont5x7.h"       // system font

#define SM_BUFFER_LEN  100
#define DEBUGGING (0)  // make 0 before release

#define C_EEPROM_APPROVALID_ADDRESS            (0x100)
#define C_EEPROM_APPROVALID_SIZE_ADDRESS       (0x100 + 50u)

#define C_APPROVAL_ID_LEN    (20)
#define LED_PIN      (3) // On board LED at pin13 (L)
#define C_ERROR_BLINK_RATE          (1000ul)
#define C_AUTHCODE_HEADER_BYTES    (3) // AC:

char incoming_char=0;      //Will hold the incoming character from the Serial Port.
//String inputString = "";         // a string to hold incoming data
char SMBuffer[SM_BUFFER_LEN];
char BufferTail;
char CmdStart = 0;
boolean CmdComplete = false;  // whether the string is complete
// EEPROM copy
byte LastApprovalIdLen;
char LastApprovalId[C_APPROVAL_ID_LEN];

// Hardware
int left = 12;
int right = 13;
int vendside = 0;

unsigned long blinkTimeStamp = 0;                // Time Stamp
unsigned long blinkTimePeriod = 0;
unsigned long errorTimeStamp = 0;

#define C_ERROR_TIMEOUT    (90000) //1.5 min

enum {
  SM_SIM_REMOVED,
  SM_SIM_INSERTED,
  SM_RING_MELODY,
  SM_CALL_READY,
  SM_SMS_READY,
  SM_ID_RELEASED_CALLS,
  SM_RELEASED_CALL_ID,
  SM_NETWORK_EMERGENCY,
  SM_NETWORK_LOST,
  SM_AUDIO_ON,
  SM_PHONEBOOK_STATUS_INIT,
  SM_NETWORK_REGISTERED,
  SM_DUMMY
} 
SM5100StatusCodes;

typedef enum{
  SM5100Init,
  SM5100Ready,
  SM5100WForSms,
  SM5100Error

}
e_SM5100States;

e_SM5100States SM5100State;

void SM5100LoopBack(void); // Useful for testing, to control the shield from PC Terminal. Just call this in the loop(), disable everything else.
void setBlinkRate(byte value);
void Blink(void);
void SM5100RxTask(void) ;
void SM5100ParseStatus(void);
void SM5100StateMachine(void);
void SM500InitializeSMS(void);
void SM5100ParseSms(void);


// ------------------  Display Related Declerations ---------------
#define BEERBOT_VERSION  1
#define TIME_TEXT_DISP    5
#define TIME_SPLASH_DISP    5000
#define SPLASH_MSG1 "Maxus "
#define SPLASH_MSG2 "Technology"
#define VEND_MSG "Please Collect Your Beer"
#define CONNECT_MSG "Connecting to GSM + WiFi ..."
#define READY_MSG "READY!!!"
#define EMPTY_LINE " "
#define STATUS_GSM_NO_SIM "GSM : No SIM"
#define STATUS_GSM_OK     "GSM : OK"
#define STATUS_GSM_NOK    "GSM : NOK"
#define STATUS_WiFi_OK    "WiFi: OK"
#define STATUS_WiFi_NOK   "WiFi: NOK"
#define STATUS_NET_OK     "WWW : OK"
#define STATUS_NET_NOK    "WWW : NOK"
#define STATUS_COSM_OK    "COSM: OK"
#define STATUS_COSM_NOK   "COSM: NOK"
#define STATUS_GSM  1
#define STATUS_WiFi  2
#define STATUS_NET  3
#define STATUS_COSM  4

typedef enum{
  DispDummy,
  DispConnecting,
  DispError,
  DispReady,
  DispVending  
} 
e_DisplayStates;

int glcdStatus;
e_DisplayStates DispState;
e_DisplayStates DispStatePrev;
boolean updateDispStat;
const char* statusGsm ;
const char* statusWiFi ;
const char* statusNet ;
const char* statusCosm ;
Image_t icon;
gText textArea;              // a text area to be defined later in the sketch
gText textAreaArray[3];      // an array of text areas  
gText countdownArea =  gText(GLCD.CenterX, GLCD.CenterY,1,1,System5x7); // text area for countdown digits

void setup()
{
  //Initialize serial ports for communication.
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  Serial1.begin(9600);
#if (DEBUGGING == 1)
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
#endif
  loadConfigFromEEPROM();
  Serial.println(F("Starting SM5100B Communication..."));
  blinkTimeStamp = millis();
}

void loop()
{
  spashScreen(); 
  SM5100Reset(); //Reset the SM5100 module
  delay(1000);
  while(1){
    SM5100RxTask();
    SM5100StateMachine();
    //Blink();
    DispScreenUpdate();
    // print the string when a newline arrives:
    if (CmdComplete) {
      SM5100ParseStatus();
      BufferTail = 0;
      CmdComplete = false;
    }
  }

}


/*
 We are interested in two types of messages:
 1. SIND status commands - they are of the form +SIND: xx \r
 2. Auth Codes for the Bot - they are of the form #AC:xxxxx&
 So this task looks for either start bits -> + or # 
 When it receives either, it starts logging all the next bytes that are not spaces
 into the buffer. Once the end character - \r or # is received, it stops logging
 and notifies the application that a command was received.
 $ and # are selected for the authcode delimiters because the sms shield does not produce 
 them in normal operation.
 
 When you receive an SMS this is what the Arduino sees:
 +CMT: "+6596376447","+6596197777","13/02/12,00:36:55+00",11
 #AC:000001&
 First you have the sender Phone number, then the SMS gateway number then the date and time
 Finally the number of characters in the message (11), followed by the message itself on a new line.
 */
void SM5100RxTask(void) {
  while (Serial1.available() > 0) {
    // get the new byte:
    char inChar = (char)Serial1.read();
    Serial.print(inChar); // dump it out to the serial port
    // discard spaces and data in between commands
    if(SM5100State != SM5100WForSms){
      if((inChar != ' ')&& (CmdStart == true)){
        if ((inChar == '\n') || (inChar == '\r')) {
          CmdComplete = true;
          CmdStart = false;
        }
        else{
          SMBuffer[BufferTail++] = inChar; 
        }
      }
      if ( (inChar == '+')) {
        CmdStart = true;
      }
    }
    else{
      if((inChar != ' ')&& (CmdStart == true)){
        if ( (inChar == '&')) {
          CmdComplete = true;
          CmdStart = false;
        }
        else{
          SMBuffer[BufferTail++] = inChar; 
        }
      }
      if ((inChar == '#')) {
        CmdStart = true;
      }
    }

  }
}
byte stat = SM_DUMMY;

void SM5100ParseStatus(void)
// Parses out the status message and sets the State
//
{
  // Parse out SIND status updates

  Serial.println(F("Parsing Status..")); 
  if(strncmp(SMBuffer,"SIND:",5)==0) {
    if( isDigit(SMBuffer[5]) && isDigit(SMBuffer[6]) ) {
      char a[3] = {
        SMBuffer[5],SMBuffer[6],'\0'                                                            };
      stat = atoi(a);
      SMBuffer[5] = '\0';
      SMBuffer[6] = '\0';
    } 
    else if(isDigit(SMBuffer[5])) {
      stat = SMBuffer[5] - '0';
      SMBuffer[5] = '\0';
    }
    Serial.println(F("Status="));
    Serial.print(stat);
    Serial.println(F(" "));
  }

  switch(stat) // Change states of SIM module based on network status
  {
  case SM_SIM_REMOVED: 
    Serial.println(F("SM_SIM_REMOVED")); 
    SM5100State = SM5100Error; 
    errorTimeStamp = millis();
    break;
  case SM_SIM_INSERTED: 
    Serial.println(F("SM_SIM_INSERTED"));  
    break;
  case SM_RING_MELODY: 
    Serial.println(F("SM_RING_MELODY"));  
    break;
  case SM_CALL_READY: 
    Serial.println(F("SM_CALL_READY"));  
    break;
  case SM_SMS_READY: 
    Serial.println(F("SM_READY")); 
    SM5100State = SM5100Ready; 
    break;
  case SM_ID_RELEASED_CALLS: 
    Serial.println(F("SM_ID_RELEASED_CALLS"));  
    break;
  case SM_RELEASED_CALL_ID: 
    Serial.println(F("SM_RELEASED_CALL_ID"));  
    break;
  case SM_NETWORK_EMERGENCY: 
    Serial.println(F("SM_NETWORK_EMERGENCY")); 
    SM5100State = SM5100Error; 
    errorTimeStamp = millis();
    break;
  case SM_NETWORK_LOST: 
    Serial.println(F("SM_NETWORK_LOST")); 
    SM5100State = SM5100Error; 
    errorTimeStamp = millis();
    break;
  case SM_AUDIO_ON: 
    Serial.println(F("SM_AUDIO_ON")); 
    break;
  case SM_PHONEBOOK_STATUS_INIT: 
    Serial.println(F("SM_PHONEBOOK_STATUS_INIT"));  
    break;
  case SM_NETWORK_REGISTERED: 
    Serial.println(F("SM_NETWORK_REGISTERED"));  
    break;
  case SM_DUMMY: 
    break;
  default:  
    Serial.println(F("Unknown Status")); 
    break;

  }

}

// 
void SM5100StateMachine(void)
{

  switch(SM5100State)
  {
  case SM5100Init:
    // Do nothing, wait for the network to be initialized or errors to be reported
    break;

  case SM5100Ready:
    SM500InitializeSMS();
    break;

  case SM5100WForSms:
    if (CmdComplete) {
      SM5100ParseSms();
      BufferTail = 0;
      CmdComplete = false;
    }
    break;

  case SM5100Error:
    if( ((millis() - errorTimeStamp) & 0xFFFFFFFF )>=  C_ERROR_TIMEOUT){
      SM5100State = SM5100Ready; // Attempt to restart..
      errorTimeStamp = millis();
      Serial.println(F("State:ERROR, trying to restart"));
      DispState = DispError;
      if(stat != SM_SIM_REMOVED)
        updateDispStatus(STATUS_GSM,0);
      else
        updateDispStatus(STATUS_GSM,3); // No SIM
    }
    break;

  }


}

void SM500InitializeSMS(void)
{
  // Set text mode
  Serial.println(F("Initial Config for SIM"));
  while (Serial1.available() > 0) Serial1.read(); //clear bytes in Rx buffer -> flush() only clears tx buffer
  Serial1.println(F("AT+CMGF=1"));
  Serial.setTimeout(10000);
  Serial1.readBytesUntil('\n', SMBuffer, 15);
  if(strncmp(SMBuffer,"OK",2)==0){
    // OK from shield 
    Serial.println(F("Text Mode"));
    Serial1.println(F("AT+CNMI=3,3,0,0"));
    delay(3000); // Let it process the command
    Serial.println(F("Deleting Messages in SIM"));
    Serial1.println(F("AT+CMGD=1,4"));
    Serial.println(F("Waiting for SMS"));
    while (Serial1.available() > 0) Serial1.read();
    SM5100State = SM5100WForSms;
    DispState = DispReady;
    updateDispStatus(STATUS_GSM,1);
  }
  // reset sind buffer
  SMBuffer[5] = '\0';
  SMBuffer[6] = '\0';
}

void SM5100Reset(void)
{
  while (Serial1.available() > 0) Serial1.read();
  Serial1.println(F("AT+CFUN=1,1"));  // reset the SMB and get the SIND
  Serial1.flush();

}

boolean compareArrays(char* array1, char* array2, int length)
{
  for(int i = 0; i < length; i++){
    if(array1[i] != array2[i]){
      return false;
    }
  } 
  return true;
}



void SM5100ParseSms(void)
{
  char temp;
  boolean approval = false;
  byte authCodeLen = 0;
  Serial.println(F("Parsing SMS.."));
  if(strncmp(SMBuffer,"AC:",C_AUTHCODE_HEADER_BYTES)==0) { // Make sure its the auth code
    // Auth code length can change (increase), so read until BufferTail
    // Compare values
    Serial.println(F("New ID ="));
    Serial.write((const uint8_t*)SMBuffer,BufferTail);
    Serial.println(F(" "));
    authCodeLen = BufferTail - C_AUTHCODE_HEADER_BYTES; // Now point to the 
    if(compareArrays((char*)LastApprovalId,(char*)&SMBuffer[C_AUTHCODE_HEADER_BYTES],authCodeLen) == false)
      approval = true;// values are different, approve beer

    if(approval){
      Serial.println(F("Beer Approved!, Vending.."));
      DispState = DispVending;
      DispScreenUpdate();
      //vendBeer(); -- > moved to display routine
      Serial.println(F("Writing ID into EEPROM"));
      // Store it into EEPROM
      writeToEEPROM(C_EEPROM_APPROVALID_ADDRESS, (char*)&SMBuffer[C_AUTHCODE_HEADER_BYTES], authCodeLen);
      writeToEEPROM(C_EEPROM_APPROVALID_SIZE_ADDRESS, (char*)&authCodeLen, 1);
      // Copying it into RAM for access in the current power cycle
      memcpy ( LastApprovalId, &SMBuffer[C_AUTHCODE_HEADER_BYTES], authCodeLen );
    }
    else{
      Serial.println(F("IDs are same, no beer for you"));

    }
  }
  else{
    Serial.println(F("Unknown Message:"));
    Serial.write((const uint8_t*)SMBuffer,BufferTail); 
    SM5100State = SM5100Error;
    setBlinkRate(3); // Status: Error
  }

}

boolean vendBeer(void)
{

  vendside = 1 - vendside;

  if (vendside == 1) {
    digitalWrite(right, HIGH);
    delay(500); //TODO - change these to 500
    digitalWrite(right, LOW);

  } 
  else {
    digitalWrite(left, HIGH);
    delay(500); //TODO - change these to 500
    digitalWrite(left, LOW);
  }
  return true;

}

void writeToEEPROM(int address, char* dataP, int len)
// Writes the data into EEPROM
// EEPROM write does not return anything..
{
  int i;
  for(i=0;i<len;i++)
  {
    EEPROM.write((address+i), *(dataP+i));
  } 
}


void readFromEEPROM(int address, char* dataP, int len)
// Reads the data from EEPROM
{
  int i;
  for(i=0;i<len;i++)
  {
    *(dataP+i) = EEPROM.read(address+i);
  } 
}

void loadConfigFromEEPROM(void)
// Loads all the configuration informaton from EEPROM To RAM for faster access later
{
  char temp;
  Serial.println(F("Reading cfg from EE"));


  Serial.println(F("Reading Last Approval ID from EEPROM"));
  readFromEEPROM(C_EEPROM_APPROVALID_SIZE_ADDRESS, (char*)&LastApprovalIdLen, 1);
  if(LastApprovalIdLen <= C_APPROVAL_ID_LEN)
  {
    readFromEEPROM( C_EEPROM_APPROVALID_ADDRESS, (char*)&LastApprovalId[0], LastApprovalIdLen ); 
    Serial.println(F("Old Approval ID:"));
    Serial.write((uint8_t*)&LastApprovalId[0],LastApprovalIdLen);
  }
  else
  {
    LastApprovalIdLen = 0; // reset the old length, case when EEPROM is uninitialized
    Serial.println("Reset Last Approval ID");
  } 

}

void setBlinkRate(byte value)
// The WiFi status and connect status cant be indiated by blinking because those functions dont return until success/fail
{
  switch(value)
  {

  case 0:
    blinkTimePeriod = 0xFFFFFFFF;
    break;

  case 3:
    blinkTimePeriod = C_ERROR_BLINK_RATE;
    break;

  default:
    break;
  } 

}

void Blink(void)
{
  if( ((millis() - blinkTimeStamp) & 0xFFFFFFFF )>=  blinkTimePeriod){
    digitalWrite( LED_PIN  , 1 ^ digitalRead(LED_PIN )) ;
    blinkTimeStamp = millis();
  }

}


void SM5100LoopBack(void)
{
  //If a character comes in from the cellular module...
  if(Serial1.available() >0){
    incoming_char=Serial1.read();    //Get the character from the cellular serial port.
    Serial.print(incoming_char);  //Print the incoming character to the terminal.
  }

  //If a character is coming from the terminal to the Arduino...
  if(Serial.available() >0){
    incoming_char=Serial.read();  //Get the character coming from the terminal
    Serial1.print(incoming_char);    //Send the character to the cellular module.
  }

}


// ----------- GLCD DISPLAY SECTION -----------------
void updateDispStatus(byte type, byte value)
// Call this to update the status message in Ready or Error States
{
  switch(type)
  {
  case STATUS_GSM:
    switch(value)
    {
    case 0: 
      statusGsm = STATUS_GSM_NOK; 
      break;
    case 1: 
      statusGsm = STATUS_GSM_OK; 
      break;
    case 3: 
      statusGsm = STATUS_GSM_NO_SIM; 
      break;
    }
    break;
  case STATUS_WiFi:
    switch(value)
    {
    case 0: 
      statusWiFi = STATUS_WiFi_NOK; 
      break;
    case 1: 
      statusWiFi = STATUS_WiFi_OK; 
      break;
    }
    break;

  case STATUS_NET:
    switch(value)
    {
    case 0: 
      statusNet = STATUS_NET_NOK; 
      break;
    case 1: 
      statusNet = STATUS_NET_OK; 
      break;
    }
    break;
  case STATUS_COSM:
    switch(value)
    {
    case 0: 
      statusCosm = STATUS_COSM_NOK; 
      break;
    case 1: 
      statusCosm = STATUS_COSM_OK; 
      break;
    }
    break;

  }
  updateDispStat = true;
}

void spashScreen(void)
{
  //  statusGsm = STATUS_GSM_NOK;
  //  statusWiFi = STATUS_WiFi_NOK;
  //  statusNet = STATUS_NET_NOK;
  //  statusCosm = STATUS_COSM_NOK;

  glcdStatus = GLCD.Init();   // initialise the library, non inverted writes pixels onto a clear screen
  if(glcdStatus) // did the initialization fail?
  {
    Serial.println("GLCD initialization Failed: ");
    Serial.println(" (status code: ");
    Serial.print(glcdStatus);
    Serial.println(')');
    return;
  }
  GLCD.ClearScreen(); 
  // Display title for some time
  GLCD.SelectFont(System5x7); // you can also make your own fonts, see playground for details   
  GLCD.CursorToXY(GLCD.Width/2 - 44, 3);
  GLCD.print("BeerBot version");
  GLCD.print(BEERBOT_VERSION, DEC);
  GLCD.DrawRoundRect(8,0,GLCD.Width-19,17, 5);  // rounded rectangle around text area   
  countdown(TIME_TEXT_DISP);  
  GLCD.ClearScreen(); 
  // Set up text area and welcome message
  textArea.DefineArea(GLCD.CenterX, 0, GLCD.Right, GLCD.Bottom, SCROLL_UP); 
  textArea.SelectFont(System5x7, BLACK);
  textArea.CursorTo(0,0); 
  textArea.println(EMPTY_LINE);
  textArea.println("Brought to you by");
  textArea.println(SPLASH_MSG1);
  textArea.println(SPLASH_MSG2);
  // start scribble
  scribble(TIME_SPLASH_DISP);  // run for 5 seconds  
  // move to status screen
  DispState = DispConnecting;
  DispStatePrev = DispDummy;
  updateDispStatus(STATUS_GSM,0);
  updateDispStatus(STATUS_WiFi,0);
  updateDispStatus(STATUS_COSM,0);
  updateDispStatus(STATUS_NET,0);
  DispScreenUpdate(); //Force the screen to update
}

void DispScreenUpdate(void)
// Call this periodically to update the screen based on events
{
  if(glcdStatus) 
    return;
  if(DispState != DispStatePrev){
    switch(DispState)
    {
    case DispConnecting:
      GLCD.ClearScreen();
      icon = sync;  
      GLCD.DrawBitmap(icon, 0,0); 
      textArea.CursorTo(0,0);
      textArea.println(EMPTY_LINE);
      textArea.print(CONNECT_MSG);
      break;

    case DispReady:
      GLCD.ClearScreen();
      icon = beer_icon;  
      GLCD.DrawBitmap(icon, 0,0); 
      break;

    case DispVending:
      GLCD.ClearScreen();
      GLCD.CursorToXY(GLCD.Width/2 - 44, 3);
      GLCD.print("   Vending In  ");
      GLCD.DrawRoundRect(8,0,GLCD.Width-19,17, 5);  // rounded rectangle around text area 
      countdown(TIME_TEXT_DISP); 
      GLCD.ClearScreen();
      icon = explosion;  
      GLCD.DrawBitmap(icon, 0,0);
      textArea.CursorTo(0,0);
      textArea.println(EMPTY_LINE);
      textArea.print(VEND_MSG);
      countdown(10);
      GLCD.ClearScreen();
      updateDispStatus(0,0); // Dummy update of status
      break;

    default:
      GLCD.ClearScreen();
      icon = error;  // the 32 pixel high icon
      GLCD.DrawBitmap(icon, 0,0); //draw the bitmap at the given x,y position

      break;

    }
    DispStatePrev = DispState;
  }

  if( (updateDispStat == true) && ((DispState == DispError) || (DispState == DispReady)) ){
    textArea.ClearArea();
    textArea.println(statusGsm);
    textArea.println(statusWiFi);
    textArea.println(statusNet);
    textArea.println(statusCosm);
    updateDispStat = false;
  }

  if(DispState == DispVending)
    DispState = DispReady;
}

void countdown(int count){
  while(count--){  // do countdown  
    countdownArea.ClearArea(); 
    countdownArea.print(count);
    delay(1000);  
  }  
}

/*
 * scribble drawing routine adapted from TellyMate scribble Video sketch
 * http://www.batsocks.co.uk/downloads/tms_scribble_001.pde
 */
void scribble( const unsigned int duration )
{
  const  float tick = 1/128.0;
  float g_head_pos = 0.0;

  for(unsigned long start = millis();  millis() - start < duration; )
  {
    g_head_pos += tick ;

    float head = g_head_pos ;
    float tail = head - (256 * tick) ;

    // set the pixels at the 'head' of the line...
    byte x = fn_x( head ) ;
    byte y = fn_y( head ) ;
    GLCD.SetDot( x , y , BLACK) ;

    // clear the pixel at the 'tail' of the line...
    x = fn_x( tail ) ;
    y = fn_y( tail ) ;  
    GLCD.SetDot( x , y , WHITE) ;
  }
}

byte fn_x( float tick )
{
  return (byte)(GLCD.Width/4 + (GLCD.Width/4-1) * sin( tick * 1.8 ) * cos( tick * 3.2 )) ;
}

byte fn_y( float tick )
{
  return (byte)(GLCD.Height/2 + (GLCD.Height/2 -1) * cos( tick * 1.2 ) * sin( tick * 3.1 )) ;
}






