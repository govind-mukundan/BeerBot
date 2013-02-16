
/*
                      Description: INO for beerbot task #1 in Basecamp.
                      Hardware: Arduino Leonardo
		      Shields: WiFi Shield for Arduino	
                      Author: Govind Mukundan (govind.mukundan at gmail.com)
                      Date: 19/Jan/2013
                      Version: 1.1
                      References: MQTT -> Library from http://knolleary.net/arduino-client-for-mqtt/
                      History:
                                1.1: First Version, tested at home without Bot
 
 */
#include <SD.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

#define DEBUGGING (0)  // make 0 before release
// GPIO defs
#define SD_CS_PIN    (4) // SD SS is pin 4 on the wifi sheild
#define WiFi_CS_PIN    (10) 
#define LED_PIN      (13) // On board LED at pin13 (L)
#define LED_INTERNET_ON  (12)

#define C_APPROVAL_ID_LEN    (20)
#define API_KEY "8dlRt66vQY2YrpGuwOM97cC5rsSSAKxpL0RvaUtZYkFSYz0g" // beerbot Cosm API key
#define FEED_ID 82941 // beerbot Cosm feed ID
#define COSM_SUBSCRIBE_STRING  "/v1/feeds/82941/datastreams/0.csv"
#define C_INITIAL_FEEDS_TO_IGNORE  (0)
#define C_SERVER "api.cosm.com"
#define C_CosmON_BLINK_RATE          (1000ul)
#define C_SSID_MAX_LEN      (20)
#define C_NWKEY_MAX_LEN      (30)


#define C_EEPROM_SSID_ADDRESS                  (0x00)
#define C_EEPROM_SSID_SIZE_ADDRESS             (0x00 + 29u)
#define C_EEPROM_NWKEY_ADDRESS                 (0x00 + 30u)
#define C_EEPROM_NWKEY_SIZE_ADDRESS            (0x00 + 59u)
#define C_EEPROM_APPROVALID_ADDRESS            (0x100)
#define C_EEPROM_APPROVALID_SIZE_ADDRESS       (0x100 + 50u)

typedef enum{
  WiFiDisconnected,
  WiFiConnected
} 
WiFiStates;


typedef enum{
  CosmDisconnected,
  CosmConnected,
  CosmSubscribed
}
CosmStates;

// State Variables
WiFiStates WiFiState;
CosmStates CosmState;
byte IgnoreFeedCount;
File myFile;
// Hardware
int left = 12;
int right = 13;
int vendside = 0;
// EEPROM copy
byte LastApprovalIdLen;
char LastApprovalId[C_APPROVAL_ID_LEN];
char ssid[C_SSID_MAX_LEN];
char key[C_NWKEY_MAX_LEN]; 
unsigned long blinkTimeStamp = 0;                // Time Stamp
unsigned long blinkTimePeriod = 0;

WiFiClient wifiClient;
//Connect to test.mosquitto.org server
//byte server [] = { 85, 119, 83, 194 };
// Connect to api.cosm.com
//byte server [] = { 64, 94, 18, 121 }; // 	64.94.18.121
//char server[]="api.cosm.com"; 
// Update these with values suitable for your network.
//byte mac[6] ; //   = {  0x7A, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
//IPAddress ip;
//char ssid[] = "MYH27503";                     // your network SSID (name)
//char key[] = "9183061503";       // your network key
int keyIndex = 0;                                // your network key Index number
int status = WL_IDLE_STATUS;                     // the Wifi radio's status


// APIs
void beerBotInit(void);
void BeerBotTask(void);
void callback(char* topic, byte* payload, unsigned int length);
boolean vendBeer(void);
void printWifiStatus(void);
void loadConfigFromEEPROM(void);
void readFromEEPROM(int address, char* dataP, int len);
void writeToEEPROM(int address, char* dataP, int len);
boolean compareArrays(char* array1, char* array2, int length);
void setBlinkRate(byte value);
void Blink(void);
PubSubClient client(C_SERVER, 1883, callback, wifiClient); // MQTT client


void setup() {
  beerBotInit();

}

void loop() {

  BeerBotTask();
  Blink();
  // Update status
  if (WiFi.status() != WL_CONNECTED){
    WiFiState = WiFiDisconnected;
  }
  if((CosmState != CosmDisconnected) && (!client.connected())){
    CosmState = CosmDisconnected;
    Serial.println("Error:Cosm>Disconn");
  }

}

void beerBotInit(void)
{

  pinMode(left, OUTPUT); //left
  pinMode(right, OUTPUT); //right
  pinMode(LED_PIN, OUTPUT);
  pinMode( LED_INTERNET_ON, OUTPUT);
  // Initially both are low
  digitalWrite( LED_PIN , LOW) ;
  digitalWrite( LED_INTERNET_ON , LOW) ;

  WiFiState = WiFiDisconnected;
  CosmState = CosmDisconnected;

  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(5000); // A 5s delay 
  #if (DEBUGGING == 1)
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  #endif
  Serial.println("Starting BREERBOT");

  // First check if WiFi shield is present, if its not there then there is nothing to do..
  if (WiFi.status() == WL_NO_SHIELD) {
    // Serial.println("WiFi shield not present"); 
    // don't continue:
    while(true);
  } 
  // The SD card access should be done before turning on the WiFi module..
  loadConfigFromEEPROM();
  blinkTimeStamp = millis();
}

