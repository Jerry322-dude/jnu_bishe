#include <math.h>
#include <Arduino.h>

// 步进电机引脚定义
const int EN = 8; 
const int dirPinX = 5; 
const int stepPinX = 2; 
const int dirPinY = 6;
const int stepPinY = 3; 
const int dirPinZ = 7; 
const int stepPinZ = 4;

// 步进电机参数
const int stepsPerRevolution = 100;  // 步进电机每转步Write数
const int penLiftSteps = 1500;       // 抬落笔步数
const float stepsPerUnit = 4000.0;  // 每单位长度对应的步数

struct Point {
  float x;
  float y;
};

// 全局变量记录当前位置
float currentX = 0;
float currentY = 0;
bool penDown = false; // 笔状态（true=落下，false=抬起）

// 优化的步进电机控制函数
void stepMotor(int stepPin, int dirPin, int steps, int direction, int delayTime = 500) {
  digitalWrite(dirPin, direction);
  for (int i = 0; i < steps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(delayTime);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(delayTime);
  }
}

// Z轴抬笔/落笔控制
void setPenPosition(bool down) {
  if (down == penDown) return;
  
  if (down) {
    // 落笔
    stepMotor(stepPinZ, dirPinZ, penLiftSteps, LOW, 1000);
  } else {
    // 抬笔
    stepMotor(stepPinZ, dirPinZ, penLiftSteps, HIGH, 1000);
  }
  penDown = down;
}

// 使用Bresenham算法实现直线插补
void moveToLinear(float targetX, float targetY, int speed = 1000) {
  long x0 = round(currentX * stepsPerUnit);
  long y0 = round(currentY * stepsPerUnit);
  long x1 = round(targetX * stepsPerUnit);
  long y1 = round(targetY * stepsPerUnit);
  
  long dx = abs(x1 - x0);
  long dy = abs(y1 - y0);
  int dirX = x0 < x1 ? HIGH : LOW;
  int dirY = y0 < y1 ? HIGH : LOW;
  
  digitalWrite(dirPinX, dirX);
  digitalWrite(dirPinY, dirY);
  
  long sx = x0 < x1 ? 1 : -1;
  long sy = y0 < y1 ? 1 : -1;
  long err = dx - dy;
  
  while (true) {
    if (x0 == x1 && y0 == y1) break;
    
    long e2 = 2 * err;
    
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
      digitalWrite(stepPinX, HIGH);
      digitalWrite(stepPinX, LOW);
    }
    
    if (e2 < dx) {
      err += dx;
      y0 += sy;
      digitalWrite(stepPinY, HIGH);
      digitalWrite(stepPinY, LOW);
    }
    
    delayMicroseconds(speed);
  }
  currentX = targetX;
  currentY = targetY;
}

// 绘制圆（优化版）
void drawCircle(float centerX, float centerY, float radius, int segments = 100, int speed = 1000) {
  // 移动到圆周起始点（抬笔状态）
  float startX = centerX + radius;
  float startY = centerY;
  moveToLinear(startX, startY, 2000);
  
  setPenPosition(true); // 落笔
  
  // 绘制圆形
  for (int i = 1; i <= segments; i++) {
    float angle = 2 * M_PI * i / segments;
    float x = centerX + radius * cos(angle);
    float y = centerY + radius * sin(angle);
    moveToLinear(x, y, speed);
  }
  
  setPenPosition(false); // 抬笔
}

// 绘制点（短暂停留）
void drawPoint(float x, float y, int duration = 300) {
  moveToLinear(x, y, 2000);
  setPenPosition(true);
  delay(duration);
  setPenPosition(false);
}

// 修改后的函数：直接使用外接圆半径计算顶点坐标
void calculatePolygon(int n, float radius, Point* vertices) {
  float theta = 2 * M_PI / n;
  
  for (int i = 0; i < n; i++) {
    float angle = i * theta - theta / 2; // 调整角度使多边形的一个边平行于x轴
    vertices[i].x = radius * sin(angle);
    vertices[i].y = -radius * cos(angle);
  }
}

