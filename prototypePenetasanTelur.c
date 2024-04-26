#include "CTBot.h" // mengunduh library CTbot
#include <Wire.h> // lib i2c
#include <LiquidCrystal_I2C.h> // lib LCD 16x2 i2c
#include <DHT.h> // lib DHT22
#include <Arduino.h> // arduino json v5/6


#define DHTPIN 4     // Pin tempat Anda menghubungkan output sensor DHT22
#define DHTTYPE DHT22   // Tipe sensor DHT (DHT11, DHT21, DHT22)

CTBot lele; // variable bot
const int ledPin1 = 23; // Tentukan pin LED yang akan digunakan
const int ledPin2 = 5; // Pin 3v3
const int relay1 = 18; // Pin Relay 1
const int relay2 = 19; // Pin Relay 2

DHT dht(DHTPIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Alamat I2C dan ukuran LCD


void setup() {
  Serial.begin(115200);
  dht.begin();
  lcd.init();                      // Inisialisasi LCD
  lcd.backlight();                // Nyalakan backlight LCD
  lcd.setCursor(0, 1);
  lcd.print("Hum  :"); 
  lcd.setCursor(0, 0);
  lcd.print("Suhu :");

  pinMode(ledPin1, OUTPUT); // Atur pin sebagai output 1
  digitalWrite(ledPin1,LOW);
  pinMode(ledPin2, OUTPUT); // Atur pin sebagai output 2
  digitalWrite(ledPin2,LOW);
  pinMode(relay1, OUTPUT); // Atur pin sebagai output relay 1
  digitalWrite(relay1,LOW);
  pinMode(relay2, OUTPUT); // Atur pin sebagai output relay 2
  digitalWrite(relay2,LOW);
  lele.wifiConnect("SSID wifi", "password");
  lele.setTelegramToken("token bot telegram");

}

void loop() {
  delay(2000);  // Tunggu 2 detik untuk membaca ulang sensor
  
  // Baca kelembapan
  float humidity = dht.readHumidity();
  // Baca suhu dalam Celsius
  float temperatureC = dht.readTemperature();
  // Baca suhu dalam Fahrenheit
  float temperatureF = dht.readTemperature(true);

  // Tampilkan pembacaan di LCD
  lcd.setCursor(7, 1);
  lcd.print(humidity);
  lcd.print(" %");
  lcd.setCursor(7, 0);
  lcd.print(temperatureC);
  lcd.print(" C");

  TBMessage pesan;

  if(lele.getNewMessage(pesan)){
    Serial.print("Ada pesan Masuk : ");
    Serial.println(pesan.text);
    if(pesan.text.equalsIgnoreCase("ON1")){
      digitalWrite(ledPin1,HIGH);
      lele.sendMessage(pesan.sender.id,"LED 1 Menyala");
    }
    else if(pesan.text.equalsIgnoreCase("OFF1")){
      digitalWrite(ledPin1,LOW);
      lele.sendMessage(pesan.sender.id,"LED 1 Padam");
    }
    else if(pesan.text.equalsIgnoreCase("ON2")){
      digitalWrite(ledPin2,HIGH);
      lele.sendMessage(pesan.sender.id,"LED 2 Menyala");
    }
    else if(pesan.text.equalsIgnoreCase("OFF2")){
      digitalWrite(ledPin2,LOW);
      lele.sendMessage(pesan.sender.id,"LED 2 Padam");
    }
    else if(pesan.text.equalsIgnoreCase("ONR1")){
      digitalWrite(relay1,HIGH);
      lele.sendMessage(pesan.sender.id,"Relay 1 Menyala");
    }
    else if(pesan.text.equalsIgnoreCase("OFFR1")){
      digitalWrite(relay1,LOW);
      lele.sendMessage(pesan.sender.id,"Relay 1 Padam");
    }
    else if(pesan.text.equalsIgnoreCase("ONR2")){
      digitalWrite(relay2,HIGH);
      lele.sendMessage(pesan.sender.id,"Relay 2 Menyala");
    }
    else if(pesan.text.equalsIgnoreCase("OFFR2")){
      digitalWrite(relay2,LOW);
      lele.sendMessage(pesan.sender.id,"Relay 2 Padam");
    }
    else if(pesan.text.equalsIgnoreCase("SUHU")){
      String suhu = String(temperatureC + "C");
      lele.sendMessage(pesan.sender.id,suhu);
    }
    else if(pesan.text.equalsIgnoreCase("HUM")){
      String kelembaban = String(humidity + "%");
      lele.sendMessage(pesan.sender.id,kelembaban);
    }
    else{
      String balas;
      balas="Maaf, perintahnya salah. Coba kirim ON,OFF,ONR,OFFR,SUHU,HUM,STATUS.";
      lele.sendMessage(pesan.sender.id,balas);
    }
  }

}
