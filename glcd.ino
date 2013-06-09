/*

 Description: GLCD Interface
 
 Author: Govind Mukundan (govind.mukundan at gmail.com)
 
 References:
 1. GLCD Display  -> http://www.adafruit.com/products/438
 2. u8glib Graphics Library -> http://code.google.com/p/u8glib/
 3. Convert Monochrome BMP to HEX -> http://www.digole.com/tools/PicturetoC_Hex_converter.php
 
 */

#include "U8glib.h"
#include "beer_icon.h"
#include "exp_icon.h"
#include "sync_icon.h"
#include "error_icon.h"

// RGB backlight using pins DAC0, DAC1 and 2
#define C_RLED  2
#define C_GLED  DAC0
#define C_BLED  DAC1
// Color table
// Note: At the moment colors are not working prob because the default frequency of analogwrite() (500Hz) is too low
// Need to access the PWM resisters directly..
static const byte Colour_Table[8][3] =
{
  {0,255,255},//Red
  {255,0,255},//Green
  {255,255,0},//Blue
  {0,0,0},//White
  {0,204,255},//Orange
  {0,0,255},//Yellow
  {127,0,255},//Light Green
  {0,204,51} //Pink
};

typedef enum{
  Red,
  Green,
  Blue,
  White,
  Orange,
  Yellow,
  LightGreen,
  Pink
}e_Colours;

#define BEERBOT_VERSION  1
#define TIME_TEXT_DISP    5
#define TIME_SPLASH_DISP    5000
#define C_VENDING_SCREEN_TIMEOUT (10000)
#define SPLASH_MSG1 " Maxus "
#define SPLASH_MSG2 "MetalWorks"
#define VEND_MSG1 "Please "
#define VEND_MSG2 "Collect"
#define VEND_MSG3 "Your Beer!"
#define VEND_MSG4 ""
#define CONNECT_MSG "Connecting"
#define READY_MSG "READY!!!"
#define STATUS_GSM_NO_SIM "GSM:SIM??"
#define STATUS_GSM_OK     "GSM:OK"
#define STATUS_GSM_NOK    "GSM:NOK"
#define STATUS_WiFi_OK    "WiFi:OK"
#define STATUS_WiFi_NOK   "WiFi:NOK"
#define STATUS_NET_OK     "WWW:OK"
#define STATUS_NET_NOK    "WWW:NOK"
#define STATUS_COSM_OK    "COSM:OK"
#define STATUS_COSM_NOK   "COSM:NOK"
#define STATUS_GSM  1
#define STATUS_WiFi  2
#define STATUS_NET  3
#define STATUS_COSM  4

const char* statusGsm ;
const char* statusWiFi ;
const char* statusNet ;
const char* statusCosm ;

typedef enum{
  DispDummy,
  DispConnecting,
  DispError,
  DispReady,
  DispVending
}
e_DisplayStates;

e_DisplayStates DispState;
e_DisplayStates DispStatePrev;
boolean updateDispStat;
boolean ShowSpashScreen;

#define FONT_NORM		u8g_font_unifont
#define FONT_SMALL		u8g_font_fixed_v0
#define SPACING     15	// Space bet two lines
#define LINE1_X		65
#define LINE1_Y		15
#define LINE2_X		65
#define LINE2_Y		(LINE1_Y + SPACING)
#define LINE3_X		65
#define LINE3_Y		(LINE2_Y + SPACING)
#define LINE4_X		65
#define LINE4_Y		(LINE3_Y + SPACING)
#define GSM_X	LINE1_X
#define GSM_Y	LINE1_Y
#define WIFI_X	LINE2_X
#define WIFI_Y	LINE2_Y
#define INET_X	LINE3_X
#define INET_Y	LINE3_Y
#define COSM_X	LINE4_X
#define COSM_Y	LINE4_Y


U8GLIB_LM6059 u8g(13, 11, 10, 9);		// SPI Com: SCK = 13, MOSI = 11, CS = 10, A0 = 9
void GLCDSetBkColor(e_Colours value);


void GLCDInit(void)
{
  // show splash screen
  ShowSpashScreen = true;
  pinMode(C_RLED, OUTPUT);
  pinMode(C_GLED, OUTPUT);
  pinMode(C_BLED, OUTPUT); 
  GLCDSetBkColor(Red);
}

void GLCDTask() {
  // 1. Update display states
  // 2. Call the Picture Loop if there is a change in state
  GLCDPictureLoop();
}

void GLCDVendBeer(void){
  DispState = DispVending;
}

void GLCDNotifyError(void)
{
  DispState = DispError;
}

