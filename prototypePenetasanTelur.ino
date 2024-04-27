#include "CTBot.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Arduino.h>


#define DHTPIN 4     // Pin tempat Anda menghubungkan output sensor DHT22
#define DHTTYPE DHT22   // Tipe sensor DHT (DHT11, DHT21, DHT22)

CTBot lele;
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
  lele.wifiConnect("SSID WI-FI", "PASSWORD");
  lele.setTelegramToken("TOKEN BOT TELEGRAM");

}

void loop() {
  TBMessage pesan;
  // Baca kelembapan
  float humidity = dht.readHumidity();
  // Baca suhu dalam Celsius
  float temperatureC = dht.readTemperature();
  float minimal = 36.00;
  float maksimal = 39.00;
  float hminimal = 60.00;

  // Tampilkan pembacaan di LCD
  lcd.setCursor(7, 1);
  lcd.print(humidity);
  lcd.print(" %");
  lcd.setCursor(7, 0);
  lcd.print(temperatureC);
  lcd.print(" C");

  delay(2000);  // Tunggu 2 detik untuk membaca ulang sensor

  // Membuat laporan "STATUS"
  String sled1 = String("LED 1 \t= EROR\n");
  String sled2 = String("LED 2 \t= EROR\n");
  String srelay1 = String("Relay 1 \t= EROR\n");
  String srelay2 = String("Relay 2 \t= EROR\n");
  String suhu = String("Suhu \t= ");
  String celcius = String("°C\n");
  String celembaban = String("Kelembaban \t= ");
  String persen = String("%");

  // Kondisi "STATUS"
  if (digitalRead(ledPin1) == HIGH ){
    sled1 = "LED 1 \t= ON\n";
  }
  else{
    sled1 = "LED 1 \t= OFF\n";
  }
  if (digitalRead(ledPin2) == HIGH ){
    sled2 = "LED 2 \t= ON\n";
  }
  else{
    sled2 = "LED 2 \t= OFF\n";
  }
  if (digitalRead(relay1) == HIGH ){
    srelay1 = "Relay 1 \t= ON\n";
  }
  else{
    srelay1 = "Relay 1 \t= OFF\n";
  }
  if (digitalRead(relay2) == HIGH ){
    srelay2 = "Relay 2 \t= ON\n";
  }
  else{
    srelay2 = "Relay 2 \t= OFF\n";
  }

  // Kondisikan LED berdasarkan suhu
  if (temperatureC > maksimal){
    digitalWrite(relay1, LOW); // Matikan Lampu jika suhu lebih dari 39°C
  }
  else if(temperatureC < minimal){
    digitalWrite(relay1, HIGH); // Hidupkan Lampu jika suhu kurang dari 36°C 
  }
  else{
    digitalWrite(ledPin2, HIGH); // Hidupkan Inikator 2 jika suhu kurang dari 36°C 
  }
  if(humidity < hminimal){
    digitalWrite(relay2, HIGH); // Hidupkan Indikator 1 jika kelembaban kurang dari 60% atau kurang dari dari 60%
  }
  else{
    digitalWrite(relay2, LOW); //  Matikan Indikator 1 jika kelembaban lebih dari 60%
  }

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
      String suhu = String(temperatureC + celcius);
      lele.sendMessage(pesan.sender.id,suhu);
    }
    else if(pesan.text.equalsIgnoreCase("HUM")){
      String kelembaban = String(humidity + persen);
      lele.sendMessage(pesan.sender.id,kelembaban);
    }
    else if(pesan.text.equalsIgnoreCase("STATUS")){
      String status = String(sled1 + sled2 + srelay1 + srelay2 + suhu + temperatureC + celcius + celembaban + humidity + persen);
      lele.sendMessage(pesan.sender.id,status);
    }
    else{
      String balas;
      balas="Maaf, perintahnya salah. Coba kirim ON,OFF,ONR,OFFR,SUHU,HUM,STATUS.";
      lele.sendMessage(pesan.sender.id,balas);
    }
  }
}
