/*
Description: WiFi and Cosm interfacing using MQTT
 
 Author: Govind Mukundan (govind.mukundan at gmail.com)
 
 References:
 1. MQTT Library -> http://knolleary.net/arduino-client-for-mqtt/
 2. WiFi Shield  -> http://arduino.cc/en/Main/ArduinoWiFiShield
 3. COSM -> https://xively.com/
 4. To update/subscribe to COSM feeds from a PC install mosquitto -> http://mosquitto.org/
 eg: to publish:
 mosquitto_pub -h api.cosm.com -u 8dlRt66vQY2YrpGuwOM97cC5rsSSAKxpL0RvaUtZYkFSYz0g -t /v2/feeds/82941.csv -m "0,1358689826"
 
 */


#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <PubSubClient.h>


#define F(x) x // Disable progmem strings for Due
#define DEBUGGING (1)  // make 0 before release
// GPIO defs
#define SD_CS_PIN    (4) // SD SS is pin 4 on the wifi sheild
#define WiFi_CS_PIN    (10) 

#define C_APPROVAL_ID_LEN    (30)
#define API_KEY "8dlRt66vQY2YrpGuwOM97cC5rsSSAKxpL0RvaUtZYkFSYz0g" // beerbot Cosm API key
#define FEED_ID 82941 // beerbot Cosm feed ID
#define COSM_SUBSCRIBE_STRING  "/v1/feeds/82941/datastreams/0.csv" //You need to mention your channel here
#define C_RESPONSE_FEED    "8dlRt66vQY2YrpGuwOM97cC5rsSSAKxpL0RvaUtZYkFSYz0g/v2/feeds/82941.csv" //reply on this stream
#define C_RESPONSE_OK       "2,PASS" //channel.value
#define C_RESPONSE_NOK      "2,FAIL"
#define C_INITIAL_FEEDS_TO_IGNORE  (0)
#define C_SERVER "api.xively.com"
#define C_SSID_MAX_LEN      (40)
#define C_NWKEY_MAX_LEN      (40)
#define C_AUTH_FILE_NAME    "auth.txt"


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
boolean IgnoreFeed;
File myFile;

// RAM copy
char LastApprovalId[C_APPROVAL_ID_LEN];
char ssid[C_SSID_MAX_LEN];
char key[C_NWKEY_MAX_LEN]; 

WiFiClient wifiClient;
//Connect to test.mosquitto.org server
//byte server [] = { 85, 119, 83, 194 };
// Connect to api.cosm.com
//byte server [] = { 64, 94, 18, 121 }; // 	64.94.18.121
//char server[]="api.cosm.com"; 
// Update these with values suitable for your network.
//byte mac[6] ; //   = {  0x7A, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
//IPAddress ip;
int keyIndex = 0;                                // your network key Index number
int status = WL_IDLE_STATUS;                     // the Wifi radio's status


// Due to some unknown bug in Cosm?? the arduino does not receive data when Cosm feed is updated, except on reconnect.
// Update: Seems like migrating to Xively has fixed this and the double update issue (phew!)
#define FORCE_RECONNECT_TO_COSM  0
unsigned long lastConnectionTime = 0;                // last time we connected to Cosm
const unsigned long connectionInterval = 60000;   // reconnect every 1min

// APIs
PubSubClient client(C_SERVER, 1883, callback, wifiClient); // MQTT client


