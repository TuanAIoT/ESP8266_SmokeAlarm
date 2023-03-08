#include "main.h"
 
void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);
  timeLast = millis();
  if(WiFi.status() != WL_CONNECTED) wifiManager.autoConnect(host);
  //setup server mqtt
  client.setServer(mqtt_host, mqtt_port);
  client.setCallback(callback);
  if (!client.connected()) reconnect();
  //Config time
  configTime(timezone, dst,"pool.ntp.org","time.nist.gov");
  while(!time(nullptr)){
    Serial.print(".");
    delay(1000);
  }
  if(EEPROM.read(0) == 1) newsOTA_result();
}

void loop() {
  if (Serial.available()) readUart();
  if(WiFi.status() != WL_CONNECTED) wifiManager.autoConnect(host);
  if (!client.connected()) reconnect();
  if(topicStr == mqtt_publish_ota) newsOTA();
  if (flagCommand == true) checkCommandWord();
  newsStatus();
  client.loop();
}

void newsStatus(){
  int timeStatus;
  if(fire == 0){
    timeStatus = 30000;
  }
  else if(fire == 1)
  {
    timeStatus = 5000;
  }
  if ((millis() - timeLast) > timeStatus) {
    timeLast = millis();
    String status = "";
    status += "{\n  ";
    status += '"';
    status += "pin";
    status += '"';
    status += ": " + pin + "\n  ";
    status += '"';
    status += "fire";
    status += '"';
    status += ": " + String(fire) + "\n  ";
    status += '"';
    status += "time";
    status += '"';
    status += ": " + printTime() + "\n}";
    for(int i=0; i< MSG_BUFFER_SIZE; i++){
      message[i] = '\0';
    }
    for(int i=0; i< status.length(); i++){
      message[i] = status[i];
    }
   client.publish(mqtt_publish_status, message);
  }
}

void newsOTA(){
  topicStr = "\0";
  int i = 0;
  String versionStr = "version_id";
  versionStr += '"';
  versionStr += ": ";
  String urlStr = "url";
  urlStr += '"';
  urlStr += ": ";
  int lengthStr = 0;
  int viTriStr = String(payloadStr).indexOf(versionStr);

  if(viTriStr != -1){
    lengthStr = versionStr.length();
    versionStr = "";
    while(payloadStr[viTriStr + lengthStr + i] != ','){
      versionStr += payloadStr[viTriStr + lengthStr + i];
      i++;
    }
    int versionTemp = versionStr.toInt();
    if(version_id < versionTemp){
      //update version
      viTriStr = String(payloadStr).indexOf(urlStr);
      lengthStr = urlStr.length();
      urlStr = "";
      i = 1;
      while(payloadStr[viTriStr + lengthStr + i] != '"'){
        urlStr += payloadStr[viTriStr + lengthStr + i];
        i++;
      }
      for(int i = 0; i < 512; i++){
        EEPROM.write(i, 0);
      }
      EEPROM.write(0,1);            //save enable newsOTA_resuilt to position 0 of EEPROM
      for(i = 0; i< urlStr.length(); i++){
        EEPROM.write(i + 5, urlStr[i]);
      }
      EEPROM.commit();   
      checkUpdate();
    }
  }
  newsOTA_result();
}

void newsOTA_result(){
  int address;
  int statusOTA;
  String url = "";
  String s = "";
  EEPROM.write(0,0);      //disable newsOTA_resuilt
  int versionTemp = EEPROM.read(1);
  if(version_id == versionTemp){
    statusOTA = 1;
  }
  else{
    statusOTA = 0;
  }
  address = 5;
  while(EEPROM.read(address) != 0 ){
    if(char(EEPROM.read(address)) != '"'){
      url += char(EEPROM.read(address));
    }
    address++;
  }  
  EEPROM.commit(); 

  s += "{\n  ";
  s += '"';
  s += "version_id";
  s += '"';
  s += ": " + String(version_id) + ",\n  ";
  s += '"';
  s += "url";
  s += '"';
  s += ": ";
  s += '"';
  s += url;
  s += '"';
  s += ",\n  ";
  s += '"';
  s += "status";
  s += ": " + String(statusOTA) + "\n}";
  for(int i = 0; i < MSG_BUFFER_SIZE; i++){
    message[i] = '\0';
  }
  for(int i = 0; i < s.length(); i++){
    message[i] = s[i];
  }
  client.publish(mqtt_publish_ota_result, message);
}

