/*
 Description: Beerbot.
 Hardware: Arduino Due
 Shields: WiFi Shield for Arduino, GSM Shield, Adafruit GLCD display	
 Author: Govind Mukundan (govind.mukundan at gmail.com)
 Date: 8/Jun/2013
 Version: 2.1
 References: See individual file 
 Installation Instructions:
 1. Write the SSID and PWD of the wifi n/w into two .txt files named ssid.txt and pwd.txt on to a micro SD card
 2. Place this sd card in the WiFi shiled.
 3. Load the sim card onto the gsm shield and mount both shields on Due
 4. Connect the GLCD
 5. Connect a 5V 2A+ supply to the GSM shield, tap the 3.3V out from the shield and connect to GLCD
 6. Turn ON and enjoy your beer. 
 7. To view log messages, connect the programming port of the Due to a terminal monitor. Baud rate is 9600bps.
 History:
 
 2.1
 Integrated GLCD with Due, and tested with the vending machine.
 
 2.0
 Port to Arduino Due
 Due does not have an internal EEPROM, so use the SD card on the WiFi shield instead.
 Note that you have to short pin 7 and 3 for the WiFi shield to work
 
 1.2: 
 Now supports WEP/WPA/OPEN networks.
 Will ignore \r and \n characters on the ssid.txt and pwd.txt
 Disabled Writes in SD library via switch GOVIND_FILE_WRITE_ON to increase code space
 Fixed bug where vend pins were same as LED pins
 
 1.1: 
 First Version, tested at home without Bot
 
 */

// Hardware
int left = 22;
int right = 23;
int vendside = 0;
#define LED_PIN      (21) // On board LED at pin13 (L)

unsigned long blinkTimeStamp = 0;                // Time Stamp
unsigned long blinkTimePeriod = 0;
#define C_CosmON_BLINK_RATE          (1000ul)
#define LED_INTERNET_ON  (13)


void setup() {
  // put your setup code here, to run once:
  GLCDInit(); // Turn on the backlight first
  Serial.begin(9600);
  Serial1.begin(9600);
  delay(1000); // A 5s delay
  Serial.println("Starting BEERRBOT");

  beerBotInit();
  SMSInit();
  digitalWrite(right, LOW);
  digitalWrite(left, LOW);
  pinMode(left, OUTPUT); //left
  pinMode(right, OUTPUT); //right
  digitalWrite(right, LOW);
  digitalWrite(left, LOW);
  blinkTimeStamp = millis();
}

void loop() {

  while(1)
  {
    GLCDTask();
    BeerBotTask();
    Blink();

    // SMS Shield
    SM5100RxTask();
    SM5100StateMachine();
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
    blinkTimePeriod = C_CosmON_BLINK_RATE;
    break;

  default:
    break;
  } 

}

byte state;
void Blink(void)
{
  if( ((millis() - blinkTimeStamp) & 0xFFFFFFFF )>=  blinkTimePeriod){
    digitalWrite( LED_INTERNET_ON  , state++ %2) ;
    blinkTimeStamp = millis();
  }

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

boolean vendBeer(void)
{
  Serial.println("Vending..");

  if (vendside % 2) {
    digitalWrite(right, HIGH);
    delay(500); 
    digitalWrite(right, LOW);

  } 
  else {
    digitalWrite(left, HIGH);
    delay(500); 
    digitalWrite(left, LOW);
  }
  vendside++;
  return true;

}