// 计算边长（可选，用于信息显示）
float calculateSideLength(int n, float radius) {
  return 2 * radius * sin(M_PI / n);
}

// 完整步骤的多边形绘制函数
void drawPolygon(int n, float radius, Point* vertices) {
  Serial.println("\nStarting complete polygon drawing sequence");

  // 1. 绘制外接圆
  Serial.println("1. Drawing circumcircle...");
  drawCircle(0, 0, radius);
  
  // 2. 抬笔
  Serial.println("2. Lifting pen...");
  setPenPosition(false);

  // 3. 绘制中心点
  Serial.println("3. Drawing center point...");
  drawPoint(0, 0);
  
  // 4. 抬笔
  Serial.println("4. Lifting pen...");
  setPenPosition(false);
  
  // 5. 移动到第一个顶点
  Serial.println("5. Moving to first vertex...");
  moveToLinear(vertices[0].x, vertices[0].y, 2000);
  
  // 6. 落笔
  Serial.println("6. Lowering pen...");
  setPenPosition(true);
  
  // 7. 绘制多边形边（确保闭合）
  Serial.println("7. Drawing polygon edges...");
  for (int i = 1; i <= n; i++) {
    int next = i % n;
    Serial.print("  Moving to vertex ");
    Serial.println(next);
    moveToLinear(vertices[next].x, vertices[next].y, 1000);
  }
  
  // 8. 抬笔
  Serial.println("8. Lifting pen...");
  setPenPosition(false);
  
  // 9. 返回中心
  Serial.println("9. Returning to center...");
  moveToLinear(0, 0, 2000);
  
  // 10. 落笔（可选）
  Serial.println("10. Lowering pen at center (optional)...");
  setPenPosition(true);
  setPenPosition(false);

  Serial.println("\nComplete polygon drawing finished!");
}

void setup() {
  // 初始化步进电机引脚
  pinMode(EN, OUTPUT);
  pinMode(dirPinX, OUTPUT);
  pinMode(stepPinX, OUTPUT);
  pinMode(dirPinY, OUTPUT);
  pinMode(stepPinY, OUTPUT);
  pinMode(dirPinZ, OUTPUT);
  pinMode(stepPinZ, OUTPUT);
  
  // 启用步进电机
  digitalWrite(EN, LOW);
  
  // 初始化串口
  Serial.begin(9600);
  while (!Serial);
  delay(1000);
  
  Serial.println("Complete Polygon Drawing with Stepper Motors");
  Serial.println("==========================================");
  
  // 初始位置设置
  setPenPosition(false);
  currentX = 0;
  currentY = 0;
  
  // 用户输入
  Serial.println("Enter number of sides (3-8): ");
  while (!Serial.available());
  int n = Serial.parseInt();
  
  Serial.println("Enter circumradius (1.0-5.0): ");
  while (!Serial.available());
  float radius = Serial.parseFloat();
  
  // 清除输入缓冲区
  while (Serial.available()) Serial.read();
  
  // 计算多边形顶点
  Point vertices[n];
  calculatePolygon(n, radius, vertices);
  float sideLength = calculateSideLength(n, radius);
  
  // 打印多边形信息
  Serial.print("\nPolygon with ");
  Serial.print(n);
  Serial.print(" sides, circumradius: ");
  Serial.println(radius);
  Serial.print("Side length: ");
  Serial.println(sideLength);
  
  Serial.println("Vertex coordinates:");
  for (int i = 0; i < n; i++) {
    Serial.print("  Vertex ");
    Serial.print(i);
    Serial.print(": (");
    Serial.print(vertices[i].x, 3);
    Serial.print(", ");
    Serial.print(vertices[i].y, 3);
    Serial.println(")");
  }
  
  // 绘制多边形
  drawPolygon(n, radius, vertices);
}

void loop() {
  // 空循环
}