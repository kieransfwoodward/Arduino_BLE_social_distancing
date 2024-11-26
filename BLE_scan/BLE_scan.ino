#include "config.h"
TTGOClass *watch;
TTGOClass *ttgo;
TTGOClass *ttgo1;
AXP20X_Class *power;

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include "time.h"
//#include <M5StickC.h>


int motorPin = 26;
int scanTime = 5;
BLEScan* pBLEScan;
int contacts[10];
int pointer = 0;
bool exist = false;
String number = "02";
String blname = "NTU" + number;
int previous = 0;
int pos = 0;
int avgRSSI[10];
int closeCount = 0;


float b, c = 0;
int battery = 0;





const int   daylightOffset_sec = 0;
const long  gmtOffset_sec = 3600;
const char* ntpServer = "europe.pool.ntp.org";
bool irq = false;

#define FIREBASE_HOST "esp32-firebase-project.firebaseio.com"
#define FIREBASE_AUTH "iog1UMsVfB4szh5SWRJOuHTOq2oRbGFm0XP5OozM"
//#define WIFI_SSID "ntu-PSKconnect"
//#define WIFI_PASSWORD "GyPKJNBUdeLqbrpDyCE5LFRE"

#define WIFI_SSID "belkin.59fa"
#define WIFI_PASSWORD "94be76fcKW"

FirebaseData firebaseData;
FirebaseJson json;





#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331915b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

//class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
//    void onResult(BLEAdvertisedDevice advertisedDevice) {
//      //  Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
//    }
//};

void setup() {
  Serial.begin(115200);
  Serial.println("Scanning...");
  pinMode(motorPin, OUTPUT );

  setCpuFrequencyMhz(80);


  ttgo = TTGOClass::getWatch();
  ttgo->begin();
  ttgo->openBL();
   power = ttgo->power;

 pinMode(AXP202_INT, INPUT_PULLUP);
    attachInterrupt(AXP202_INT, [] {
        irq = true;
    }, FALLING);

    //!Clear IRQ unprocessed  first
    ttgo->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ, true);
    ttgo->power->clearIRQ();


  //! begin motor attach to 33 pin , In TWatch-2020 it is IO4
  ttgo->motor_begin();

ttgo->motor->onec();

 ///  ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
  //ttgo->tft->drawString("Please touch screen",  50, 100, 2);

  delay(500);
  // watch->displaySleep();
  //watch->powerOff();

  //attach touch screen interrupt pin
  pinMode(TP_INT, INPUT);


//power->setPowerOutPut(AXP202_LDO2, false);

  BLEDevice::init("NTU8");

  //  pBLEScan = BLEDevice::getScan(); //create new scan
  //  // pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  //  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  //  pBLEScan->setInterval(100);
  //  pBLEScan->setWindow(99);  // less or equal setInterval value

  connectWifi();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getLocalTime();
  disconnectWifi();


  // M5.begin();
  //M5.Lcd.fillScreen(GREEN);
  //   M5.Axp.ScreenBreath(6);


}
void batteryLevel() {
  // c = M5.Axp.GetVapsData() * 1.4 / 1000;
  // b = M5.Axp.GetVbatData() * 1.1 / 1000;
  battery = ((b - 3.0) / 1.2) * 100;


  Serial.println(battery);
}

