#include <Arduino_APDS9960.h>
#include <Arduino_HTS221.h>
#include <Arduino_LPS22HB.h>
#include <Arduino_LSM9DS1.h>
#include <ArduinoBLE.h>

 #define GREEN 23
 #define RED 22

BLEService                     service                 ("181A");
BLEIntCharacteristic        pressureCharacteristic     ("2A6D", BLENotify);

//#define DEBUG True
// String to calculate the local and device name
String name;
unsigned long lastNotify = 0;

void printSerialMsg(const char * msg) {
  #ifdef DEBUG
  if (Serial) {
    Serial.println(msg);
  }
  #endif
}

void blinkLoop(int led) {
  

  while (1) {
    digitalWrite(led, !HIGH);
    delay(500);
    digitalWrite(led, !LOW);
    delay(500);
    if (led == GREEN){
      break;
    }
  }
}

void setup() {
  #ifdef DEBUG
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Started");
  #endif
  
  pinMode(GREEN, OUTPUT);
  pinMode(RED, OUTPUT);

  digitalWrite(GREEN,HIGH);
  digitalWrite(RED,HIGH);

  

  if (!BARO.begin()) {
    printSerialMsg("Failed to initialized BARO!");
    blinkLoop(RED);
  }
  blinkLoop(GREEN);

  if (!BLE.begin()) {
    printSerialMsg("Failed to initialized BLE!");
    blinkLoop(RED);
  }
  blinkLoop(GREEN);



  String address = BLE.address();
  #ifdef DEBUG
  if (Serial) {
    Serial.print("address = ");
    Serial.println(address);
  }
  #endif
  address.toUpperCase();

  name = "BLE sensors - ";
  name += address[address.length() - 5];
  name += address[address.length() - 4];
  name += address[address.length() - 2];
  name += address[address.length() - 1];

  #ifdef DEBUG
  if (Serial) {
    Serial.print("name = ");
    Serial.println(name);
  }
  #endif

  BLE.setLocalName(name.c_str());
  BLE.setDeviceName(name.c_str());
  BLE.setAdvertisedService(service);

  
  service.addCharacteristic(pressureCharacteristic);

  BLE.addService(service);
  BLE.advertise();
 
}

void updateSubscribedCharacteristics() {
  if (pressureCharacteristic.subscribed()) {
    float pressure = BARO.readPressure();
    int pPressure=pressure*10000;
    Serial.print("P: ");
    Serial.println(pPressure);
    digitalWrite(GREEN, !digitalRead(GREEN));
    Serial.println(pressureCharacteristic.writeValue(pPressure));
  }
 
}

void loop() {
  BLE.poll(100);
  while (BLE.connected()) {
    updateSubscribedCharacteristics();
    delay(500);
  }
}
