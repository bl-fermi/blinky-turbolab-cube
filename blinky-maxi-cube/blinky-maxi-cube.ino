const boolean CHATTY_CATHY  = false;
const char*   MQTT_SERVER   = "192.168.4.1";
const char*   MQTT_USERNAME = "bl-fermi-01";
const char*   MQTT_PASSWORD = "andLetThereBeLite!";
const char*   BOX           = "bl-fermi-01";
const char*   TRAY_TYPE     = "bl-turbolab";
const char*   TRAY_NAME     = "01";
const char*   HUB           = "cube";

#include <Controllino.h>  

union CubeData
{
  struct
  {
    int16_t state;
    int16_t watchdog;
    int16_t newData;
    int16_t opsCmd;
    int16_t vacLimit;
    int16_t errState;
    int16_t opsState;
    int16_t wrnState;
    int16_t vacState;
    int16_t vacGuage;
    int16_t magProbe;
  };
  byte buffer[22];
};

CubeData cubeData;

byte mac[] = { 0x42, 0x4C, 0x30, 0x30, 0x30, 0x31 };

#include "BlinkyEtherCube.h"

unsigned long lastPublishTime;
unsigned long publishInterval = 2000;


int errComPin = CONTROLLINO_D0;
int opsComPin = CONTROLLINO_D1;
int wrnComPin = CONTROLLINO_D2;
int strCmdPin = CONTROLLINO_D3;
int sbyCmdPin = CONTROLLINO_D4;
int vntCmdPin = CONTROLLINO_D5;

int errNrmPin = CONTROLLINO_A0;
int errFltPin = CONTROLLINO_A1;
int opsNrmPin = CONTROLLINO_A2;
int opsFltPin = CONTROLLINO_A3;
int wrnNrmPin = CONTROLLINO_A4;
int vacGauPin = CONTROLLINO_A12;
int magPrbPin = CONTROLLINO_A13;

int16_t oldErrState = 0;
int16_t oldOpsState = 0;
int16_t oldWrnState = 0;
int16_t oldVacState = 0;
int16_t nowVacState = 0;
unsigned long lastDebounceTime;

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
  lastDebounceTime = lastPublishTime;
  cubeData.state = 1;
  cubeData.watchdog = 0;
  cubeData.errState = 0;
  cubeData.opsState = 0;
  cubeData.wrnState = 0;
  cubeData.vacGuage = 0;
  cubeData.magProbe = 0;
  cubeData.opsCmd = 0;
  cubeData.newData = 0;
  cubeData.vacLimit = 1022;
  cubeData.vacState = 0;

  pinMode(errComPin, OUTPUT); 
  pinMode(opsComPin, OUTPUT); 
  pinMode(wrnComPin, OUTPUT); 
  pinMode(strCmdPin, OUTPUT); 
  pinMode(sbyCmdPin, OUTPUT); 
  pinMode(vntCmdPin, OUTPUT); 

  pinMode(errNrmPin, INPUT); 
  pinMode(errFltPin, INPUT); 
  pinMode(opsNrmPin, INPUT); 
  pinMode(opsFltPin, INPUT); 
  pinMode(wrnNrmPin, INPUT); 
  pinMode(vacGauPin, INPUT); 
  pinMode(magPrbPin, INPUT); 

  digitalWrite(errComPin,1);
  digitalWrite(opsComPin,1);
  digitalWrite(wrnComPin,1);
  digitalWrite(strCmdPin,0);
  digitalWrite(sbyCmdPin,0);
  digitalWrite(vntCmdPin,0);

}

