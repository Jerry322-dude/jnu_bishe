#include <Arduino.h>

// 引脚定义
const int EN = 8, dirX = 5, stepX = 2, dirY = 6, stepY = 3;

// 控制标志
bool stopped = false, drawing = false, redraw = false;

// 直线移动
void moveLine(int dx, int dy) {
  if (stopped) return;
  
  digitalWrite(dirX, dx >= 0);
  digitalWrite(dirY, dy >= 0);
  
  int steps = max(abs(dx), abs(dy));
  if (steps == 0) return;
  
  float ex = 0, ey = 0;
  float dxf = (float)abs(dx) / steps;
  float dyf = (float)abs(dy) / steps;
  
  for (int i = 0; i < steps && !stopped; i++) {
    ex += dxf;
    ey += dyf;
    
    if (ex >= 0.5) {
      digitalWrite(stepX, HIGH);
      delayMicroseconds(2);
      digitalWrite(stepX, LOW);
      ex -= 1.0;
    }
    
    if (ey >= 0.5) {
      digitalWrite(stepY, HIGH);
      delayMicroseconds(2);
      digitalWrite(stepY, LOW);
      ey -= 1.0;
    }
    
    delayMicroseconds(400);
  }
}

// 绘制六边形
void drawHex() {
  drawing = true;
  Serial.println("开始绘制");
  
  // 六条边
  moveLine(-3200, 5543);
  moveLine(-6400, 0);
  moveLine(-3200, -5543);
  moveLine(3200, -5543);
  moveLine(6400, 0);
  moveLine(3200, 5543);
  
  drawing = false;
  Serial.println("完成");
}

void setup() {
  pinMode(stepX, OUTPUT);
  pinMode(dirX, OUTPUT);
  pinMode(stepY, OUTPUT);
  pinMode(dirY, OUTPUT);
  pinMode(EN, OUTPUT);
  
  digitalWrite(EN, HIGH);
  
  Serial.begin(9600);
  Serial.println("就绪: d=绘制, s=暂停, r=重绘");
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    
    if (c == 'd' && !drawing) {
      stopped = false;
      redraw = true;
      digitalWrite(EN, LOW);
    } 
    else if (c == 's') {
      stopped = true;
      digitalWrite(EN, HIGH);
    } 
    else if (c == 'r') {
      stopped = false;
      redraw = true;
      digitalWrite(EN, LOW);
    }
  }
  
  if (redraw && !drawing) {
    redraw = false;
    drawHex();
  }
}