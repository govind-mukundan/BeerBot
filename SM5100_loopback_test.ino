/*

Simple Loopback test for SMB5100 and Leonardo
@Govind


*/

char incoming_char;
#define DEBUGGING (1)  // make 0 before release

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
Serial1.println(F("AT+CFUN=1,1"));  // reset the SMB and get the SIND
}

void loop()
{
SM5100LoopBack();

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
