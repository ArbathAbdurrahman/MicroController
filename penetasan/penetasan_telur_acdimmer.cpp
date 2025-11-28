#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <RBDdimmer.h>
#include <EEPROM.h>

// ================= DHT =================
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);


float lastTemp = 0;
float lastHum  = 0;

// ================= AC DIMMER =================
#define ZC_PIN 32
#define DIM_PIN 33
dimmerLamp dimmer(DIM_PIN, ZC_PIN);

// ============= RELAY TIMER =============
#define RELAY_PIN 2
bool relayTimerEnabled = false;
unsigned long relayOnDuration = 0;
unsigned long relayInterval = 0;
unsigned long lastRelayRun = 0;
unsigned long relayTurnOffTime = 0;
bool relayCurrentlyOn = false;

// ================ LCD =================
LiquidCrystal_I2C lcd(0x27 , 16, 2);

// ================ WIFI & MQTT =================
WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "SSID";
const char* password = "PASS";

const char* mqtt_server = "IP";
const int   mqtt_port   = 1883;
const char* mqtt_user   = "USER";
const char* mqtt_pass   = "PASS";

const char* topic_status  = "topic/penetasan/status";
const char* topic_command = "topic/penetasan/command";

// =============== VAR TEMPERATURE ===============
float targetTemp = 37.5;
float Kp = 40;
int dimmerPower = 0;

// =============== BUZZER ========================
#define BUZZER 13
bool buzzerActive = false;

// =============== NON-BLOCK TIMERS ===============
unsigned long lastSensorRead = 0;
unsigned long lastLCD = 0;
unsigned long lastMQTTPublish = 0;
unsigned long lastMQTTReconnectAttempt = 0;

// =============== EEPROM SAVE ===================
void saveSettings() {
  EEPROM.put(0, targetTemp);
  EEPROM.put(4, relayOnDuration);
  EEPROM.put(8, relayInterval);
  EEPROM.commit();
}

void loadSettings() {
  EEPROM.get(0, targetTemp);
  EEPROM.get(4, relayOnDuration);
  EEPROM.get(8, relayInterval);
}

// ==================  MQTT CALLBACK  ==================
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();

  Serial.print("MQTT Command: ");
  Serial.println(msg);

  if (msg.indexOf("\"SET\"") >= 0) {
    int s = msg.indexOf(":") + 2;
    int e = msg.indexOf("\"", s);
    targetTemp = msg.substring(s, e).toFloat();
    saveSettings();
  }

  if (msg.indexOf("\"BUZZER\":\"ON\"") >= 0) {
    buzzerActive = true;
    digitalWrite(BUZZER, HIGH);
  }
  if (msg.indexOf("\"BUZZER\":\"OFF\"") >= 0) {
    buzzerActive = false;
    digitalWrite(BUZZER, LOW);
  }

  if (msg.indexOf("\"RT_ON\"") >= 0) {
    int s = msg.indexOf(":") + 2;
    int e = msg.indexOf("\"", s);
    relayOnDuration = msg.substring(s, e).toInt() * 1000;
    relayTimerEnabled = true;
    saveSettings();
  }

  if (msg.indexOf("\"RT_INT\"") >= 0) {
    int s = msg.indexOf("\"RT_INT\"") + 10;
    int e = msg.indexOf("\"", s);
    relayInterval = msg.substring(s, e).toInt() * 3600000;
    relayTimerEnabled = true;
    saveSettings();
  }
}

// ================= WIFI Auto Reconnect =================
void handleWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Lost. Reconnecting...");
    WiFi.begin(ssid, password);
  }
}

// ================= MQTT Auto Reconnect =================
void handleMQTT() {
  unsigned long now = millis();
  if (client.connected()) return;

  if (now - lastMQTTReconnectAttempt < 10000) return; // attempt every 10 sec
  lastMQTTReconnectAttempt = now;

  Serial.println("Reconnecting MQTT...");
  if (client.connect("ESP_DIMMER", mqtt_user, mqtt_pass)) {
    client.subscribe(topic_command);
    Serial.println("MQTT Reconnected.");
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  EEPROM.begin(32);
  loadSettings();

  dht.begin();
  lcd.init();
  lcd.backlight();

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  dimmer.begin(NORMAL_MODE, ON);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  WiFi.begin(ssid, password);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// ================= MAIN LOOP =================
void loop() {
  handleWiFi();
  handleMQTT();
  client.loop();

  unsigned long now = millis();

  // ================== BACA SENSOR / PID setiap 2 detik ==================
  if (now - lastSensorRead >= 2000) {
      lastSensorRead = now;

      float h = dht.readHumidity();
      float t = dht.readTemperature();

      if (!isnan(t)) {
          lastTemp = t;
          lastHum = h;

          float error = targetTemp - t;
          dimmerPower = constrain(error * Kp, 0, 100);
          dimmer.setPower(dimmerPower);
      }
  }

  // ================== UPDATE LCD setiap 2 detik ==================
  if (now - lastLCD >= 2000) {
      lastLCD = now;

      lcd.setCursor(0, 0);
      lcd.print("Suhu:");
      lcd.print(lastTemp, 1);
      lcd.write(223);
      lcd.print("C   ");   // hapus karakter sisa

      lcd.setCursor(0, 1);
      lcd.print("Hum :");
      lcd.print(lastHum, 1);
      lcd.print("%   ");
  }

  // ================= PUBLISH STATUS setiap 5 detik ==================
  if (now - lastMQTTPublish >= 5000 && client.connected()) {
    lastMQTTPublish = now;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    String payload = "{";
    payload += "\"suhu\":" + String(t) + ",";
    payload += "\"kelembaban\":" + String(h) + ",";
    payload += "\"power\":" + String(dimmerPower) + ",";
    payload += "\"SET\":" + String(targetTemp);
    payload += "}";

    client.publish(topic_status, payload.c_str());
  }

  // ================= RELAY TIMER LOGIC =================
  if (relayTimerEnabled) {

    // Start ON
    if (!relayCurrentlyOn && (now - lastRelayRun >= relayInterval)) {
      digitalWrite(RELAY_PIN, HIGH);
      relayCurrentlyOn = true;
      relayTurnOffTime = now + relayOnDuration;
    }

    // Auto OFF
    if (relayCurrentlyOn && now >= relayTurnOffTime) {
      digitalWrite(RELAY_PIN, LOW);
      relayCurrentlyOn = false;
      lastRelayRun = now;
    }
  }
}

/* ====== COMMAND ====== 

{"SET":"37.8"}

{"BUZZER":"ON"}
{"BUZZER":"OFF"}

{"RT_ON": "10", "RT_INT": "3"}

*/
