#include <Arduino.h>

// 定义引脚
const int EN = 8;
const int stepPinY = 3;
const int dirPinY = 6;

// 初始状态
bool yDirState = HIGH;

// 步进脉冲间隔时间（微秒），数值越小速度越快
unsigned long stepInterval = 150; 

// 移动时间（毫秒）
int moveDuration = 3000; 

// 停止标志
bool stopFlag = false;

// 上次步进时间
unsigned long lastStepTime = 0;

void setup() {
  // 设置引脚为输出模式
  pinMode(EN, OUTPUT);
  pinMode(stepPinY, OUTPUT);
  pinMode(dirPinY, OUTPUT);

  // 使能引脚初始状态
  digitalWrite(EN, LOW); // 使能步进电机

  // 初始化串口通信
  Serial.begin(9600);
  Serial.println("++++++++++++++++++++++++++++++");
  Serial.println("+ CNC X-Y Stepper Motor Control  +");
  Serial.println("+    Customized by You       +");
  Serial.println("++++++++++++++++++++++++++++++");
}

void loop() {
  if (!stopFlag) {
    unsigned long currentTime = micros();

    // 控制步进脉冲
    if (currentTime - lastStepTime >= stepInterval) {
      digitalWrite(stepPinY, HIGH);
      delayMicroseconds(10); // 脉冲宽度
      digitalWrite(stepPinY, LOW);
      lastStepTime = currentTime;
    }

    static unsigned long startTime = millis();
    if (millis() - startTime >= moveDuration) {
      // 切换方向
      yDirState =!yDirState;
      startTime = millis();
    }

    // 设置方向
    digitalWrite(dirPinY, yDirState);
  }

  if (Serial.available()) {  // 检查串口缓存是否有数据等待传输
    char cmd = Serial.read();  // 获取指令

    if (cmd == 'p') { // 停止命令
      stopFlag = true;
      Serial.println("Motion stopped.");
      // 停止步进电机脉冲输出
      digitalWrite(stepPinY, LOW);
      // 进入无限循环，停止程序运行
      while (1); 
    }
  }
}