void beerBotInit(void)
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_INTERNET_ON, OUTPUT);
  // Initially both are low
  digitalWrite( LED_PIN , LOW) ;
  digitalWrite( LED_INTERNET_ON , LOW) ;

  WiFiState = WiFiDisconnected;
  CosmState = CosmDisconnected;

  // First check if WiFi shield is present, if its not there then there is nothing to do..
  if (WiFi.status() == WL_NO_SHIELD) {
    // Serial.println("WiFi shield not present"); 
    // don't continue:
    Serial.println("ERROR:No WiFi Shield");
    while(true);
  } 
  // The SD card access should be done before turning on the WiFi module..
  loadConfigFromSD();

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
    updateDispStatus(STATUS_WiFi,0);
    updateDispStatus(STATUS_NET,0);
    updateDispStatus(STATUS_COSM,0);
    WiFiConnectToNetwork();
    while ( (status != WL_CONNECTED)) {
      // wait 10 seconds for connection:
      delay(10000);
      Serial.print("Connecting to");
      Serial.println(ssid);
      // status = WiFi.begin(ssid, keyIndex, key);
      WiFiConnectToNetwork();
    }
    // Status: WiFi COnnected
    digitalWrite( LED_PIN , HIGH) ;
    digitalWrite( LED_INTERNET_ON , HIGH) ;
    updateDispStatus(STATUS_WiFi,1);
    updateDispStatus(STATUS_NET,0);
    updateDispStatus(STATUS_COSM,0);
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
        Serial.println("Connected to api.xively.com");
        CosmState = CosmConnected;
        updateDispStatus(STATUS_NET,1);
        updateDispStatus(STATUS_COSM,0);
      }
      break;

    case CosmConnected:

      Serial.println("Subs to feed");
      if(true== client.subscribe(COSM_SUBSCRIBE_STRING)){
        CosmState = CosmSubscribed;
        Serial.print("OK");
        Serial.println(COSM_SUBSCRIBE_STRING);
        updateDispStatus(STATUS_COSM,1);
        setBlinkRate(3); // Status: Cosm COnnected
        digitalWrite( LED_PIN , HIGH) ;
        digitalWrite( LED_INTERNET_ON , LOW) ;
        lastConnectionTime = millis();
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
        Serial.println("Loop Disconnected");
        updateDispStatus(STATUS_COSM,0);
        GLCDNotifyError();
      }
#if(FORCE_RECONNECT_TO_COSM == 1)
      if ((millis() - lastConnectionTime) > connectionInterval) {
        CosmState = CosmDisconnected;
        Serial.println("Force Disconnect");
        client.disconnect();
        // update connection time so we wait before connecting again
        lastConnectionTime = millis();
      }
#endif
      break;

    default:
      break;

    }
    break;

  default:
    break;

  }

  // Update status
  if (WiFi.status() != WL_CONNECTED){
    WiFiState = WiFiDisconnected;
  }
  if((CosmState != CosmDisconnected) && (!client.connected())){
    CosmState = CosmDisconnected;
    Serial.println("Error:Cosm>Disconn");
    Serial.println("Client Disconnected");
    GLCDNotifyError();
    updateDispStatus(STATUS_COSM,0);
  }

}