void checkUpdate(){
  int address = 5;
  String url;
  while(EEPROM.read(address) != 0 ){
    url += char(EEPROM.read(address));
    address++;
  }
  String versionStr = "esp8266.ino." + String(version_id);
  Serial.print("\nDang tien hanh update firmware");
  client.publish(mqtt_publish_ota,"{\n  Start update firmware\n}");
  ESPhttpUpdate.update(espClient, url, versionStr);
}

void callback(char* topic, byte* payload, unsigned int length) {
  topicStr = String(topic);
  if(topicStr != mqtt_publish_smoke){
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.println("]");
    payloadStr = "";
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
      payloadStr += (char)payload[i];
    }
    Serial.println();
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_clientID, mqtt_username, mqtt_pass)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(mqtt_publish_status, "connected");
      // ... and resubscribe
      client.subscribe(mqtt_subscribe);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

String printTime(){
  String strTime;
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);

  strTime += '"' + String(p_tm->tm_year + 1900) + '-';
  if((p_tm->tm_mon + 1) < 10) strTime += '0' + String(p_tm->tm_mon + 1) + '-';
  else strTime += String(p_tm->tm_mon + 1) + '-'; 
  if((p_tm->tm_mday) < 10) strTime += '0' + String(p_tm->tm_mday) + ' '; 
  else strTime += String(p_tm->tm_mday) + ' ';
  if((p_tm->tm_hour) < 10) strTime += '0' + String(p_tm->tm_hour) + ':';
  else strTime += String(p_tm->tm_hour) + ':';
  if((p_tm->tm_min) < 10) strTime += '0' +String(p_tm->tm_min) + ':';
  else strTime += String(p_tm->tm_min) + ':';
  if((p_tm->tm_sec) < 10) strTime += '0' + String(p_tm->tm_sec);
  else strTime += String(p_tm->tm_sec);  
  strTime += '"';
  return strTime;
}

//smoke alarm

void readUart(){
  int dataUart = Serial.read();
  bufferUART[position] = dataUart;
  if(dataUart == 0x55){
    if(position != 0){
      if(checkSum() == true){
        version = bufferUART[2];
        commandWord = bufferUART[3];
        dataLength = bufferUART[4];
        dataLength += bufferUART[5];
        flagCommand = true;
      }
      position = 0;
    }
  }
  position++;
}

void checkCommandWord(){
    switch(commandWord)
  {
    case 0x01:
      Serial.println("Truy van thong tin san pham");
      productInformation();
      break;  
    case 0x02:
      Serial.println("Bao cao trang thai mang cua thiet bi");
      //client.publish(mqtt_publish_smoke, "Báo cáo trạng thái mạng của thiết bị");
      reportWifi();
      break;
    case 0x03:
      Serial.println("Khoi dong lai WIFI");
      //client.publish(mqtt_publish_smoke,"Khởi động lại WIFI");
      break;  
    case 0x04:
      Serial.println("Dat lai che do cau hinh chon WIFI");
      //client.publish(mqtt_publish_smoke,"Đặt lại chế độ cấu hình chọn WIFI");
      break;
    case 0x05:
      Serial.println("Bao cao trang thai thoi gian thuc");
      //client.publish(mqtt_publish_smoke,"Báo cáo trạng thái thời gian thực");
      productFunction();
      break;
    case 0x06:
      Serial.println("Lay gio dia phuong");
      //client.publish(mqtt_publish_smoke,"Lấy giờ địa phương");
      getLocalTime();
      break;
    case 0x07:
      Serial.println("Kiem tra chuc nang WIFI");
      //client.publish(mqtt_publish_smoke,"Kiểm tra chức năng WIFI");
      break;
    case 0x08:
      Serial.println("Bao cao trang thai loai ban ghi");
      //client.publish(mqtt_publish_smoke,"Báo cáo trạng thái loại bản ghi");
      break;
    case 0x09:
      Serial.println("Gui lenh module");
      //client.publish(mqtt_publish_smoke,"Gửi lệnh module");
      break;
    default:
      Serial.println("error Command Word");
      //client.publish(mqtt_publish_smoke,"error Command Word");
  }
  flagCommand = false;
}

