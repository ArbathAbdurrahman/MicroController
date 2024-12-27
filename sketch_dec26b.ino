// Pin untuk Sensor Turbidity
const int turbidityPin = 34;  // Pin Analog ESP32

void setup() {
  // Inisialisasi Serial
  Serial.begin(115200);
}

void loop() {
  // Membaca nilai analog dari sensor
  int turbidityValue = analogRead(turbidityPin);
  
  // Konversi ke voltage
  float voltage = turbidityValue * (3.3 / 4095.0);
  
  // Tampilkan nilai
  Serial.print("Raw Value: ");
  Serial.print(turbidityValue);
  Serial.print("\tVoltage: ");
  Serial.print(voltage, 3);  // Menampilkan 3 angka desimal
  Serial.println(" V");
  
  delay(1000);  // Delay 1 detik
}