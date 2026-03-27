#include <math.h>

const int EN = 8, stepX = 2, dirX = 5, stepY = 3, dirY = 6;
const float radius = 178.57;
const int numSteps = 360;

// 控制标志
bool stopped = false, completed = false;

void setup() {
  pinMode(EN, OUTPUT);
  pinMode(stepX, OUTPUT);
  pinMode(dirX, OUTPUT);
  pinMode(stepY, OUTPUT);
  pinMode(dirY, OUTPUT);
  
  digitalWrite(EN, LOW);
  
  Serial.begin(9600);
  Serial.println("就绪: d=绘制一个圆");
}

void moveX(int steps) {
  if (stopped || completed) return;
  
  digitalWrite(dirX, steps > 0);
  for (int i = 0; i < abs(steps) && !stopped && !completed; i++) {
    digitalWrite(stepX, HIGH);
    delayMicroseconds(200);
    digitalWrite(stepX, LOW);
    delayMicroseconds(200);
  }
}

void moveY(int steps) {
  if (stopped || completed) return;
  
  digitalWrite(dirY, steps > 0);
  for (int i = 0; i < abs(steps) && !stopped && !completed; i++) {
    digitalWrite(stepY, HIGH);
    delayMicroseconds(200);
    digitalWrite(stepY, LOW);
    delayMicroseconds(200);
  }
}

// 绘制一个完整的圆形
void drawCircleOnce() {
  if (completed) return;
  
  Serial.println("绘制圆形...");
  
  for (int i = 0; i <= numSteps; i++) {
    if (stopped) return;
    
    float angle = 2.0 * PI * i / numSteps;
    moveX(round(radius * cos(angle)));
    moveY(round(radius * sin(angle)));
  }
  
  // 绘制完成
  completed = true;
  digitalWrite(EN, HIGH);  // 禁用电机
  Serial.println("圆形绘制完成，系统停止");
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    
    if (c == 'd' && !completed) {
      stopped = false;
      digitalWrite(EN, LOW);
      drawCircleOnce();
    } 
    else if (c == 's' && !completed) {
      stopped = true;
      digitalWrite(EN, HIGH);
    }
  }
}