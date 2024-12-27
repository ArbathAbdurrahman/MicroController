#define BLYNK_TEMPLATE_ID "TMPL6vYqy6wlS"
#define BLYNK_TEMPLATE_NAME "kejernihan"
#define BLYNK_AUTH_TOKEN "g6r7JXVnEYcwXuOQP7Qud-8V-w8BY8oA"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

#define DIGITAL_PIN 25  // Pin digital yang digunakan (contoh: GPIO25)

char ssid[] = "mintorejo";       // Ganti dengan nama WiFi Anda
char pass[] = "fariz123";   // Ganti dengan password WiFi Anda

BlynkTimer timer;

void checkWaterCondition() {
  int digitalValue = digitalRead(DIGITAL_PIN);  // Membaca nilai digital (HIGH/LOW)

  if (digitalValue != HIGH) {
    Serial.println("Kondisi Air = Keruh");
    Blynk.virtualWrite(V1, "Keruh"); // Kirim status ke Virtual Pin V1
  } else {
    Serial.println("Kondisi Air = Jernih");
    Blynk.virtualWrite(V1, "Jernih"); // Kirim status ke Virtual Pin V1
  }
}

void setup() {
  pinMode(DIGITAL_PIN, INPUT);  // Atur pin sebagai input
  Serial.begin(115200);        // Inisialisasi komunikasi serial
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected");
  } else {
    Serial.println("WiFi Connection Failed");
  }

  // Atur timer untuk membaca data setiap 2 detik
  timer.setInterval(2000L, checkWaterCondition);
}

void loop() {
  Blynk.run();
  timer.run();  // Jalankan timer
}
