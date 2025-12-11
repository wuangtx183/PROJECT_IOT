#include <Arduino.h>
#include <ESP32Servo.h>
#include <TCS3200.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// ===== INCLUDE CÁC FILE CẤU HÌNH CỦA BẠN =====
#include "secrets/wifi.h"       // Chứa tên/pass wifi
#include "secrets/mqtt.h"       // Chứa thông tin broker EMQX
#include "ca_cert_emqx.h"       // Chứa chứng chỉ SSL

// ===== CẤU HÌNH MQTT SECURE =====
WiFiClientSecure espClient;
PubSubClient client(espClient);

// ===== 3 TOPIC RIÊNG BIỆT =====
const char* topic_red = "hethong/do";
const char* topic_blue = "hethong/xanh";
const char* topic_yellow = "hethong/vang";

// ===== BIẾN ĐẾM =====
int countRed = 0;
int countBlue = 0;
int countYellow = 0;

// ===== CẤU HÌNH CHÂN (GIỮ NGUYÊN) =====
Servo servo1; 
Servo servo2;
#define SERVO1_PIN 26
#define SERVO2_PIN 25
#define SERVO_IDLE 90
#define SERVO_PUSH 0

#define IR1_PIN 32
#define IR2_PIN 27
#define IR3_PIN 33

#define S0 18
#define S1 19
#define S2 21
#define S3 22
#define OUT 23

// Logic & Thời gian
const unsigned long DELAY_BEFORE_PUSH = 1000;
const unsigned long SERVO_TIME = 400;
bool ir1Triggered = false;
bool ir2Triggered = false;
unsigned long ir1Time = 0;
unsigned long ir2Time = 0;
bool ir3LastState = HIGH;

enum Color { NONE, RED_C, BLUE_C, YELLOW_C };
Color lastColor = NONE;
unsigned long lastColorRead = 0;
const unsigned long COLOR_INTERVAL = 120;
int red = 0, green = 0, blue = 0;

// ================= HÀM KẾT NỐI =================
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Dang ket noi WiFi: ");
  Serial.println(WiFiSecrets::ssid);

  WiFi.begin(WiFiSecrets::ssid, WiFiSecrets::pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi da ket noi");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Cấu hình chứng chỉ SSL để nối EMQX
  espClient.setCACert(ca_cert);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Dang ket noi MQTT (SSL)...");
    
    // Tạo Client ID ngẫu nhiên
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    // Kết nối sử dụng thông tin từ namespace EMQX
    if (client.connect(clientId.c_str(), EMQX::username, EMQX::password)) {
      Serial.println("Da ket noi!");
    } else {
      Serial.print("That bai, rc=");
      Serial.print(client.state());
      Serial.println(" thu lai sau 5s");
      delay(5000);
    }
  }
}

// ================= HÀM XỬ LÝ MÀU =================
void readColor() {
  int timeout = 15000;
  digitalWrite(S2, LOW); digitalWrite(S3, LOW);
  red = pulseIn(OUT, LOW, timeout);
  digitalWrite(S2, LOW); digitalWrite(S3, HIGH);
  blue = pulseIn(OUT, LOW, timeout);
  digitalWrite(S2, HIGH); digitalWrite(S3, HIGH);
  green = pulseIn(OUT, LOW, timeout);
  
  // Fix lỗi đọc 0
  if (red == 0) red = 9999;
  if (green == 0) green = 9999;
  if (blue == 0) blue = 9999;
}

Color detectColor() {
  if (red > 10 && red < 47 && green > 50 && blue > 30) return RED_C;
  if (blue < 100 && red > 110 && green > 90) return BLUE_C;
  if (red <= 12 && green < 55 && blue < 40) return YELLOW_C;
  return NONE;
}

// ================= SETUP & LOOP =================
void setup() {
  Serial.begin(115200);
  
  setup_wifi();
  
  // Cấu hình MQTT Server từ namespace EMQX
  client.setServer(EMQX::broker, EMQX::port);

  // Cấu hình cảm biến & Servo
  pinMode(IR1_PIN, INPUT); pinMode(IR2_PIN, INPUT); pinMode(IR3_PIN, INPUT);
  
  servo1.setPeriodHertz(50); servo2.setPeriodHertz(50);
  servo1.attach(SERVO1_PIN, 500, 2400); servo2.attach(SERVO2_PIN, 500, 2400);
  servo1.write(SERVO_IDLE); servo2.write(SERVO_IDLE);

  pinMode(S0, OUTPUT); pinMode(S1, OUTPUT); pinMode(S2, OUTPUT); pinMode(S3, OUTPUT); pinMode(OUT, INPUT);
  digitalWrite(S0, HIGH); digitalWrite(S1, HIGH);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();

  // Đọc màu
  if (now - lastColorRead >= COLOR_INTERVAL) {
    lastColorRead = now;
    readColor();
    Color c = detectColor();
    if (c != NONE) lastColor = c;
  }

  // 1. MÀU ĐỎ -> Gửi topic "hethong/do"
  if (digitalRead(IR1_PIN) == LOW && !ir1Triggered && lastColor == RED_C) {
    ir1Triggered = true; ir1Time = now;
  }
  if (ir1Triggered && now - ir1Time >= DELAY_BEFORE_PUSH) {
    servo1.write(SERVO_PUSH); delay(SERVO_TIME); servo1.write(SERVO_IDLE);
    
    countRed++;
    Serial.printf("Do: %d\n", countRed);
    client.publish(topic_red, String(countRed).c_str()); 
    
    ir1Triggered = false; lastColor = NONE;
  }

  // 2. MÀU VÀNG -> Gửi topic "hethong/vang"
  if (digitalRead(IR2_PIN) == LOW && !ir2Triggered && lastColor == YELLOW_C) {
    ir2Triggered = true; ir2Time = now;
  }
  if (ir2Triggered && now - ir2Time >= 4000) {
    servo2.write(SERVO_PUSH); delay(SERVO_TIME); servo2.write(SERVO_IDLE);
    
    countYellow++;
    Serial.printf("Vang: %d\n", countYellow);
    client.publish(topic_yellow, String(countYellow).c_str());
    
    ir2Triggered = false; lastColor = NONE;
  }

  // 3. MÀU XANH -> Gửi topic "hethong/xanh"
  bool s3 = digitalRead(IR3_PIN);
  if (ir3LastState == HIGH && s3 == LOW && lastColor == BLUE_C) {
    
    countBlue++;
    Serial.printf("Xanh: %d\n", countBlue);
    client.publish(topic_blue, String(countBlue).c_str());
    
    lastColor = NONE;
  }
  ir3LastState = s3;
}