// Select your modem:
// #define TINY_GSM_MODEM_SIM800
#define TINY_GSM_MODEM_SIM808
// #define TINY_GSM_MODEM_SIM900
// #define TINY_GSM_MODEM_A6
// #define TINY_GSM_MODEM_A7
// #define TINY_GSM_MODEM_M590
// #define TINY_GSM_MODEM_ESP8266
// #define TINY_GSM_MODEM_XBEE

#include <TinyGsmClient.h>
#include <PubSubClient.h>

// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn[]  = "FreedomPop";
const char user[] = "";
const char pass[] = "";

// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

// or Software Serial on Uno, Nano
//#include <SoftwareSerial.h>
//SoftwareSerial SerialAT(2, 3); // RX, TX

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

const char* broker = "158.49.112.108";

const char* topicPos = "GsmClient/2/posi";


long lastReconnectAttempt = 0;

void setup() {

  // Set console baud rate
  Serial.begin(115200);
  delay(10);

  // Set GSM module baud rate
  SerialAT.begin(115200);
  delay(3000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  modem.restart();

  String modemInfo = modem.getModemInfo();
  Serial.print("Modem: ");
  Serial.println(modemInfo);

  // Unlock your SIM card with a PIN
  modem.simUnlock("8010");

  Serial.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    Serial.println(" fail");
    while (true);
  }
  Serial.println(" OK");

  Serial.print("Connecting to ");
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println(" fail");
    while (true);
  }
  Serial.println(" OK");

  // MQTT Broker setup
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);
}

boolean mqttConnect() {
  Serial.print("Connecting to ");
  Serial.print(broker);
  if (!mqtt.connect("GsmClientTest")) {
    Serial.println(" fail");
    return false;
  }
  Serial.println(" OK");
  return mqtt.connected();
}

void loop() {

  if (mqtt.connected()) {
    // mqtt.loop();
    
#if defined(TINY_GSM_MODEM_SIM808)
  float gps_lat, gps_lon, gps_speed=0;
  int gps_alt=0, gps_vsat=0, gps_usat=0, gps_year=0, gps_month=0, gps_day=0, gps_hour=0, gps_minute=0, gps_second=0;

  modem.enableGPS();
  DBG("Esperando senal de cobertura.....");
  while (!modem.getGPS(&gps_lat, &gps_lon, &gps_speed, &gps_alt, &gps_vsat, &gps_usat));
  DBG("GPS lat:", gps_lat);
  DBG("GPS lon:", gps_lon);
  DBG("GPS speed:", gps_speed);
  DBG("GPS alt:", gps_alt);
  DBG("GPS vsat:", gps_vsat);
  DBG("GPS usat:", gps_usat);

  modem.disableGPS();
  
#endif
    String s = String(gps_lon)+String(",")+String(gps_lat);
    char buf[s.length()];
    s.toCharArray(buf,s.length());
    uint8_t buff[s.length()];
    memcpy(buff, buf, s.length());
    mqtt.publish(topicPos, buff ,s.length(),true);
  } else {
    // Reconnect every 10 seconds
    unsigned long t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
  }

}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, len);
  Serial.println();

}

