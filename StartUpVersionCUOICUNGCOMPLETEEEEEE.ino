// pH configuration
#define SensorPin 34 // Sử dụng chân analog 34 trên ESP32 (ADC1_CH6)
#define Offset 0.00 // Điều chỉnh độ lệch (offset) nếu cần
#define LED 2
#define samplingInterval 20
#define printInterval 800
#define ArrayLenth 40 // Số lần thu thập
int pHArray[ArrayLenth]; // Lưu giá trị trung bình từ cảm biến
int pHArrayIndex = 0;

// BLYNK SETUP
#define BLYNK_TEMPLATE_ID "TMPL6cnaY1JYN"
#define BLYNK_TEMPLATE_NAME "HUPTECH"
#define BLYNK_AUTH_TOKEN "BRaW0BkR7jGxh_bpfd8thxZRb-FyHqsD"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Wifi setup
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Fatlab";
char pass[] = "12345678@!";

// DS18B20 configuration
#define ONE_WIRE_BUS 14
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// LCD configuration
LiquidCrystal_I2C lcd(0x27, 16, 2); // Địa chỉ I2C, kích thước màn hình (16x2)

// Relay configuration
#define RELAY_PIN 4  // Chân D4
bool isRelayOn = false;

float temperature = 0.0; // Biến lưu trữ nhiệt độ

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT); // Khai báo chân relay là OUTPUT
  Serial.begin(9600);
  Serial.println("Thử nghiệm cảm biến pH!");
  Blynk.begin(auth, ssid, pass);
  sensors.begin();

  // Khởi tạo màn hình LCD
  lcd.init();
  lcd.backlight();
}

void loop() {
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static float pHValue, voltage;

  if (millis() - samplingTime > samplingInterval) {
    pHArray[pHArrayIndex++] = analogRead(SensorPin); // Mảng lưu trữ giá trị trung bình của pH
    if (pHArrayIndex == ArrayLenth) pHArrayIndex = 0;
    voltage = avergearray(pHArray, ArrayLenth) * 3.3 / 4095; // ESP32 hoạt động ở 3.3V
    pHValue = 3.5 * voltage + Offset;
    samplingTime = millis();

    // Gửi giá trị pH đến Blynk
    Blynk.virtualWrite(V1, pHValue);
  }

  if (millis() - printTime > printInterval) {
    // Đọc nhiệt độ từ DS18B20
    sensors.requestTemperatures();
    float t = sensors.getTempCByIndex(0);

    // Kiểm tra xem việc đọc nhiệt độ có thành công không
    if (t == DEVICE_DISCONNECTED_C) {
      Serial.println("Không thể đọc từ cảm biến DS18B20!");
      return;
    }

    // Điều khiển relay dựa trên nhiệt độ
    if (t < 30 && !isRelayOn) {
      digitalWrite(RELAY_PIN, HIGH);  // Bật relay
      isRelayOn = true;
      Serial.println("Bật relay");
    } else if (t >= 37 && isRelayOn) {
      digitalWrite(RELAY_PIN, LOW);  // Tắt relay
      isRelayOn = false;
      Serial.println("Tắt relay");
    }

    // Cập nhật giá trị nhiệt độ
    temperature = t;

    // Gửi giá trị nhiệt độ lên chân V0 của Blynk
    Blynk.virtualWrite(V0, temperature);

    // Hiển thị trên màn hình LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("pH:");
    lcd.print(pHValue, 2); // Hiển thị giá trị pH với 2 chữ số thập phân
    lcd.print("|ND:");
    lcd.print(temperature);

    lcd.setCursor(3, 1);
    if (pHValue <= 5.5) {
      lcd.print("HOAN_THANH");
    } else {
      lcd.print("DANG_MUOI");
    }

    printTime = millis();
  }

  Blynk.run();
}

double avergearray(int* arr, int number) {
  int i;
  int max, min;
  double avg;
  long amount = 0;
  if (number <= 0) {
    Serial.println("Lỗi: Số lượng mẫu cần lấy trung bình phải lớn hơn 0!");
    return 0;
  }
  if (number < 5) {
    for (i = 0; i < number; i++) {
      amount += arr[i];
    }
    avg = amount / number;
    return avg;
  } else {
    if (arr[0] < arr[1]) {
      min = arr[0];
      max = arr[1];
    } else {
      min = arr[1];
      max = arr[0];
    }
    for (i = 2; i < number; i++) {
      if (arr[i] < min) {
        amount += min;
        min = arr[i];
      } else {
        if (arr[i] > max) {
          amount += max;
          max = arr[i];
        } else {
          amount += arr[i];
        }
      }
    }
    avg = (double)amount / (number - 2);
  }
  return avg;
}
