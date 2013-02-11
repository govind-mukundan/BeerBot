/*

                      Description: INO for beerbot task #2 in Basecamp.
                      Hardware: Arduino Leonardo
                      Author: Govind Mukundan (govind.mukundan at gmail.com)
                      Date: 12/Feb/2013
                      Version: 1.1
                      References: Basic intro to SM5100 -> http://tronixstuff.wordpress.com/tag/sm5100/
                      History:
                                1.1: First Version, tested at home without Bot using SINGTEL SIM card

 Hardware Notes:
 --> Power supply should be at least 5V, 2.5Ampere and must be given to the GSM Shield, NOT to the Arduino power jack.
 --> Ardiono should be powered from the 5V pin on the shield
 --> The Leonardo uses the USB port as the serial port, so pins (0,1) are free to use for the GSM shield (Serial1).
 --> However, the SMB Shield is by default configured to use pins (2,3) for UART, So you have to switch the Rx and Tx Jumpers on the board to use Serial1
 Once thats done, the shield should work directly
 
 AT Command Notes:
 --- Frequency band -----
 Singtel uses GSM Band 900, This can be configured using:
 AT+SBAND=0\r\n
 Starhub aparrently uses GSM 1800
 AT+SBAND=9\r\n shoudl cover both??
 --- Status -----
 After powering on the Arduino with the shield installed, verify that the module reads and recognizes the SIM card.
 With a terimal window open and set to Arduino port and 9600 buad, power on the Arduino. The startup sequence should look something
 like this:
 
 Starting SM5100B Communication...
 
 +SIND: 1
 +SIND: 10,"SM",1,"FD",1,"LD",1,"MC",1,"RC",1,"ME",1
 
 Communication with the module starts after the first line is displayed. The second line of communication, +SIND: 10, tells us if the module
 can see a SIM card. If the SIM card is detected every other field is a 1; if the SIM card is not detected every other field is a 0.
 
 3.) Wait for a network connection before you start sending commands. After the +SIND: 10 response the module will automatically start trying
 to connect to a network. Wait until you receive the following repsones:
 
 +SIND: 11
 +SIND: 3
 +SIND: 4
 
 The +SIND response from the cellular module tells the the modules status. Here's a quick run-down of the response meanings:
 0 SIM card removed
 1 SIM card inserted
 2 Ring melody
 3 AT module is partially ready
 4 AT module is totally ready
 5 ID of released calls
 6 Released call whose ID=<idx>
 7 The network service is available for an emergency call
 8 The network is lost
 9 Audio ON
 10 Show the status of each phonebook after init phrase
 11 Registered to network
 
 After registering on the network you can begin interaction. Here are a few simple and useful commands to get started:
 
 To make a call:
 AT command - ATDxxxyyyzzzz
 Phone number with the format: (xxx)yyy-zzz
 
 If you make a phone call make sure to reference the devices datasheet to hook up a microphone and speaker to the shield.
 
 To send a txt message:
 AT command - AT+CMGF=1
 This command sets the text message mode to 'text.'
 AT command = AT+CMGS="xxxyyyzzzz"(carriage return)'Text to send'(CTRL+Z)
 This command is slightly confusing to describe. The phone number, in the format (xxx)yyy-zzzz goes inside double quotations. Press 'enter' after closing the quotations.
 Next enter the text to be send. End the AT command by sending CTRL+Z (0x1A in ASCII)
 
 --- Receiving SMS --------
 
 AT+CMGF=1
 Which sets the SMS mode to text. The second command is:
 
 AT+CNMI=3,3,0,0
 
 This command tells the GSM module to immediately send any new SMS data to the serial out.
 
 AT+CMGD=1,4 --> Deletes ALL SMSes
 
 --- Read All/New SMS ---
 AT+CMGF=1
 AT+CMGL="ALL"
 
---- Sent.ly API -----
To send an SMS via Sent.ly using HTTP,
1. Create an account
2. Register an android phone
3. Log into the phone app
4. Use the foll HTTP GET syntax 
https://sent.ly/command/sendsms?username=sent_ly_uname&password=sent_ly_pwd&to=%2b6596376447&text=%23AC:1360604509%26

--> The sender number will be the android phone runnung sent.ly
--> %23AC:1360604509%26, 0x23 and 0x26 are # and & in ASCII (URL Encoding) 
--> Note that the App on the phone MUST be running in order for the SMSes to be sent, if not you will get an error code "ERROR 3" on the browser
 
 http://playground.arduino.cc/Main/Printf
 
 */


#include <string.h>         //Used for string manipulations
#include <ctype.h>
#include <EEPROM.h>

#define SM_BUFFER_LEN  100
#define DEBUGGING (1)  // make 0 before release

#define C_EEPROM_APPROVALID_ADDRESS            (0x100)
#define C_EEPROM_APPROVALID_SIZE_ADDRESS       (0x100 + 50u)

#define C_APPROVAL_ID_LEN    (20)
#define LED_PIN      (8) // On board LED at pin13 (L)
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
  SM5100RxTask();
  SM5100StateMachine();
  Blink();
  // print the string when a newline arrives:
  if (CmdComplete) {
    SM5100ParseStatus();
    BufferTail = 0;
    CmdComplete = false;
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

void SM5100ParseStatus(void)
// Parses out the status message and sets the State
//
{
  // Parse out SIND status updates
  byte stat = SM_DUMMY;
  Serial.println(F("Parsing Status..")); 
  if(strncmp(SMBuffer,"SIND:",5)==0) {
    if( isDigit(SMBuffer[5]) && isDigit(SMBuffer[6]) ) {
      char a[3] = {
        SMBuffer[5],SMBuffer[6],'\0'                              };
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
    }
    break;

  }


}

void SM500InitializeSMS(void)
{
  // Set text mode
  Serial1.flush();
  Serial1.println(F("AT+CMGF=1"));
  Serial.setTimeout(10000);
  Serial1.readBytesUntil('\n', SMBuffer, 15);
  if(strncmp(SMBuffer,"OK",2)==0){
    // OK from shield 
    Serial.println(F("Text Mode"));
    Serial1.flush();
    Serial1.println(F("AT+CNMI=3,3,0,0"));
    delay(3000); // Let it process the command
    Serial.println(F("Deleting Messages in SIM"));
    Serial1.println(F("AT+CMGD=1,4"));
    Serial.println(F("Waiting for SMS"));
    SM5100State = SM5100WForSms;
  }
  // reset sind buffer
  SMBuffer[5] = '\0';
  SMBuffer[6] = '\0';
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
      vendBeer();
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


