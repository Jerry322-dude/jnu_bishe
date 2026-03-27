#include <math.h>

// 步进电机引脚定义
const int dirPinX = 5, stepPinX = 2;
const int dirPinY = 6, stepPinY = 3;
const int EN = 8;

// 三角形参数
const float radius = 5000.0;  // 外接圆半径（步数）
bool isStopped = false;

// 带直线插补的移动函数
void moveLine(int endX, int endY) {
  if (isStopped) return;
  
  // 设置方向
  digitalWrite(dirPinX, endX >= 0 ? HIGH : LOW);
  digitalWrite(dirPinY, endY >= 0 ? HIGH : LOW);
  
  // 计算步数和斜率
  int stepsX = abs(endX);
  int stepsY = abs(endY);
  int totalSteps = max(stepsX, stepsY);
  
  if (totalSteps == 0) return;
  
  // Bresenham直线插补算法
  float errorX = 0, errorY = 0;
  float deltaX = stepsX / (float)totalSteps;
  float deltaY = stepsY / (float)totalSteps;
  
  for (int i = 0; i < totalSteps && !isStopped; i++) {
    errorX += deltaX;
    errorY += deltaY;
    
    // X轴步进
    if (errorX >= 0.5) {
      digitalWrite(stepPinX, HIGH);
      delayMicroseconds(1);
      digitalWrite(stepPinX, LOW);
      errorX -= 1.0;
    }
    
    // Y轴步进
    if (errorY >= 0.5) {
      digitalWrite(stepPinY, HIGH);
      delayMicroseconds(1);
      digitalWrite(stepPinY, LOW);
      errorY -= 1.0;
    }
    
    delayMicroseconds(100);  // 控制速度
  }
}

// 计算等边三角形的顶点
void calculateTriangle(int &x1, int &y1, int &x2, int &y2, int &x3, int &y3) {
  // 第一个顶点在正上方（90°）
  x1 = 0;
  y1 = (int)radius;
  
  // 第二个顶点在210°（120°+90°） = 330°？需要仔细计算
  // 等边三角形：每个角60°，顶点间隔120°
  // 以(0, radius)为起点，旋转120°得到第二个顶点
  float angle1 = PI / 2;  // 90度
  float angle2 = angle1 + 2 * PI / 3;  // 90 + 120 = 210度
  float angle3 = angle2 + 2 * PI / 3;  // 210 + 120 = 330度
  
  x2 = (int)(radius * cos(angle2));
  y2 = (int)(radius * sin(angle2));
  
  x3 = (int)(radius * cos(angle3));
  y3 = (int)(radius * sin(angle3));
}

// 绘制等边三角形
void drawEquilateralTriangle() {
  Serial.println("开始绘制等边三角形...");
  
  // 计算顶点坐标
  int x1, y1, x2, y2, x3, y3;
  calculateTriangle(x1, y1, x2, y2, x3, y3);
  
  // 验证边长
  float d12 = sqrt(pow(x2-x1, 2) + pow(y2-y1, 2));
  float d23 = sqrt(pow(x3-x2, 2) + pow(y3-y2, 2));
  float d31 = sqrt(pow(x1-x3, 2) + pow(y1-y3, 2));
  
  // 开始绘制
  
  // 边1: 顶点1 -> 顶点2
  moveLine(x2 - x1, y2 - y1);
  
  if (isStopped) return;
  
  // 边2: 顶点2 -> 顶点3
  moveLine(x3 - x2, y3 - y2);
  
  if (isStopped) return;
  
  // 边3: 顶点3 -> 顶点1
  moveLine(x1 - x3, y1 - y3);
  
  Serial.println("三角形绘制完成!");
}

// 简化版本：使用预计算的精确值
void drawTriangleSimple() {
  Serial.println("绘制等边三角形（预计算版本）...");
  
  // 对于半径5000的等边三角形，顶点在：
  // 顶点1: (0, 5000)
  // 顶点2: (-4330, -2500)  // 注意：4330 = 5000 * cos(30°), 2500 = 5000 * sin(30°)
  // 顶点3: (4330, -2500)
  
  // 边1: (0,5000) -> (-4330,-2500)
  Serial.println("边1: (0,5000) -> (-4330,-2500)");
  moveLine(-4330, -7500);  // ΔX = -4330, ΔY = -7500
  
  if (isStopped) return;
  
  // 边2: (-4330,-2500) -> (4330,-2500)
  Serial.println("边2: (-4330,-2500) -> (4330,-2500)");
  moveLine(8660, 0);  // ΔX = 8660, ΔY = 0
  
  if (isStopped) return;
  
  // 边3: (4330,-2500) -> (0,5000)
  Serial.println("边3: (4330,-2500) -> (0,5000)");
  moveLine(-4330, 7500);  // ΔX = -4330, ΔY = 7500
  
  Serial.println("三角形绘制完成!");
}

void setup() {
  pinMode(dirPinX, OUTPUT);
  pinMode(stepPinX, OUTPUT);
  pinMode(dirPinY, OUTPUT);
  pinMode(stepPinY, OUTPUT);
  pinMode(EN, OUTPUT);
  
  digitalWrite(EN, LOW);  // 使能电机
  digitalWrite(dirPinX, LOW);
  digitalWrite(dirPinY, LOW);
  
  Serial.begin(9600);
  Serial.println("三角形绘图仪已就绪");
  Serial.println("发送 't' 绘制三角形");
  Serial.println("发送 's' 停止");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    if (cmd == 't' || cmd == 'T') {
      isStopped = false;
      // 使用预计算版本
      drawTriangleSimple();
    } else if (cmd == 's' || cmd == 'S') {
      isStopped = true;
      Serial.println("已停止");
    }
  }
}