/*
Hardware Notes:
 --> Power supply should be at least 5V, 2.5Ampere and must be given to the GSM Shield, NOT to the Arduino
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
 
 
 
 ToDo:
 1. Auto Connect and initialize SMS Shield.
 2. Indicate that ready to receive or Error
 3. Once authorization SMS is received, vend beer
 
 
 http://playground.arduino.cc/Main/Printf
 
 */


#include <string.h>         //Used for string manipulations
#include <ctype.h>

#define SM_BUFFER_LEN  100
#define DEBUGGING (1)  // make 0 before release


char incoming_char=0;      //Will hold the incoming character from the Serial Port.
//String inputString = "";         // a string to hold incoming data
char SMBuffer[SM_BUFFER_LEN];
char BufferTail;
char CmdStart = 0;
boolean CmdComplete = false;  // whether the string is complete

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

void setup()
{
  //Initialize serial ports for communication.
  Serial.begin(9600);
  Serial1.begin(9600);
  #if (DEBUGGING == 1)
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  #endif
  Serial.println(F("Starting SM5100B Communication..."));

}

void loop()
{
  SM5100RxTask();
  SM5100StateMachine();
  // print the string when a newline arrives:
  if (CmdComplete) {
    SM5100ParseStatus();
    BufferTail = 0;
    CmdComplete = false;
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


/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.
 */
void SM5100RxTask() {
  while (Serial1.available()) {
    // get the new byte:
    char inChar = (char)Serial1.read();
    Serial.print(inChar); // dump it out to the serial port
    // discard spaces and data in between commands
    if((inChar != ' ')&& (CmdStart == true)){
      if ((inChar == '\n') || (inChar == '\r')) {
        CmdComplete = true;
        CmdStart = false;
      }
      else{
        SMBuffer[BufferTail++] = inChar; 
      }
    }
    if ( (inChar == '+') || () ) {
      CmdStart = true;
    }

  }
}

void SM5100ParseStatus(void)
// Parses out the status message and sets the State
//
{
  // Parse out SIND status updates
  byte stat = SM_DUMMY;
  if(strncmp(SMBuffer,"SIND:",5)==0) {
    if( isDigit(SMBuffer[5]) && isDigit(SMBuffer[6]) ) { //(BufferTail >= 7) && (
      char a[3] = {
        SMBuffer[5],SMBuffer[6],'\0'            };
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

  switch(stat)
  {
  case SM_SIM_REMOVED: 
    Serial.println(F("SM_SIM_REMOVED")); 
    SM5100State = SM5100Error; 
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
    break;
  case SM_NETWORK_LOST: 
    Serial.println(F("SM_NETWORK_LOST")); 
    SM5100State = SM5100Error; 
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
      Serial.println(F("Waiting for SMS"));
      SM5100State = SM5100WForSms;
    }
  // reset sind buffer
  SMBuffer[5] = '\0';
  SMBuffer[6] = '\0';
}


void SM5100ParseSms(void)
{
    if(strncmp(SMBuffer[0]=='#') {

    Serial.println(F("Auth="));
    Serial.print(stat);
    Serial.println(F(" "));
  }
  
}




