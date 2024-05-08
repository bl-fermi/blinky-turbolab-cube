const boolean CHATTY_CATHY  = true;
const char*   MQTT_SERVER   = "10.1.1.1";
const char*   MQTT_USERNAME = "blinky-lite-box-01";
const char*   MQTT_PASSWORD = "areallybadpassword";
const char*   BOX           = "blinky-lite-box-01";
const char*   TRAY_TYPE     = "blinky-mqtt";
const char*   TRAY_NAME     = "maxi-01";
const char*   HUB           = "cube";

#include <Controllino.h>  
#define BOUNCE_DELAY 50 
union CubeData
{
  struct
  {
    int16_t state;
    int16_t watchdog;
    int16_t linoD0;
    int16_t linoD1;
    int16_t linoR0;
    int16_t linoR1;
    int16_t linoA0;
    int16_t linoA1;
    int16_t newData;
  };
  byte buffer[18];
};
CubeData cubeData;

struct PositionSwitch
{
  int currentState;
  int lastState;
  int inputPin;
  int hotPin;
  unsigned long lastDebounceTime;  
  unsigned long switchTime;  
  unsigned long debounceDelay;    
};

PositionSwitch posSwitch[2];

byte mac[] = { 0x42, 0x4C, 0x30, 0x30, 0x30, 0x31 };

#include "BlinkyEtherCube.h"

unsigned long lastPublishTime;
unsigned long publishInterval = 2000;

void setup()
{
  if (CHATTY_CATHY)
  {
    Serial.begin(9600);
    delay(10000);
    Serial.println("Starting communications...");
  }

  // Optional setup to overide defaults
  BlinkyEtherCube.setChattyCathy(CHATTY_CATHY);
  BlinkyEtherCube.setMqttRetryMs(3000);
  BlinkyEtherCube.setBlMqttKeepAlive(8);
  BlinkyEtherCube.setBlMqttSocketTimeout(4);
  
  // Must be included
  BlinkyEtherCube.setMqttServer(mac, MQTT_SERVER, MQTT_USERNAME, MQTT_PASSWORD);
  BlinkyEtherCube.setMqttTray(BOX,TRAY_TYPE,TRAY_NAME, HUB);
  BlinkyEtherCube.init(&cubeData);

  lastPublishTime = millis();
  cubeData.state = 1;
  cubeData.watchdog = 0;
  cubeData.linoD0 = 0;
  cubeData.linoD1 = 0;
  cubeData.linoR0 = 0;
  cubeData.linoR1 = 0;
  cubeData.linoA0 = 0;
  cubeData.linoA1 = 0;
  cubeData.newData = 0;

  pinMode(CONTROLLINO_R0, OUTPUT); 
  pinMode(CONTROLLINO_R1, OUTPUT); 
   
  digitalWrite(CONTROLLINO_D0, cubeData.linoD0);   
  digitalWrite(CONTROLLINO_D1, cubeData.linoD1);   

  posSwitch[0].inputPin = CONTROLLINO_A0;
  posSwitch[0].hotPin   = CONTROLLINO_D0;

  posSwitch[1].inputPin = CONTROLLINO_A1;
  posSwitch[1].hotPin   = CONTROLLINO_D1;

  for (int ii = 0; ii < 2; ++ii)
  {
    posSwitch[ii].debounceDelay = BOUNCE_DELAY;
    pinMode(posSwitch[ii].inputPin, INPUT);
    pinMode(posSwitch[ii].hotPin, OUTPUT);
    digitalWrite(posSwitch[ii].hotPin, 1);
    delay(posSwitch[ii].debounceDelay);
    posSwitch[ii].currentState = digitalRead(posSwitch[ii].inputPin);
    posSwitch[ii].lastState = posSwitch[ii].currentState;
    posSwitch[ii].lastDebounceTime = millis();
    posSwitch[ii].switchTime = posSwitch[ii].lastDebounceTime;
  }
}

//modified  this routine from https://docs.arduino.cc/built-in-examples/digital/Debounce/
boolean readSwitch(PositionSwitch* pswitch)
{
  int switchReading = digitalRead(pswitch->inputPin);
  unsigned long now = millis();
  boolean newState = false;
  
  if (switchReading != pswitch->lastState) 
  {
    pswitch->lastDebounceTime = now;
  }

  if ((now - pswitch->lastDebounceTime) > pswitch->debounceDelay) 
  {
    if (switchReading != pswitch->currentState) 
    {
      pswitch->currentState = switchReading;
      pswitch->switchTime = now;
      newState = true;
    }
  }
  pswitch->lastState = switchReading;
  return newState;
}

void loop()
{
  unsigned long nowTime = millis();
  if (readSwitch(&posSwitch[0]))
  {
    cubeData.linoA0 = posSwitch[0].currentState;
    cubeData.newData = 1;
  }
  if (readSwitch(&posSwitch[1]))
  {
    cubeData.linoA1 = posSwitch[1].currentState;
    cubeData.newData = 1;
  }
  if (cubeData.newData > 0)
  {
    lastPublishTime = nowTime;
    cubeData.watchdog = cubeData.watchdog + 1;
    if (cubeData.watchdog > 32760) cubeData.watchdog= 0 ;
    BlinkyEtherCube.publishToServer();
    BlinkyEtherCube.loop();
    cubeData.newData = 0;
  }
 
  if ((nowTime - lastPublishTime) > publishInterval)
  {
    lastPublishTime = nowTime;
    cubeData.watchdog = cubeData.watchdog + 1;
    if (cubeData.watchdog > 32760) cubeData.watchdog= 0 ;
    BlinkyEtherCube.publishToServer();
  }  
  BlinkyEtherCube.loop();
}
void handleNewSettingFromServer(uint8_t address)
{
  switch(address)
  {
    case 0:
      break;
    case 1:
      break;
    case 2:
      digitalWrite(CONTROLLINO_D0, cubeData.linoD0);  
      break;
    case 3:
      digitalWrite(CONTROLLINO_D1, cubeData.linoD1);    
      break;
    case 4:
      digitalWrite(CONTROLLINO_R0, cubeData.linoR0);  
      break;
    case 5:
      digitalWrite(CONTROLLINO_R1, cubeData.linoR1);    
      break;
    default:
      break;
  }
}
