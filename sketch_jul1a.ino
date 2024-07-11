#include <Wire.h>                         // i2C 통신을 위한 라이브러리
#include <LiquidCrystal_I2C.h>            // LCD 1602 I2C용 라이브러리
#include <Servo.h>
#include "DHT.h"
#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>  

#define IR 10
#define IR2 3
#define FAN 11
#define PIEZO 7
#define DHTPIN 8
#define DHTTYPE DHT11
#define NEO 6
#define BT_RXD 5
#define BT_TXD 4
#define SERVO 13
#define ALBE 12
#define LED 9

Servo myServo;
Servo albe;

DHT dht(DHTPIN, DHTTYPE);

Adafruit_NeoPixel RGB_LED = Adafruit_NeoPixel(3, NEO, NEO_GRB);

LiquidCrystal_I2C lcd(0x27, 16, 2);       // 접근주소: 0x3F or 0x27

SoftwareSerial bluetooth(BT_RXD, BT_TXD);

bool albeMoved = true; 
bool fireDetected = false; // 화재 감지 상태를 나타내는 플래그 변수

int count = 1;

void setup() {
  Serial.begin(9600);
  myServo.attach(SERVO);
  albe.attach(ALBE);
  pinMode(IR, INPUT);
  pinMode(IR2, INPUT);
  pinMode(FAN, OUTPUT);
  pinMode(PIEZO, INPUT);
  pinMode(LED, OUTPUT);
  lcd.init();
  
  dht.begin();

  RGB_LED.begin();
  RGB_LED.setBrightness(50); //RGB_LED 밝기조절
  RGB_LED.clear();

  myServo.write(100);

  bluetooth.begin(9600);
  digitalWrite(LED, LOW);
}

void loop() {
  lcd.backlight();

  if (bluetooth.available()) {
    String receivedData = bluetooth.readStringUntil('\n');
    Serial.print("Received: ");
    Serial.println(receivedData);

    if(receivedData == "DOOR" && !fireDetected) { // 화재가 감지되지 않은 경우에만 도어 제어
      if(count % 2 != 0) { 
        myServo.write(0);
        count++; 
      } else {
        myServo.write(100);
        count--;
      }
      Serial.println(count);
    }

    if(receivedData == "ON" && !fireDetected) {
       digitalWrite(FAN, LOW);
    } else if(receivedData == "OFF" && !fireDetected) {
      digitalWrite(FAN, HIGH);
    }

    if(receivedData.length() == 8) {
      int r = strtol(receivedData.substring(2, 4).c_str(), NULL, 16);
      int g = strtol(receivedData.substring(4, 6).c_str(), NULL, 16);
      int b = strtol(receivedData.substring(6, 8).c_str(), NULL, 16);

      for (int i = 0; i < 3; i++) {
        RGB_LED.setPixelColor(i, r, g, b);
      }
      RGB_LED.show();

      Serial.print("R: ");
      Serial.print(r);
      Serial.print(" G: ");
      Serial.print(g);
      Serial.print(" B: ");
      Serial.println(b);
    } else {
      int brightness = receivedData.toInt();
      RGB_LED.setBrightness(brightness);
      RGB_LED.show();
      Serial.print("Brightness: ");
      Serial.println(brightness);
    }
  }

  if (Serial.available()) {
    bluetooth.write(Serial.read());
  }

  int rv = digitalRead(IR);
  int rv2 = digitalRead(IR2);

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  lcd.setCursor(0, 0);
      lcd.print("T : "); lcd.print((int)t); lcd.print("C");
      lcd.setCursor(8, 0);
      lcd.print("H : "); lcd.print((int)h); lcd.print("%");

  if((rv == LOW || rv2 == LOW) && (h >= 50 && t >= 23)) {
    if (!fireDetected) {
      fireDetected = true; // 화재 감지 상태 설정
      myServo.write(0);
      digitalWrite(FAN, LOW);
      for (int i = 0; i < 3; i++) {
        RGB_LED.setPixelColor(i, 255, 0, 0);  // 빨강 출력
      }
      RGB_LED.show();
      tone(PIEZO, 232);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("FIRE DETECTED!!!");
      digitalWrite(LED, HIGH);

      if(albeMoved) {
      albe.write(0);
      delay(5000);
      albe.write(180);
      delay(10000);
      albeMoved = false;
    } 
    }
  } else {
    if (fireDetected) {
      fireDetected = false; // 화재 감지 상태 해제
      myServo.write(100);
      digitalWrite(FAN, HIGH);
      digitalWrite(PIEZO, LOW);
      lcd.clear(); // LCD 화면 지우기
      digitalWrite(LED, LOW);
      noTone(PIEZO);
      lcd.setCursor(0, 0);
      lcd.print("T : "); lcd.print((int)t); lcd.print("C");
      lcd.setCursor(8, 0);
      lcd.print("H : "); lcd.print((int)h); lcd.print("%");
    }
  }
}

void RGB_Color(float c, int wait) {
  for (int i = 0; i < 3; i++) {
    RGB_LED.setPixelColor(i, c);
    RGB_LED.show();
  }
  delay(wait);
}
