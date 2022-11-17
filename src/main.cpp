#include <Arduino_APDS9960.h>
#include <Arduino_HTS221.h>
#include <Arduino_LPS22HB.h>
#include <Arduino_LSM9DS1.h>
#include <ArduinoBLE.h>

const int VERSION = 0x00000001;
const float TEMPERATURE_CALIBRATION = -5.0;

float slpALT = 1013.25;

BLEService                     service                 (("181A"));
BLEUnsignedIntCharacteristic   slpCorCharacteristic    (("0001"), BLEWrite);
BLEIntCharacteristic        temperatureCharacteristic  (("2A6E"), BLENotify);
BLEIntCharacteristic        pressureCharacteristic     (("2A6D"), BLENotify);
BLEIntCharacteristic         humidityCharacteristic    (("2A6F"), BLENotify);
BLEIntCharacteristic         altitudeCharacteristic    (("2A6C"), BLENotify);

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

void blinkLoop() {
  while (1) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }
}

void setup() {
  #ifdef DEBUG
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Started");
  #endif

  delay(2000);

  if (!HTS.begin()) {
    printSerialMsg("Failed to initialized HTS!");
    blinkLoop();
  }

  if (!BARO.begin()) {
    printSerialMsg("Failed to initialized BARO!");
    blinkLoop();
  }
  
  if (!BLE.begin()) {
    printSerialMsg("Failed to initialized BLE!");
    blinkLoop();
  }

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

  service.addCharacteristic(altitudeCharacteristic);
  service.addCharacteristic(pressureCharacteristic);
  service.addCharacteristic(humidityCharacteristic);
  service.addCharacteristic(temperatureCharacteristic);
  service.addCharacteristic(slpCorCharacteristic);
  slpCorCharacteristic.setValue(VERSION);

  BLE.addService(service);
  BLE.advertise();
}


float readAltitude(float seaLevel,float pres) {
  return 44330.0 * (1.0 - pow(pres / seaLevel, 0.1903));
}

void updateSubscribedCharacteristics() {
  bool doTemperature = temperatureCharacteristic.subscribed();
  bool doHumidity = humidityCharacteristic.subscribed();
  if (doTemperature || doHumidity) {
    float temperature = HTS.readTemperature();
    float temperatureCalibrated = temperature + TEMPERATURE_CALIBRATION;
    if (doTemperature) {
      int tTemperature = temperatureCalibrated*100;
      Serial.print("T: ");
      Serial.println(tTemperature);
      temperatureCharacteristic.writeValue(tTemperature);
    }
    if (doHumidity) {
      float humidity = HTS.readHumidity();
      float dp = temperature - ((100.0 - humidity) / 5.0);
      float humidityCalibrated = (100.0 - (5.0 * (temperatureCalibrated - dp)));
      Serial.print("H: ");
      int hHumidity=humidityCalibrated*100;
      Serial.println(hHumidity);
      humidityCharacteristic.writeValue(hHumidity);
    }
  }
  if (pressureCharacteristic.subscribed()) {
    float pressure = BARO.readPressure();
    int pPressure=pressure*10000;
    Serial.print("P: ");
    Serial.println(pPressure);
    pressureCharacteristic.writeValue(pPressure);
  }
  if (slpCorCharacteristic.written()) {
        int rawSLP = slpCorCharacteristic.value();    // any value other than 0
        slpALT=rawSLP/100.0F;
        Serial.println(slpALT);
  }

   if (altitudeCharacteristic.subscribed()) {
    float prs = BARO.readPressure()*10.F;
    Serial.println(prs);
    float alti= readAltitude(slpALT,prs)*100;
    Serial.print("A: ");
    Serial.println(alti);
    altitudeCharacteristic.writeValue(alti);
  }
}

void loop() {
  BLE.poll(1000);
  while (BLE.connected()) {
    updateSubscribedCharacteristics();
    delay(500);
  }
}