void GLCDPictureLoop(void)
{
  // The driver may be splitting the display frame buffer into 2 halves (pages), we don't know or care so we must call draw for each page
  if(ShowSpashScreen)
  {
    u8g.firstPage();
    do {

      spashScreen(); //The actual drawing is done here
    } 
    while( u8g.nextPage() );
    ShowSpashScreen = false;
    delay(5000);
  }
  if((updateDispStat == true) || (DispState != DispStatePrev))
  {
    Serial.println("GLCD:Updating Display..");
    u8g.firstPage();
    do {

      DispScreenUpdate(); //The actual drawing is done here
    } 
    while( u8g.nextPage() );

    DispStatePrev = DispState;
    updateDispStat = false;
    if(DispState == DispVending){
      // After the screen is built, vend the beer and wait for some time
      vendBeer();
      delay(C_VENDING_SCREEN_TIMEOUT);
      DispState = DispReady;
    }
  }
}

void DispScreenUpdate(void)
// Call this periodically to update the screen based on events
{
  // Only draw the screen if the state has changed
  switch(DispState)
  {
  case DispConnecting:
    GLCDSetBkColor(Blue);
    u8g.drawBitmap( 0, 0,SYNC_ICON_WIDTH/8 ,  SYNC_ICON_HEIGHT , sync_icon);
    u8g.setFont(FONT_SMALL);
    u8g.drawStr( LINE1_X, 10, CONNECT_MSG);
    u8g.drawStr( LINE2_X, 20, "...");
    u8g.drawStr( LINE3_X, 30, statusGsm);
    u8g.drawStr( LINE3_X, 40, statusWiFi);
    u8g.drawStr( LINE3_X, 50, statusNet);
    u8g.drawStr( LINE3_X, 60, statusCosm);
    Serial.println("GLCD:Connecting..");
    break;

  case DispReady:
    GLCDSetBkColor(Green);
    u8g.drawBitmap( 0, 0,BEER_ICON_WIDTH/8 ,  BEER_ICON_HEIGHT , beer_icon);
    Serial.println("GLCD:Ready..");
    updateDispStat = true;
    break;

  case DispVending:
    GLCDSetBkColor(Blue);
    u8g.drawBitmap( 0, 0,EXP_ICON_WIDTH/8 ,  EXP_ICON_HEIGHT , exp_icon);
    Serial.println("GLCD:Vending..");
    u8g.setFont(FONT_SMALL);
    u8g.drawStr( LINE1_X, 10, "-------------");
    u8g.drawStr( LINE2_X, 20, VEND_MSG1);
    u8g.drawStr( LINE3_X, 30, VEND_MSG2);
    u8g.drawStr( LINE3_X, 40, VEND_MSG3);
    u8g.drawStr( LINE3_X, 50, VEND_MSG4);
    u8g.drawStr( LINE3_X, 60, "-------------");
    break;

  default:
    GLCDSetBkColor(Red);
    u8g.drawBitmap( 0, 0,ERROR_ICON_WIDTH/8 ,  ERROR_ICON_HEIGHT , error_icon);
    Serial.println("GLCD:Error..");
    updateDispStat = true;
    break;

  }

  if( (updateDispStat == true) && ((DispState == DispError) || (DispState == DispReady)) ){
    u8g.setFont(FONT_NORM);
    u8g.drawStr( GSM_X, GSM_Y, statusGsm);
    u8g.drawStr( WIFI_X, WIFI_Y, statusWiFi);
    u8g.drawStr( INET_X, INET_Y, statusNet);
    u8g.drawStr( COSM_X, COSM_Y, statusCosm);
  }
}

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
      if( DispState != DispReady)
        DispState = DispReady;
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
      if( DispState != DispReady)
        DispState = DispReady;
      break;
    }
    break;

  }
  updateDispStat = true;
}


void spashScreen(void)
{
  statusGsm = STATUS_GSM_NOK;
  statusWiFi = STATUS_WiFi_NOK;
  statusNet = STATUS_NET_NOK;
  statusCosm = STATUS_COSM_NOK;

  // Display title for some time
  u8g.setFont(u8g_font_courB10); // you can also make your own fonts, see playground for details
  // GLCD.CursorToXY(GLCD.Width/2 - 44, 3);
  u8g.drawStr(0,15,"BeerBot v:1.0");
  u8g.setFont(FONT_SMALL);
  u8g.drawStr(0,30,"Brought to you by");
  u8g.setFont(u8g_font_courB10);
  u8g.drawStr(0,45,"Maxus Global");

  // move to status screen
  DispState = DispConnecting;
  DispStatePrev = DispDummy;
  updateDispStatus(STATUS_GSM,0);
  updateDispStatus(STATUS_WiFi,0);
  updateDispStatus(STATUS_COSM,0);
  updateDispStatus(STATUS_NET,0);
  //updateDispStatus(0,0); // Dummy update of status
  //DispScreenUpdate(); //Force the screen to update
}

void GLCDSetBkColor(e_Colours value)
{
  analogWrite(C_RLED, Colour_Table[value][0]);
  analogWrite(C_GLED, Colour_Table[value][1]);
  analogWrite(C_BLED, Colour_Table[value][2]); 
  
}


