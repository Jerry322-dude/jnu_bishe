#include <Arduino.h>

// 定义引脚
const int xStepPin = 2;
const int xDirPin = 5;
const int yStepPin = 3;
const int yDirPin = 6;
const int EN = 8;

// 参数配置
const int stepsPerSide = 6400;  // 每条边的步数
const int pulseInterval = 200;  // 脉冲间隔（微秒）

// 停止标志与绘制状态
volatile bool isStopped = false;
bool squareDrawn = false;       // 新增绘制完成标志

// 生成脉冲（优化时序）
void step(int stepPin) {
  if (!isStopped) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(max(1, pulseInterval / 2));  // 确保最小高电平时间
    digitalWrite(stepPin, LOW);
    delayMicroseconds(max(1, pulseInterval / 2));
  }
}

// 移动函数（修正方向逻辑）
void move(int stepPin, int dirPin, int steps, bool forward) {
  if (!isStopped) {
    digitalWrite(dirPin, forward ? HIGH : LOW);  // 修正方向极性
    for (int i = 0; i < steps; i++) {
      step(stepPin);
    }
  }
}

// 绘制正方形（添加完成标志）
void drawSquare() {
  if (!isStopped && !squareDrawn) {
    Serial.println("Drawing square...");
    
    // X+方向
    move(xStepPin, xDirPin, stepsPerSide, true);
    // Y+方向
    move(yStepPin, yDirPin, stepsPerSide, true);
    // X-方向
    move(xStepPin, xDirPin, stepsPerSide, false);
    // Y-方向
    move(yStepPin, yDirPin, stepsPerSide, false);
    
    squareDrawn = true;  // 标记已完成
    Serial.println("Square completed.");
  }
}

void setup() {
  // 初始化引脚
  pinMode(xStepPin, OUTPUT);
  pinMode(xDirPin, OUTPUT);
  pinMode(yStepPin, OUTPUT);
  pinMode(yDirPin, OUTPUT);
  pinMode(EN, OUTPUT);
  
  // 初始状态
  digitalWrite(EN, LOW);  
  digitalWrite(xDirPin, LOW);
  digitalWrite(yDirPin, LOW);

  // 初始化串口
  Serial.begin(9600);
  Serial.println("System ready. Send 'r' to start.");
}

void loop() {
  // 处理串口命令
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 's') {
      isStopped = true;
      digitalWrite(EN, HIGH);  // 紧急停止时禁用电机
      Serial.println("Emergency stop!");
    } else if (cmd == 'r') {
      isStopped = false;
      squareDrawn = false;      // 重置标志以重新绘制
      digitalWrite(EN, LOW);    // 使能电机
      Serial.println("Resetting...");
    }
  }

  // 仅当未完成时绘制一次
  if (!squareDrawn && !isStopped) {
    drawSquare();
  }
}