void WiFiConnectToNetwork(void)
// This function finds the network encryption type and issues the appropriate library call
{
  status = WL_IDLE_STATUS;
  byte i= 0;
  byte numSsid = WiFi.scanNetworks(); // Find the total number of networks
  // Loop through all the networks, find the index of the one with the configured SSID
  // Then find the encryption type of that index, and make the correct begin() call
  // Get the encryption type of the newtork
  for(i=0; i<numSsid; i++)
  {
    Serial.println(WiFi.SSID(i));
    if(strncmp(WiFi.SSID(i),ssid,strlen(ssid)) == 0)
    {
      Serial.println("Matching SSID found");
      byte enc = WiFi.encryptionType(i); 
      switch(enc)
      {
      case 2: //WPA

      case 4: //WPA
        Serial.println("WPA Detected");
        status = WiFi.begin(ssid, key); 
        break;

      case 5:
        Serial.println("WEP Detected");
        status = WiFi.begin(ssid, keyIndex, key);
        break;

      case 7: // OPEN network
        Serial.println("OPEN n/w");
        status = WiFi.begin(ssid);
        break;
      }
      break;
    }
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

  // Whenever you connect to COSM, it will send you the last value of the feed.
  // This should not trigger a vend when the bot is set up for the first time.

  boolean approval = false;

  // Compare values
  if(compareArrays((char*)LastApprovalId,(char*)payload,length) == false)
    approval = true;// values are different, approve beer

  if(approval){
    if(IgnoreFeed == true){
      Serial.println("Virgin:Ignoring Feed Update");
      IgnoreFeed = false;
      Serial.println("Writing ID into SD Card");
      // Store it into SD
      writeToAuthSD((uint8_t*)payload,length);
      // Copying it into RAM for access in the current power cycle
      memcpy ( LastApprovalId, payload, length );
      client.publish(C_RESPONSE_FEED,C_RESPONSE_NOK);
    }
    else
    {
      Serial.println("Beer Approved!, Vending..");
      GLCDVendBeer();
      Serial.println("Writing ID into SD Card");
      // Store it into SD
      writeToAuthSD((uint8_t*)payload,length);
      // Copying it into RAM for access in the current power cycle
      memcpy ( LastApprovalId, payload, length );
      client.publish(C_RESPONSE_FEED,C_RESPONSE_OK);
    }
  }
  else{
    Serial.println("IDs are same, no beer for you");
    client.publish(C_RESPONSE_FEED,C_RESPONSE_NOK);
  }

}

void writeToAuthSD( uint8_t* dataP, int len)
{
  // Disable wifi SS
  //pinMode(WiFi_CS_PIN, OUTPUT);
  //digitalWrite(WiFi_CS_PIN, HIGH);
  // delete the file if it exists or you can open it and seek to the end..
  if(SD.exists(C_AUTH_FILE_NAME))
    SD.remove(C_AUTH_FILE_NAME);

  myFile = SD.open(C_AUTH_FILE_NAME, FILE_WRITE); // Open for write
  myFile.write(dataP, len);
  myFile.close();
}



void loadConfigFromSD(void)
// Loads configureation data from SD card, need only be done once at boot up
{
  char ssidt[30]; 
  char siz;
  int i = 0;
  byte temp = 0;

  if (SD.begin(SD_CS_PIN)) {
    Serial.println("SD init done");
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    myFile = SD.open("ssid.txt");
    if (myFile) {
      Serial.println("ssid:");
      // read from the file until there's nothing else in it:
      while ((myFile.available()) && ( i<C_SSID_MAX_LEN)) {
        temp = myFile.read();
        if((temp != '\r') && (temp != '\n')) // Ignore CR and LF. Some text editors do now allow you to write a line without them..
        {
          Serial.write(temp);
          ssid[i++] = temp;
        }
      }
      // close the file:
      myFile.close();

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
        if((temp != '\r') && (temp != '\n')){
          Serial.write(temp);
          key[i++] = temp;
        }
      }
      // close the file:
      myFile.close();

    }
    else {
      // if the file didn't open, print an error:
      Serial.println("error opening auth.txt");
    }

    // Open Auth Code
    i = 0;
    if(SD.exists(C_AUTH_FILE_NAME)){
      myFile = SD.open(C_AUTH_FILE_NAME);
      IgnoreFeed = false;
    }
    else{
      Serial.println("Auth code does not exist, this is virgin Beerbot");
      Serial.println("We will ignore the first Auth message from COSM");
      IgnoreFeed = true;
    }
    if (myFile) {
      Serial.println("Last Auth Code:");
      // read from the file until there's nothing else in it:
      while ((myFile.available()) && ( i<C_APPROVAL_ID_LEN)) {
        temp = myFile.read();
        if((temp != '\r') && (temp != '\n')){
          Serial.write(temp);
          LastApprovalId[i++] = temp;
        }
      }
      // close the file:
      myFile.close();
    }
    else {
      // if the file didn't open, print an error:
      Serial.println("error opening auth.txt");
    }

  } 
  else {
    // if the file didn't open, print an error:
    Serial.println("SD Card is not present");
  }



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




