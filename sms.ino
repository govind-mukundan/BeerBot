/*

Description: SMS interface for Beerbot

Author: Govind Mukundan (govind.mukundan at gmail.com)

References:
1. GSM Shield  -> https://www.sparkfun.com/products/9607
2. Interfacing Tutorial -> http://tronixstuff.wordpress.com/tag/sm5100/

*/

#define F(x) x // Disable progmem strings for Due
#define C_ERROR_BLINK_RATE          (1000ul)
#define C_AUTHCODE_HEADER_BYTES    (3) // AC:
#define SM_BUFFER_LEN  100

#define C_ERROR_TIMEOUT    (90000) //1.5 min

typedef enum {
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

char incoming_char=0;      //Will hold the incoming character from the Serial Port.
char SMBuffer[SM_BUFFER_LEN];
char BufferTail;
char CmdStart = 0;
boolean CmdComplete = false;  // whether the string is complete
unsigned long errorTimeStamp = 0;
SM5100StatusCodes Sm5100stat = SM_NETWORK_REGISTERED;
extern char LastApprovalId[];

e_SM5100States SM5100State;

static boolean BootUp;

// ----------- SMS Shield -------------------------
void SMSInit(void)
{
  // Set the system to a known state.
  SM5100State = SM5100Init;
  BufferTail = 0;
  BootUp = true;  
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
  byte stat;
  Serial.println(F("Parsing Status..")); 
  if(strncmp(SMBuffer,"SIND:",5)==0) {
    if( isDigit(SMBuffer[5]) && isDigit(SMBuffer[6]) ) {
      char a[3] = {
        SMBuffer[5],SMBuffer[6],'\0'                                                                  };
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

  if(BootUp == true){
    SM5100Reset(); 
    BootUp = false;
  }

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
      SM5100Reset();
      GLCDNotifyError();
      if(Sm5100stat != SM_SIM_REMOVED)
        updateDispStatus(STATUS_GSM,0);
      else
        updateDispStatus(STATUS_GSM,3); // No SIM
    }
    break;

  }
  if (CmdComplete) {
    SM5100ParseStatus();
    BufferTail = 0;
    CmdComplete = false;
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
  Serial.println(F("1"));
  //if(strncmp(SMBuffer,"OK",2)==0){
  // OK from shield 
  Serial.println(F("Text Mode"));
  Serial1.println(F("AT+CNMI=3,3,0,0"));
  delay(3000); // Let it process the command
  Serial.println(F("Deleting Messages in SIM"));
  Serial1.println(F("AT+CMGD=1,4"));
  Serial.println(F("Waiting for SMS"));
  while (Serial1.available() > 0) Serial1.read();
  SM5100State = SM5100WForSms;
  updateDispStatus(STATUS_GSM,1);
  //}
  // reset sind buffer
  SMBuffer[5] = '\0';
  SMBuffer[6] = '\0';
}

void SM5100Reset(void)
{
  Serial.println(F("Resetting SMB 5100"));
  while (Serial1.available() > 0) Serial1.read();
  Serial1.println(F("AT+CFUN=1,1"));  // reset the SMB and get the SIND
  Serial1.flush();

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
      Serial.println("Beer Approved!, Vending..");
      GLCDVendBeer();
      Serial.println("Writing ID into SD Card");
      // Store it into SD
      writeToAuthSD((uint8_t*)&SMBuffer[C_AUTHCODE_HEADER_BYTES],authCodeLen);
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