void loop()
{
  cubeData.newData = 0;
  int16_t errNrm = digitalRead(errComPin);
  int16_t errFlt = digitalRead(errFltPin);

  int16_t opsNrm = digitalRead(opsNrmPin);
  int16_t opsFlt = digitalRead(opsFltPin);
  
  int16_t wrnNrm = digitalRead(wrnNrmPin);
  
  cubeData.vacGuage = analogRead(vacGauPin);
  cubeData.magProbe = analogRead(magPrbPin);

  if ((errNrm > 0) && (errFlt < 1)) cubeData.errState = 0;
  if ((errNrm < 1) && (errFlt > 0)) cubeData.errState = 1;
  if (cubeData.errState != oldErrState) cubeData.newData = 1;
  oldErrState = cubeData.errState;

  if ((opsNrm > 0) && (opsFlt < 1)) cubeData.opsState = 1;
  if ((opsNrm < 1) && (opsFlt > 0)) cubeData.opsState = 0;
  if (cubeData.opsState != oldOpsState) cubeData.newData = 1;
  oldOpsState = cubeData.opsState;

  if (wrnNrm > 0)  cubeData.wrnState = 1;
  if (wrnNrm < 1)  cubeData.wrnState = 0;
  if (cubeData.wrnState != oldWrnState) cubeData.newData = 1;
  oldWrnState = cubeData.wrnState;

  unsigned long nowTime = millis();
  nowVacState = 0;
  if (cubeData.vacGuage >=  cubeData.vacLimit) nowVacState = 1;
  if (nowVacState != oldVacState) lastDebounceTime = nowTime;
  if ((nowTime - lastDebounceTime) > 5000) 
  {
    if (nowVacState != cubeData.vacState) 
    {
      cubeData.vacState = nowVacState;
      cubeData.newData = 1;
      Serial.println("Vac Limit");
    }
  }
  oldVacState = nowVacState;

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
    if (CHATTY_CATHY)
    {
      Serial.print("Err Pins: ");
      Serial.print(errNrm);
      Serial.print(",");
      Serial.println(errFlt);

      Serial.print("Ops Pins: ");
      Serial.print(opsNrm);
      Serial.print(",");
      Serial.println(opsFlt);

      Serial.print("Wrn Pins: ");
      Serial.print(wrnNrm);
      Serial.print(",");
      Serial.println("0");
      
      Serial.print("Vac Gaug: ");
      Serial.println(cubeData.vacGuage);
      
      Serial.print("Mag Prob: ");
      Serial.println(cubeData.magProbe);
      Serial.println("");
    }
   
    lastPublishTime = nowTime;
    cubeData.watchdog = cubeData.watchdog + 1;
    if (cubeData.watchdog > 32760) cubeData.watchdog= 0 ;
    BlinkyEtherCube.publishToServer();
  }  
  BlinkyEtherCube.loop();
}
void handleNewSettingFromServer(uint8_t address)
{
  cubeData.newData = 0;
  switch(address)
  {
    case 3:
      switch(cubeData.opsCmd)
      {
        case 0:
          digitalWrite(strCmdPin,0);
          digitalWrite(sbyCmdPin,0);
          digitalWrite(vntCmdPin,0);
          break;
        case 1:
          digitalWrite(strCmdPin,1);
          digitalWrite(sbyCmdPin,0);
          digitalWrite(vntCmdPin,0);
          break;
        case 2:
          digitalWrite(strCmdPin,0);
          digitalWrite(sbyCmdPin,1);
          digitalWrite(vntCmdPin,0);
          break;
        case 3:
          digitalWrite(strCmdPin,0);
          digitalWrite(sbyCmdPin,0);
          digitalWrite(vntCmdPin,1);
          break;
        default:
          break;
      }       
      cubeData.newData = 1;
      break;
    case 4:
      lastDebounceTime = 0;
      break;
    default:
      break;
  }
  if (cubeData.newData > 0)
  {
    lastPublishTime = millis();
    cubeData.watchdog = cubeData.watchdog + 1;
    if (cubeData.watchdog > 32760) cubeData.watchdog= 0 ;
    BlinkyEtherCube.publishToServer();
    BlinkyEtherCube.loop();
    cubeData.newData = 0;
  }
}
