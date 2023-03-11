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
#include <WiFiClientSecure.h>
#include <EEPROM.h>   //EEPROM save information
#include <time.h>   //real time
#include <MD5Builder.h>
#define MSG_BUFFER_SIZE 500

unsigned int version_id = 1;    // version id
bool flagEvent = false;
String timeEvent = "";
String nameProduct = "";        //name product
unsigned int fire;              // status fire
int battery = 100;            // %pin, -1 là sử dụng nguồn ngoài
int timezone = 7*3600;          //lấy múi giờ Vietnam : 7
int dst = 0;
unsigned long timeLast = 0;
String payloadStr = "";
String topicStr = "";
char message[MSG_BUFFER_SIZE];           //message MQTT
//read data UART smoke alarm

bool flagCommand;
int position = 0;
char bufferUART[200];
int version;
int commandWord;
int dataLength;
//end
const char host[] = "ESP8266";
const char mqtt_host[] = "aiot-jsc1.ddns.net";
const char mqtt_clientID[] = "stm8";
const int mqtt_port = 8883;
const char mqtt_username[] = "stm8";
const char mqtt_pass[] = "stm8";
const char deviceSecret[] = "MIID0zCCAr";
const char deviceUsername[] = "cc55d98ee6c4ebf10e3abe1f00eacde3";
const char deviceModel[] = "/SFO-01";
const char publish_connection[] = "/connection";
const char publish_status[] = "/status";
const char publish_event[] = "/event";
const char publish_ota[] = "SFO-01/ota";
const char publish_ota_result[] = "/ota-result";
const char root_ca[] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
MIID0zCCArugAwIBAgIUbvcszjYWz4GXdSIRHb9y6w/2okMwDQYJKoZIhvcNAQEL
BQAweTELMAkGA1UEBhMCVk4xDjAMBgNVBAgMBUhhTm9pMQ4wDAYDVQQHDAVIYU5v
aTENMAsGA1UECgwEQUlvVDENMAsGA1UECwwEQUlvVDENMAsGA1UEAwwEQUlvVDEd
MBsGCSqGSIb3DQEJARYOYWlvdEBnbWFpbC5jb20wHhcNMjIxMTExMDc1MzI1WhcN
MzIxMTA4MDc1MzI1WjB5MQswCQYDVQQGEwJWTjEOMAwGA1UECAwFSGFOb2kxDjAM
BgNVBAcMBUhhTm9pMQ0wCwYDVQQKDARBSW9UMQ0wCwYDVQQLDARBSW9UMQ0wCwYD
VQQDDARBSW9UMR0wGwYJKoZIhvcNAQkBFg5haW90QGdtYWlsLmNvbTCCASIwDQYJ
KoZIhvcNAQEBBQADggEPADCCAQoCggEBAKwd6AXCATG1a/l/F9YQCwVKIa22f33b
1VSbBXMI+FaVP/rWw6C60n7gr8Ot6EomBweAlj8ylx31jDOcnlgbq+wIFZMbpXim
MrgtZbbESAv7CCWm3ts1rX05EY6K27QcUQB/zMz2+c3aVnOkJCKezrTvMlMjmVWV
B7HW0rpesu4SX1pW77bpwGjdpaK+L78QS2KCghZMvnzkJ/2cVUhvZYUAw0W1Lkeb
ZSbeUV3w40rI7RLmNQp6+aV+WMCP8N84piYWA/aazu8cRW37OcWN2zEyHa3mZwtS
vwUWBD2VnJ9b1Q7R8H9a47JQPTbIN9hGJvl0Ak5Igv8oWmmBW8qpLfUCAwEAAaNT
MFEwHQYDVR0OBBYEFHfyMPAqXmJ/pZXvhHfY/KlKROfkMB8GA1UdIwQYMBaAFHfy
MPAqXmJ/pZXvhHfY/KlKROfkMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEL
BQADggEBADI2AYq3m+rYOOvnOoLti/agoWq7V29L3AOOhHKOR0bbC40/whvesA7K
wO6HFrFvYok/63vyr0HxeKwZtlrdUhchxY4SPPYo5n0b1oHUwFqENqLuBoBGu5IA
A8FQwTo9QTEvilmxg24nMm9kPKRdVtyCjIdAEYxeoZphX6JwBz0+M/FZ0T109rhl
OmtxtPw/cJzVhPfe6kQka+Lk/umDLQty4ex9mA+dwFKXmTwJoiOi19VvVjGnKWau
AFPVl5iTOW2CNbj0IuSxLiEqx94WGpj+RaPcJr/YprKVTp4xcAoPPEzvYxpDRNAI
aTIA7Lxot40x9vvpMvo8mpEck2phc8c=
-----END CERTIFICATE-----
)CERT";

WiFiClientSecure espClient;
PubSubClient client(espClient);
WiFiManager wifiManager;
ESP8266WebServer webServer(80);          
ESP8266HTTPUpdateServer httpUpdate;     
MD5Builder _md5;

void newsStatus();
void newsEvent();
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
String md5(String str);