void BeerBotTask(void)
{
  switch(WiFiState)
  {
  case WiFiDisconnected:
    // Exit this state only when a connection is established
    Serial.println("Init network");
    // Status that WiFi is connecting
    digitalWrite( LED_PIN , HIGH) ;
    digitalWrite( LED_INTERNET_ON , LOW) ;
    setBlinkRate(0);
    status = WiFi.begin(ssid, keyIndex, key);
    while ( (status != WL_CONNECTED)) {
      // wait 10 seconds for connection:
      delay(10000);
      Serial.print("Connecting to");
      Serial.println(ssid);
      status = WiFi.begin(ssid, keyIndex, key);
    }
    // Status: WiFi COnnected
    digitalWrite( LED_PIN , HIGH) ;
    digitalWrite( LED_INTERNET_ON , HIGH) ;
    WiFiState = WiFiConnected;
    CosmState = CosmDisconnected;
    Serial.println("Connected to wifi");
    printWifiStatus();
    break;

  case WiFiConnected: 

    switch(CosmState)
    {
    case CosmDisconnected:
      Serial.println("Connecting to cosm");
      setBlinkRate(0); // Status: Cosm COnnected
      digitalWrite( LED_PIN , HIGH) ;
      digitalWrite( LED_INTERNET_ON , HIGH) ;
      if (client.connect("GovindsBot", API_KEY, "1")) {
        Serial.println("Connected to api.cosm.com");
        CosmState = CosmConnected;
      }
      break;

    case CosmConnected:

      Serial.println("Subs to feed");
      if(true== client.subscribe(COSM_SUBSCRIBE_STRING)){
        
        CosmState = CosmSubscribed;
        Serial.print("OK");
        Serial.println(COSM_SUBSCRIBE_STRING);
        IgnoreFeedCount = 0; // reset the ignore counter
        setBlinkRate(3); // Status: Cosm COnnected
        digitalWrite( LED_PIN , HIGH) ;
        digitalWrite( LED_INTERNET_ON , LOW) ;
      }
      else{
        Serial.println("Error Subs to feed");
      }
      break;

    case CosmSubscribed:
      // Nonthing to do here, just call the loop task
      if(!client.loop()){
        CosmState = CosmDisconnected;
        Serial.println("Error:Cosm>Disconn");
      }
      break;

    default:
      break;

    }
    break;

  default:
    break;

  }

}

// Callback from the MQTT library
void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  //Serial.println("Data recenved on Feed");
  Serial.println(topic);
  Serial.println("RxLength:");
  Serial.print(length);
  Serial.println("RxData:");
  Serial.write(payload,length);
  Serial.println(" ");

  // If the new approval ID is not the same as the old one, write it into EEPROM and vend a beer
  if(IgnoreFeedCount < C_INITIAL_FEEDS_TO_IGNORE){
    Serial.println("Ignore first 2 msg");
    IgnoreFeedCount++;
    return;
  }

  boolean approval = false;
  if((length <= C_APPROVAL_ID_LEN) && (LastApprovalIdLen <= length)) // Epoch is always increasing, new length must be greater than or equal to old length
  {
    // Compare values
    if(compareArrays((char*)LastApprovalId,(char*)payload,length) == false)
      approval = true;// values are different, approve beer

    if(approval){
      Serial.println("Beer Approved!, Vending..");
      vendBeer();
      Serial.println("Writing ID into EEPROM");
      // Store it into EEPROM
      writeToEEPROM(C_EEPROM_APPROVALID_ADDRESS, (char*)payload, length);
      writeToEEPROM(C_EEPROM_APPROVALID_SIZE_ADDRESS, (char*)&length, 1);
      // Copying it into RAM for access in the current power cycle
      memcpy ( LastApprovalId, payload, length );
      client.publish("8dlRt66vQY2YrpGuwOM97cC5rsSSAKxpL0RvaUtZYkFSYz0g/v2/feeds/82941.csv","2,1");
    }
    else{
      Serial.println("IDs are same, no beer for you");
      client.publish("8dlRt66vQY2YrpGuwOM97cC5rsSSAKxpL0RvaUtZYkFSYz0g/v2/feeds/82941.csv","2,0");
    }
  } 
  else{
    Serial.println("ERROR>EpochTime");
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


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("RSSI:");
  Serial.print(rssi);
  Serial.println(" dBm");
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
  Serial.println("Reading cfg from EE");

  readFromEEPROM(C_EEPROM_SSID_SIZE_ADDRESS, &temp, 1);
  if(temp <= C_SSID_MAX_LEN)
  {
    readFromEEPROM( C_EEPROM_SSID_ADDRESS, ssid, temp ); 
    Serial.println(" SSID:");
    Serial.write((uint8_t*)ssid,temp);
  }
  readFromEEPROM(C_EEPROM_NWKEY_SIZE_ADDRESS, &temp, 1);
  if(temp <= C_NWKEY_MAX_LEN)
  {
    readFromEEPROM( C_EEPROM_NWKEY_ADDRESS, key, temp ); 
    Serial.println(" KEY:");
    Serial.write((uint8_t*)key,temp);
  }

  Serial.println("Reading Last Approval ID from EEPROM");
  readFromEEPROM(C_EEPROM_APPROVALID_SIZE_ADDRESS, (char*)&LastApprovalIdLen, 1);
  if(LastApprovalIdLen <= C_APPROVAL_ID_LEN)
  {
    readFromEEPROM( C_EEPROM_APPROVALID_ADDRESS, (char*)&LastApprovalId[0], LastApprovalIdLen ); 
    Serial.println("Old Approval ID:");
    Serial.write((uint8_t*)&LastApprovalId[0],LastApprovalIdLen);
  }
  else
  {
    LastApprovalIdLen = 0; // reset the old length, case when EEPROM is uninitialized
    Serial.println("Reset Last Approval ID");
  } 
  LoadWiFiSSID();
}