void reportWifi(){
  char temp[8] = {0x55, 0xAA, 0x00, 0x02, 0x00, 0x01, 0x01, 0x03};
  if(WiFi.status() != WL_CONNECTED){
    temp[6] = 0x02;
    temp[7] = 0x04;
  }
  else if(WiFi.status() == WL_CONNECTED){
    temp[6] = 0x03;
    temp[7] = 0x05;
  }
  else if(client.connected()){
    temp[6] = 0x04;
    temp[7] = 0x06;
  }
  Serial.write(temp,8);
  Serial.write(temp,8);
  Serial.write(temp,8);  
}

void getLocalTime(){
  delay(500);
  char information[7] = {0x55, 0xAA, 0x00, 0x01, 0x00, 0x00, 0x00};
  Serial.write(information,7);
  char localTime[15] = {0x55, 0xAA, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D};
  Serial.write(localTime,15);
  Serial.write(localTime,15);
  Serial.write(localTime,15);
}

void productInformation(){
  char tempStr[dataLength];
  for(int i = 0; i < dataLength; i++){
    tempStr[i] = bufferUART[6+i];
    nameProduct += tempStr[i];
    Serial.print(tempStr[i]);
    tempStr[i+1] = '\0';
  }
 client.publish(mqtt_publish_smoke, tempStr); 
}

void productFunction(){
  int dpID;
  int typeData;
  int featureProductLength;
  int featureProduct;
  char dataArr[dataLength];
  for(int i = 0; i < dataLength; i++){
    dataArr[i] = bufferUART[6+i];
  }
  dpID = dataArr[0];
  typeData = dataArr[1];
  featureProductLength = dataArr[2] + dataArr[3];
  featureProduct = dataArr[4];
  switch(dpID)
  {
    case 0x00:
      Serial.println("Yeu cau gui Bao cao trang thai thoi gian thuc");
      //client.publish(mqtt_publish_smoke,"Yêu cầu gửi Báo cáo trạng thái thời gian thực");
      SmokeDetectionStatus(typeData, featureProductLength, featureProduct);
      break;
    case 0x01:
      Serial.println("Tinh trang phat hien khoi");
      //client.publish(mqtt_publish_smoke,"Tình trạng phát hiện khói");
      SmokeDetectionStatus(typeData, featureProductLength, featureProduct);
      break;
    case 0x0b:
      Serial.println("Bao loi");
      //client.publish(mqtt_publish_smoke,"Báo lỗi");
      errorMessage(typeData, featureProductLength, featureProduct);
      break;
    case 0x0e:
      Serial.println("Trang thai pin");
      //client.publish(mqtt_publish_smoke,"Trạng thái pin");
      batteryStatus(typeData, featureProductLength, featureProduct);
      break;
    case 0x10:
      Serial.println("Tat bao dong");
      //client.publish(mqtt_publish_smoke,"Tắt báo động");
      alarmOff(typeData, featureProductLength, featureProduct);
      break;
    case 0x65:
      Serial.println("Khoi ap suat thap");
      //client.publish(mqtt_publish_smoke,"Khói áp suất thấp");
      lowPressureSmoke(typeData, featureProductLength, featureProduct);
      break;
    default:
    Serial.println("Error dpID: ");
    //client.publish(mqtt_publish_smoke,"Error dpID: ");
    Serial.println(dpID);
  }
}