void loop() {
 // batteryLevel();


  int per = ttgo->power->getBattPercentage();


  if(per<10){
    ttgo->displayWakeup();
    ttgo->openBL();
    ttgo->setBrightness(50);
    ttgo->tft->fillScreen(TFT_YELLOW);
  }






if (irq) {
        irq = false;
        ttgo->power->readIRQ();
        if (ttgo->power->isPEKShortPressIRQ()) {
            ttgo->displayWakeup();
    ttgo->openBL();
    ttgo->setBrightness(50);
    ttgo->tft->fillScreen(TFT_GREEN);
    Serial.println("button pressed");
    delay(1000);
        }
        ttgo->power->clearIRQ();
    }


    

  if (closeCount == 0) {
    // M5.Axp.ScreenBreath(6);
    //M5.Lcd.fillScreen(GREEN);
 //   ttgo->tft->fillScreen(TFT_BLACK);

ttgo->closeBL();
ttgo->displaySleep();


    
   // ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
  //  int per = ttgo->power->getBattPercentage();
  //  String perc = String(per);

 // ttgo->tft->drawString(perc,  50, 100, 2);

  }
  else {
    // M5.Axp.ScreenBreath(12);
    //  M5.Lcd.fillScreen(RED);
    ttgo->displayWakeup();
    ttgo->openBL();
    ttgo->setBrightness(50);
    ttgo->tft->fillScreen(TFT_RED);
  }
  closeCount = 0;


  if (pointer != 0) {
    for (int i = 0; i < pointer; i++) {
      Serial.println("i");
      Serial.println(i);
      Serial.println("pointer");
      Serial.println(pointer);
      Serial.println("previous");
      Serial.println(previous);
      Serial.println("contact");
      Serial.println(contacts[i]);
      if (contacts[i] != previous) {
        Serial.println("WIFI");
      //  connectWifi();
        upload(contacts[i], getLocalTime(), avgRSSI[i]);
        disconnectWifi();
        previous = contacts[i];

      }
    }
    pointer = 0;
  }

  Serial.println(esp_get_free_heap_size());

  BLEDevice::init("NTU8");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  //  pAdvertising->setScanResponse(true);
  // pAdvertising->setMinPreferred(0x06);
  // pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();





  pBLEScan = BLEDevice::getScan();
  // scan->setActiveScan(true); //faster results but more power
  BLEScanResults results = pBLEScan->start(1, false);




 // delay(5000);

  for (int i = 0; i < results.getCount(); i++)
  {
    BLEAdvertisedDevice device = results.getDevice(i);
    String BLname = device.getName().c_str();

    if (BLname.substring(0, 3) == "NTU") {


      int rssi = device.getRSSI();
      Serial.print("   Device Name:  ");
      Serial.print(BLname);
      Serial.print("  RSSI: ");
      Serial.print(rssi);
      Serial.println(" ");


      if (rssi > -52) { //was 50 with esp32 dev board
        closeCount++;


        //delay(2000);
        contacts[pointer] = BLname.substring(3, 5).toInt();
        avgRSSI[pointer] = rssi;
     //   if (BLname.substring(3, 5).toInt() != previous) {
          //    M5.Axp.ScreenBreath(12);

          // M5.Lcd.fillScreen(RED);
              ttgo->displayWakeup();
    ttgo->openBL();
    ttgo->setBrightness(100);
    ttgo->tft->fillScreen(TFT_RED);

          ttgo->motor->onec();


      //  }
       // pointer = pointer + 1;

    }

  }


  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  pBLEScan->stop();
  pAdvertising->stop();

  pService->stop();
  // pServer->removeService(pService);

  delete pService;
  delete pServer;
  delete pCharacteristic;

  // delay(2000);
  BLEDevice::getAdvertising()->stop();
  BLEDevice::deinit(false);

}
}


void connectWifi() {



  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  int count = 0;
  while (WiFi.status() != WL_CONNECTED && count<10)
  {

    //ttgo->motor->onec();

    Serial.print(".");
    delay(300);
    count++;
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println();
}

void disconnectWifi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);

}

void upload(int Vrdata, String timestamp, int rssi) {
  Firebase.begin("https://socialdistance-68d6f.firebaseio.com/", "iM35q1aIJcbiRRa7dEmoWNlyLhGchRwVBeXYJPFr");
  Firebase.setMaxRetry(firebaseData, 3);


  int Sdata = map(Vrdata, 0, 200, 0, 200);
  Serial.println("upload data");
  Serial.println(Sdata);

  String combined = timestamp + rssi;


  json.set(combined, Sdata);

  String user =  "/8";

  if (Firebase.updateNode(firebaseData, user, json)) {

    Serial.println(firebaseData.dataPath());

    Serial.println(firebaseData.dataType());

    Serial.println(firebaseData.jsonString());

  } else {
    Serial.println(firebaseData.errorReason());
  }


}

String getLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
  }

  char timeStringBuff[50]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "%B %d %Y %H:%M:%S", &timeinfo);


  Serial.println(timeStringBuff);

  //Optional: Construct String object
  String timestamp(timeStringBuff);

  return timestamp;
}