void LoadWiFiSSID(void)
// Can be optimized a bit by reloading EEPROM contents if they were changed..
{
  char ssidt[30];
  char siz;
  int i = 0;
  byte temp = 0;
  Serial.println(freeRam()); 

  // Disable wifi SS
  pinMode(WiFi_CS_PIN, OUTPUT);
  digitalWrite(WiFi_CS_PIN, HIGH);
  if (SD.begin(SD_CS_PIN)) {
    //Serial.println(" SD initialization failed!");
    Serial.println("SD init done.");
    Serial.println(freeRam()); 
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    myFile = SD.open("ssid.txt");
    if (myFile) {
      Serial.println("ssid:");

      // read from the file until there's nothing else in it:
      while ((myFile.available()) && ( i<C_SSID_MAX_LEN)) {
        temp = myFile.read();
        Serial.write(temp);
        ssidt[i++] = temp;
      }
      // close the file:
      myFile.close();
      // Compare with the value read from EEPROM and write it back if values are different OR if sizes are different
      readFromEEPROM(C_EEPROM_SSID_SIZE_ADDRESS, &siz, 1);
      if( (false == compareArrays(ssidt, ssid, i)) || (siz != i) ){
        Serial.println("SSID is new, writing to EEPROM..");
        writeToEEPROM(C_EEPROM_SSID_ADDRESS, (char*)ssidt, i);
        writeToEEPROM(C_EEPROM_SSID_SIZE_ADDRESS, (char*)&i, 1); 
        // Clear the old key in RAM
        for(temp = 0; temp<i; temp++)
          ssid[temp] = '\0';
        // Copy new ssid to Ram
        memcpy ( ssid, ssidt, i );
      }

    }
    else {
      // if the file didn't open, print an error:
      Serial.println("error opening ssid.txt");
    }
    i = 0;
    myFile = SD.open("pwd.txt");
    if (myFile) {
      Serial.println("pwd:");
      // read from the file until there's nothing else in it:
      while ((myFile.available()) && ( i<C_NWKEY_MAX_LEN)) {
        temp = myFile.read();
        Serial.write(temp);
        ssidt[i++] = temp;
      }
      // close the file:
      myFile.close();
      // Compare with the value read from EEPROM and write it back if values are different
      readFromEEPROM(C_EEPROM_NWKEY_SIZE_ADDRESS, &siz, 1);
      if( (false == compareArrays(ssidt, key, i)) || (siz != i) ){
        Serial.println("SSID is new, writing to EEPROM..");
        writeToEEPROM(C_EEPROM_NWKEY_ADDRESS, (char*)ssidt, i);
        writeToEEPROM(C_EEPROM_NWKEY_SIZE_ADDRESS, (char*)&i, 1); 
        // Clear the old key in RAM
        for(temp = 0; temp<i; temp++)
          key[temp] = '\0';
        // Copy new key to Ram
        memcpy ( key, ssidt, i );
      }
    } 
    else {
      // if the file didn't open, print an error:
      Serial.println("error opening pwd.txt");
    }

  } 
  else {
    // if the file didn't open, print an error:
    Serial.println("SD Card is not available..");
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

void Blink(void)
{
  if( ((millis() - blinkTimeStamp) & 0xFFFFFFFF )>=  blinkTimePeriod){
    digitalWrite( LED_INTERNET_ON  , 1 ^ digitalRead(LED_INTERNET_ON )) ;
    blinkTimeStamp = millis();
  }

}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}



