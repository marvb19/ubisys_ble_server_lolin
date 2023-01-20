#include <Arduino.h>

//BLE Libs
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLE2902.h"


//I2C Libs
#include <Wire.h>
//i2c Slave Address
#define I2C_DEV_ADDR 0x55
#define SCL 4
#define SDA 0
int scl_pin = 4;
int sda_pin = 0;


//#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
//#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define SERVICE_UUID        "12345678-9abc-def0-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "12345678-9abc-def0-1234-56789abcdef1"

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;

boolean deviceConnected = false;
boolean oldDeviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      BLEDevice::startAdvertising();
    }
    void onWrite(BLECharacteristic *pCharacteristic){
      std::string value = pCharacteristic->getValue();
      //std::string value = pCharacteristic->setValue(Wire.read());
      Serial.println("MyCallback");
    }
    
};

//i2c stuff
uint32_t i = 0;
char value;

void onRequest(){
  //Wire.print(i++);
  //Wire.print(" Packets.");
  Serial.println("onRequest");
}

void onReceive(int len){
  Serial.printf("onReceive[%d]: ", len);
  while(Wire.available()){
    //Serial.write(Wire.read());
    value = Wire.read();
    //Serial.print(value);
  }
  Serial.println(value);

  if (deviceConnected) {
    //TODO

    int send_val = static_cast<int>(value);
    pCharacteristic->setValue(send_val);
    Serial.print("NOTIFY: ");
    Serial.println(send_val);

    pCharacteristic->notify();
    delay(50); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
  }

  if (!deviceConnected && oldDeviceConnected) {
        delay(1000); // give the bluetooth stack the chance to get things ready
        BLEDevice::startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
      // do stuff here on connecting 
      oldDeviceConnected = deviceConnected;
    }
}


void setup()
{
  Serial.begin(115200);

  //Setting up BLE
  Serial.println("Starting BLE Server!");

  BLEDevice::init("ESP32-BLE-Server");
  pServer = BLEDevice::createServer();
  pService = pServer->createService(SERVICE_UUID);
  pServer->setCallbacks(new MyServerCallbacks());
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );

  pCharacteristic->setValue("0");
  
  //pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in the Client!");


  //Setting up i2c
  Serial.setDebugOutput(true);
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
  TwoWire Wire = TwoWire(0);
  Wire.begin(SDA, SCL);
  Wire.beginTransmission((uint8_t)I2C_DEV_ADDR);

  #if CONFIG_IDF_TARGET_ESP32
    char message[64];
    snprintf(message, 64, "%u Packets.", i++);
    Wire.slaveWrite((uint8_t *)message, strlen(message));
  #endif



}

void loop()
{
  
}
