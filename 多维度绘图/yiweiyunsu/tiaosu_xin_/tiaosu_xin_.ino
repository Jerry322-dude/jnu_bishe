#include <Arduino.h>

// 定义引脚
const int EN = 8;
const int xStepPin = 2;
const int yStepPin = 3;
const int xDirPin = 5;
const int yDirPin = 6;

// 定义脉冲频率和初始速度
const long pulseFrequency = 1600;  // 脉冲频率（Hz）
int xSpeed = 50;  // X 轴速度（0 - 255 之间）
int ySpeed = 50;  // Y 轴速度（0 - 255 之间）

unsigned long startTime;  // 记录开始时间
bool isRunning = true;  // 运行标志

void setup() {
  // 设置引脚模式
  pinMode(EN, OUTPUT);
  pinMode(xStepPin, OUTPUT);
  pinMode(yStepPin, OUTPUT);
  pinMode(xDirPin, OUTPUT);
  pinMode(yDirPin, OUTPUT);

  // 启用步进电机
  digitalWrite(EN, LOW);
  
  // 初始化 X 轴和 Y 轴方向
  digitalWrite(xDirPin, HIGH);
  digitalWrite(yDirPin, HIGH);

  startTime = millis();  // 记录开始时间
  Serial.begin(9600);    // 初始化串口通信
}

void loop() {
  // 生成 X 轴和 Y 轴脉冲
  if (isRunning) {
    for (int i = 0; i < xSpeed; i++) {  // 根据速度调整 X 轴脉冲数量
      digitalWrite(xStepPin, HIGH);
      delayMicroseconds(1000000 / pulseFrequency / 2);
      digitalWrite(xStepPin, LOW);
      delayMicroseconds(1000000 / pulseFrequency / 2);
    }

    for (int i = 0; i < ySpeed; i++) {  // 根据速度调整 Y 轴脉冲数量
      digitalWrite(yStepPin, HIGH);
      delayMicroseconds(1000000 / pulseFrequency / 2);
      digitalWrite(yStepPin, LOW);
      delayMicroseconds(1000000 / pulseFrequency / 2);
    }

    // 检查运行时间是否超过 3 秒
    if (millis() - startTime >= 3000) {
      isRunning = false;  // 停止运行
    }

    // 在此处添加调整 X 轴和 Y 轴速度的逻辑，例如通过串口接收新的速度值
    if (Serial.available() > 0) {
      String command = Serial.readStringUntil('\n');
      if (command.startsWith("x=")) {
        int newXSpeed = command.substring(2).toInt();
        if (newXSpeed >= 0 && newXSpeed <= 255) {
          xSpeed = newXSpeed;
        }
      } else if (command.startsWith("y=")) {
        int newYSpeed = command.substring(2).toInt();
        if (newYSpeed >= 0 && newYSpeed <= 255) {
          ySpeed = newYSpeed;
        }
      }
    }
  }
}