void SmokeDetectionStatus(int typeData, int featureProductLength, int featureProduct){
  if(featureProduct == 0x00){
    fire = 1;
    Serial.println("Bao dong co khoi");
    //client.publish(mqtt_publish_smoke,"Báo động có khói");
  } 
  else if (featureProduct == 0x01){
    fire = 0;
    Serial.println("xoa canh bao khoi");
    //client.publish(mqtt_publish_smoke,"Đã xóa cảnh báo khói");
  } 
  else{
    //client.publish(mqtt_publish_smoke,"Yêu cầu gửi giá trị cảnh báo khói");
  }  
}

void errorMessage(int typeData, int featureProductLength, int featureProduct){
  if(featureProduct == 0x00){
    Serial.println("Canh bao loi");
    //client.publish(mqtt_publish_smoke,"Cảnh báo lỗi");
  } 
  else if (featureProduct == 0x01){
    Serial.println("Giai phong canh bao");
    //client.publish(mqtt_publish_smoke,"Giải phóng cảnh báo lỗi");
  } 
  else{
    //client.publish(mqtt_publish_smoke,"Yêu cầu gửi giá trị Cảnh báo lỗi");
  }
}

void batteryStatus(int typeData, int featureProductLength, int featureProduct){
  pin = "";
  if(featureProduct == 0x00){
    pin += '"';
    pin += "low";
    pin += '"';
    Serial.print("Trang thai pin yeu");
    //client.publish(mqtt_publish_smoke,"Trạng thái pin yếu");
  } 
  else if (featureProduct == 0x01){
    pin += '"';
    pin += "middle";
    pin += '"';
    Serial.print("Trang thai pin trung binh");
    //client.publish(mqtt_publish_smoke,"Trạng thái pin trung bình");
  } 
  else if (featureProduct == 0x02){
    pin += '"';
    pin += "high";
    pin += '"';
    Serial.print("Trang thai pin cao");
    //client.publish(mqtt_publish_smoke,"Trạng thái pin cao");
  }
  else{
    Serial.print("Trang thai pin error");
    //client.publish(mqtt_publish_smoke,"Yêu cầu gửi giá trị Trạng thái pin");
  }
}

void alarmOff(int typeData, int featureProductLength, int featureProduct){
  if(featureProduct == 0x00){
    Serial.print("Tat bao dong Off");
    //client.publish(mqtt_publish_smoke,"Tắt báo động Off");
  } 
  else if (featureProduct == 0x01){
    Serial.print("Tat bao dong On");
    //client.publish(mqtt_publish_smoke,"Tắt báo động On");
  } 
  else{
    Serial.print("Bao dong error");
    //client.publish(mqtt_publish_smoke,"Tắt báo động: error");
  }  
}

void lowPressureSmoke(int typeData, int featureProductLength, int featureProduct){
  if(featureProduct == 0x00){
    Serial.print("Khoi ap suat thap Off");
    //client.publish(mqtt_publish_smoke,"Khói áp suất thấp Off");
  } 
  else if (featureProduct == 0x01){
    Serial.print("Khoi ap suat thap On");
    //client.publish(mqtt_publish_smoke,"Khói áp suất thấp On");
  } 
  else{
    char temp[8] = {0x55, 0xAA, 0x00, 0x05, 0x00, 0x01, 0x00, 0x05};
    Serial.write(temp,8);
  }  
}

bool checkSum(){
  int i = 0;
  int sumData = 0;
  int checksum;

  if(bufferUART[3] == 1 && bufferUART[5] > 0){
    Serial.println("\nCheck sum true");
    return true;
  }
  Serial.println();
  while(i<position){
    Serial.print(bufferUART[i],HEX);
    Serial.print(" ");

    if(bufferUART[i] != 0x55 && bufferUART[i] != 0xAA){
      if(i < (position-1)){
        sumData += bufferUART[i];
      }
      else{
        checksum = bufferUART[i] + 1;
      }
    }
    i++;
  }

  if (sumData == checksum){
    Serial.println("\nCheck sum true");
    return true;
  }
  else {
    Serial.println("\nCheck sum false");
    bufferUART[0] = 0x55;
    return false;
  }
}

