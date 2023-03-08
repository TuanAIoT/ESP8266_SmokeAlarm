#include <Arduino.h>
#include <ESP8266WiFi.h>        //ESP8266 by ESP8266 Community
// OTA firmware
#include <ESP8266HTTPUpdateServer.h>    //HTTP Update Server
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
//WiFiManager auto connect
#include <ESP8266WebServer.h>   //webserver esp8266
#include <DNSServer.h>          
#include <WiFiManager.h>        //WiFiManager by tzapu
//MQTT
#include <PubSubClient.h>       //PubSubClient by Nick O'Leary
#include <WiFiClient.h>
#include <EEPROM.h>   //EEPROM save information
#include <time.h>   //real time

#define MSG_BUFFER_SIZE 500

unsigned int version_id = 1;    // version id
String nameProduct = "";        //name product
unsigned int fire;              // status fire
String pin = "none";            // status pin
int timezone = 7*3600;          //lấy múi giờ Vietnam : 7
int dst = 0;
unsigned long timeLast = 0;
String payloadStr = "";
String topicStr = "";
char message[MSG_BUFFER_SIZE];           //message MQTT

const char* host = "ESP8266";
const char* mqtt_host = "aiot-jsc1.ddns.net";
const char* mqtt_clientID = "stm8";
const int mqtt_port = 1889;
const char* mqtt_username = "stm8";
const char* mqtt_pass = "stm8";

const char* mqtt_subscribe = "demostm8/#";
const char* mqtt_publish_status = "demostm8/status";
const char* mqtt_publish_ota = "demostm8/ota";
const char* mqtt_publish_ota_result = "demostm8/ota-result";

//read data UART smoke alarm
const char* mqtt_publish_smoke = "demostm8/smoke";
bool flagCommand;

int position = 0;
char bufferUART[200];
int version;
int commandWord;
int dataLength;
//end

WiFiClient espClient;
PubSubClient client(espClient);

WiFiManager wifiManager;
ESP8266WebServer webServer(80);          
ESP8266HTTPUpdateServer httpUpdate;     

void newsStatus();
void newsOTA();
void newsOTA_result();
void checkUpdate();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect() ;
String printTime();
void readUart();
void checkCommandWord();
void reportWifi();
void getLocalTime();
void productInformation();
void productFunction();
void SmokeDetectionStatus(int typeData, int featureProductLength, int featureProduct);
void errorMessage(int typeData, int featureProductLength, int featureProduct);
void batteryStatus(int typeData, int featureProductLength, int featureProduct);
void alarmOff(int typeData, int featureProductLength, int featureProduct);
void lowPressureSmoke(int typeData, int featureProductLength, int featureProduct);
bool checkSum();
