#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <LiquidCrystal_I2C.h>

// ==================== KONFIGURASI PIN ====================
#define DHTPIN 2           // GPIO4 (D4)
#define DHTTYPE DHT22
#define RELAY1 14          // GPIO14 (D5)
#define RELAY2 12          // GPIO12 (D6)
#define BUZZER 13         // GPIO13 (D7)
// =========================================================

// ==================== KONFIGURASI LCD ====================
LiquidCrystal_I2C lcd(0x27 , 16, 2); //0x27 0x3F 

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

// ==================== KONFIGURASI WIFI & MQTT ====================
const char* ssid = "SSID";
const char* password = "PASS";

const char* mqtt_server = "IP";   // ganti dengan IP / domain broker
const int mqtt_port = 1884;
const char* mqtt_user = "USER";
const char* mqtt_pass = "PASS";

const char* topic_status  = "topic/penetasan/status";
const char* topic_command = "topic/penetasan/command";
// ================================================================

bool relay1State = false;
bool relay2State = true; //normaly open
bool buzzerActive = false;

// ---------- BATAS SUHU ----------
float tempTop = 38.5;  // default batas atas
float tempBottom = 36.5; // default batas bawah

// --------- RECONNECT ---------
unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 10000; 

// ---------- FUNGSI: koneksi WiFi ----------
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Menghubungkan ke WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Terhubung!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// ---------- CALLBACK: saat ada pesan MQTT ----------
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();

  Serial.print("Pesan diterima di ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);

  if (String(topic) == topic_command) {
    // --- Relay Control ---
    if (message.indexOf("ON1") >= 0) {
      digitalWrite(RELAY1, HIGH);
      relay1State = true;
    } 
    else if (message.indexOf("OFF1") >= 0) {
      digitalWrite(RELAY1, LOW);
      relay1State = false;
    }

    if (message.indexOf("ON2") >= 0) {
      digitalWrite(RELAY2, LOW);
      relay2State = false;
    } 
    else if (message.indexOf("OFF2") >= 0) {
      digitalWrite(RELAY2, HIGH);
      relay2State = true;
    }

    // --- Buzzer manual ---
    if (message.indexOf("BON") >= 0) {
      digitalWrite(BUZZER, HIGH);
      buzzerActive = true;
    } 
    else if (message.indexOf("BOFF") >= 0) {
      digitalWrite(BUZZER, LOW);
      buzzerActive = false;
    }

    // --- Atur batas suhu ---
    if (message.indexOf("\"TT\"") >= 0) {
      int start = message.indexOf(":") + 2;  // cari posisi angka
      int end = message.indexOf("\"", start);
      String val = message.substring(start, end);
      tempTop = val.toFloat();
      Serial.print("Batas suhu atas diubah ke: ");
      Serial.println(tempTop);
    }

    if (message.indexOf("\"TB\"") >= 0) {
      int start = message.indexOf(":") + 2;
      int end = message.indexOf("\"", start);
      String val = message.substring(start, end);
      tempBottom = val.toFloat();
      Serial.print("Batas suhu bawah diubah ke: ");
      Serial.println(tempBottom);
    }

    // --- Kirim status terkini ---
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    String payload = "{";
    payload += "\"suhu\":" + String(t) + ",";
    payload += "\"kelembaban\":" + String(h) + ",";
    payload += "\"relay1\":\"" + String(relay1State ? "ON" : "OFF") + "\",";
    payload += "\"relay2\":\"" + String(relay2State ? "OFF" : "ON") + "\",";
    payload += "\"buzzer\":\"" + String(buzzerActive ? "ON" : "OFF") + "\",";
    payload += "\"TT\":" + String(tempTop) + ",";
    payload += "\"TB\":" + String(tempBottom);
    payload += "}";
    client.publish(topic_status, payload.c_str());
  }
}

// ---------- FUNGSI RECONNECT ----------
bool reconnect() {
  if (client.connect("ESP12E_Client", mqtt_user, mqtt_pass)) {
    Serial.println("MQTT terhubung kembali!");
    client.subscribe(topic_command);
    return true;
  }
  return false;
}


// ---------- SETUP ----------
void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Penetasan Ready");
  delay(2000);
  lcd.clear();

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, HIGH);
  digitalWrite(BUZZER, LOW);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// ---------- LOOP ----------
void loop() {
  // ====== CEK WIFI DAN MQTT TANPA MEMBLOKIR ======
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastWifiAttempt = 0;
    if (millis() - lastWifiAttempt > 10000) {
      Serial.println("WiFi terputus! Mencoba koneksi ulang...");
      WiFi.begin(ssid, password);
      lastWifiAttempt = millis();
    }
  } 
  else if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > reconnectInterval) {
      lastReconnectAttempt = now;
      if (reconnect()) {
        lastReconnectAttempt = 0;
      } else {
        Serial.println("Gagal konek MQTT, lanjutkan offline mode...");
      }
    }
  } else {
    client.loop(); // hanya dijalankan jika MQTT aktif
  }

  // ====== LOGIKA PEMBACAAN SUHU DAN KONTROL ======
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error baca DHT");
    Serial.println("Sensor DHT gagal dibaca!");
    digitalWrite(RELAY2, HIGH);
    relay2State = true;
    Serial.println("Suhu tinggi! Relay 2 OFF.");
    buzzerActive = true;
    digitalWrite(BUZZER, HIGH);
    delay(5000);
    buzzerActive = false;
    digitalWrite(BUZZER, LOW);
    return; // keluar dari loop untuk mencegah data error
  }

  // --- kontrol relay otomatis berdasarkan suhu ---
  if (t > tempTop) {
    digitalWrite(RELAY2, HIGH);
    relay2State = true;
    Serial.println("Suhu tinggi! Relay 2 OFF.");
  } else if (t < tempBottom) {
    digitalWrite(RELAY2, LOW);
    relay2State = false;
    Serial.println("Suhu rendah! Relay 2 ON.");
  }

  // ====== TAMPILKAN DATA DI LCD ======
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Suhu:");
  lcd.print(t, 1);
  lcd.print((char)223); // simbol derajat
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Hum:");
  lcd.print(h, 1);
  lcd.print("%");

  delay(8000); // tampilkan suhu & humidity selama 8 detik

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Top:");
  lcd.print(tempTop, 1);
  lcd.print((char)223);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Bottom:");
  lcd.print(tempBottom, 1);
  lcd.print((char)223);
  lcd.print("C");

  delay(2000); // tampilkan batas suhu selama 2 detik

  // ====== KIRIM STATUS JIKA MQTT AKTIF ======
  if (client.connected()) {
    String payload = "{";
    payload += "\"suhu\":" + String(t) + ",";
    payload += "\"kelembaban\":" + String(h) + ",";
    payload += "\"relay1\":\"" + String(relay1State ? "ON" : "OFF") + "\",";
    payload += "\"relay2\":\"" + String(relay2State ? "OFF" : "ON") + "\",";
    payload += "\"buzzer\":\"" + String(buzzerActive ? "ON" : "OFF") + "\",";
    payload += "\"TT\":" + String(tempTop) + ",";
    payload += "\"TB\":" + String(tempBottom);
    payload += "}";
    client.publish(topic_status, payload.c_str());
  } else {
    // tampilkan ke serial monitor untuk mode offline
    Serial.print("Offline Mode | Suhu: ");
    Serial.print(t);
    Serial.print(" | Kelembaban: ");
    Serial.println(h);
  }
}
