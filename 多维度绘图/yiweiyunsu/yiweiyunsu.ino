#include <Arduino.h>

// 定义引脚
const int EN = 8;
const int yPulPin = 3;
const int yDirPin = 6;

// 定义脉冲频率和初始速度
const long pulseFrequency = 6400;  // 脉冲频率（Hz）
int ySpeed = 150;  // Y 轴速度（0 - 255 之间）

unsigned long startTime;  // 记录开始时间
bool isRunning = true;  // 运行标志

void setup() {
  // 设置引脚模式
  pinMode(EN, OUTPUT);
  pinMode(yPulPin, OUTPUT);
  pinMode(yDirPin, OUTPUT);

  // 使能引脚初始状态
  digitalWrite(EN, LOW); // 使能步进电机

  // 初始化 Y 轴方向
  digitalWrite(yDirPin, HIGH);

  startTime = millis();  // 记录开始时间
  Serial.begin(9600);    // 初始化串口通信
}

void loop() {

  // 生成 Y 轴脉冲
  if (isRunning) {
    for (int i = 0; i < ySpeed; i++) {  // 根据速度调整脉冲数量
      digitalWrite(yPulPin, HIGH);
      delayMicroseconds(1000000 / pulseFrequency / 2);
      digitalWrite(yPulPin, LOW);
      delayMicroseconds(1000000 / pulseFrequency / 2);
    }

    // 检查运行时间是否超过 3 秒
    if (millis() - startTime >= 3000) {
      isRunning = false;  // 停止运行
    }

    // 在此处添加调整 Y 轴速度的逻辑，例如通过串口接收新的速度值
    if (Serial.available() > 0) {
      int newSpeed = Serial.parseInt();
      if (newSpeed >= 0 && newSpeed <= 255) {
        ySpeed = newSpeed;
      }
    }
  }
}
