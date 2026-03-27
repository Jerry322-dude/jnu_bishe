// 步进电机引脚定义
const byte EN = 8; 
const byte dirPinY = 5; 
const byte stepPinY = 2; 
const byte dirPinX = 6;
const byte stepPinX = 3; 
const byte dirPinZ = 7; 
const byte stepPinZ = 4;

// 步进电机参数
const int penLiftSteps = 5000;
const int stepsPerMM = 100;
const int MOVE_SPEED = 300;    // 高速模式
const int DRAW_SPEED = 500;   // 高速模式

// 绘图参数
const float CIRCLE_RADIUS = 8.0;    // 圆半径8mm
const float CIRCLE_DIAMETER = 16.0; // 圆直径16mm

// 纸张参数
const int PAPER_WIDTH_MM = 297;
const int PAPER_HEIGHT_MM = 210;
const int IMAGE_WIDTH_PX = 640;
const int IMAGE_HEIGHT_PX = 480;

// 放大比例
const float SCALE_FACTOR = 1.3;  // 坐标放大1.3倍

// 坐标偏移参数 - 所有机械坐标x减少50mm，y减少15mm
const float X_OFFSET = -50.0;  // X轴偏移
const float Y_OFFSET = -15.0;  // Y轴偏移

// 方向控制变量
bool xReversed = true;  // X轴翻转
bool yReversed = true;  // Y轴翻转

// 镜像控制 - 沿Y轴镜像
const bool MIRROR_Y_AXIS = true;  // 启用Y轴镜像

// 数字坐标结构
struct DigitPos {
  int x = -1;
  int y = -1;
  bool set = false;
};

// 全局变量
DigitPos digits[5];
byte receivedCount = 0;
bool readyToDraw = false;
float currentX = 0.0, currentY = 0.0;
bool penIsDown = false;

void setup() {
  // 初始化引脚
  pinMode(EN, OUTPUT);
  pinMode(dirPinX, OUTPUT);
  pinMode(stepPinX, OUTPUT);
  pinMode(dirPinY, OUTPUT);
  pinMode(stepPinY, OUTPUT);
  pinMode(dirPinZ, OUTPUT);
  pinMode(stepPinZ, OUTPUT);
  
  digitalWrite(EN, LOW);
  
  // 初始化串口
  Serial.begin(9600);
  while (!Serial); // 等待串口连接
  
  // 清空缓冲区
  while (Serial.available()) Serial.read();
  
  // 打印系统信息
  Serial.println(F("========================================"));
  Serial.println(F("     数字坐标绘图系统     "));
  Serial.println(F("========================================"));
  Serial.println(F("系统设置："));
  Serial.println(F("  1. 坐标放大1.3倍"));
  Serial.println(F("  2. 所有机械坐标x减少50mm，y减少15mm"));
  Serial.print(F("放大比例："));
  Serial.print(SCALE_FACTOR);
  Serial.println(F("倍"));
  Serial.print(F("坐标偏移：X:"));
  Serial.print(X_OFFSET);
  Serial.print(F(" mm, Y:"));
  Serial.print(Y_OFFSET);
  Serial.println(F(" mm"));
  
  // 初始化当前位置
  currentX = 0;
  currentY = 0;
  
  // 打印欢迎信息
  Serial.println(F("第一步：输入5个数字的坐标"));
  Serial.println(F("格式示例："));
  Serial.println(F("  Digit 1: Center at (128, 138)"));
  Serial.println(F("  ..."));
  Serial.println(F("  每行输入一个，按回车发送"));
  Serial.println(F("========================================"));
  Serial.println();
  
  // 测试移动到原点
  goHome();
}

void loop() {
  // 检查串口输入
  if (Serial.available()) {
    // 读取一行
    String line = Serial.readStringUntil('\n');
    line.trim();
    
    // 显示收到的内容（调试）
    Serial.print(F("> "));
    Serial.println(line);
    
    // 处理输入
    processInput(line);
  }
  
  delay(10);
}

void processInput(String line) {
  if (line.length() == 0) return;
  
  // 检查是否是坐标输入
  if (line.indexOf("Digit") != -1) {
    if (!readyToDraw) {
      handleCoordinateInput(line);
    } else {
      Serial.println(F("坐标已加载，输入1-5开始绘制"));
    }
    return;
  }
  
  // 检查是否是单字符命令
  if (line.length() == 1) {
    char cmd = line[0];
    
    if (cmd >= '1' && cmd <= '5') {
      drawDigit(cmd - '1');
    }
    else if (cmd == 'a' || cmd == 'A') {
      drawAll();
    }
    else if (cmd == 'e' || cmd == 'E') {
      drawEvenOdd();
    }
    else if (cmd == 'h' || cmd == 'H') {
      goHome();
      Serial.println(F("已返回原点"));
    }
    else if (cmd == 'l' || cmd == 'L') {
      listCoordinates();
    }
    else if (cmd == 'r' || cmd == 'R') {
      resetCoordinates();
      Serial.println(F("坐标已重置，请重新输入"));
    }
    else if (cmd == 'x' || cmd == 'X') {
      xReversed = !xReversed;
      Serial.print(F("X轴方向已"));
      Serial.println(xReversed ? F("翻转") : F("正常"));
    }
    else if (cmd == 'y' || cmd == 'Y') {
      yReversed = !yReversed;
      Serial.print(F("Y轴方向已"));
      Serial.println(yReversed ? F("翻转") : F("正常"));
    }
    else if (cmd == 'd' || cmd == 'D') {
      showDirection();
    }
    else if (cmd == 't' || cmd == 'T') {
      testDirection();
    }
    else if (cmd == '?') {
      showHelp();
    }
    else if (cmd == 's' || cmd == 'S') {
      showStatus();
    }
    else {
      Serial.print(F("未知命令："));
      Serial.println(cmd);
      Serial.println(F("输入 ? 查看帮助"));
    }
  }
}

// 坐标转换函数：应用所有变换
float convertToMachineX(int pixelX) {
  float machineX = (float)pixelX * PAPER_WIDTH_MM / IMAGE_WIDTH_PX * SCALE_FACTOR;
  machineX += X_OFFSET; // 应用X偏移
  // 边界检查
  if (machineX < 0) machineX = 0;
  if (machineX > PAPER_WIDTH_MM) machineX = PAPER_WIDTH_MM;
  return machineX;
}

float convertToMachineY(int pixelY) {
  float machineY = PAPER_HEIGHT_MM - (float)pixelY * PAPER_HEIGHT_MM / IMAGE_HEIGHT_PX * SCALE_FACTOR;
  // 应用Y轴镜像
  if (MIRROR_Y_AXIS) {
    machineY = PAPER_HEIGHT_MM - machineY;
  }
  machineY += Y_OFFSET; // 应用Y偏移
  // 边界检查
  if (machineY < 0) machineY = 0;
  if (machineY > PAPER_HEIGHT_MM) machineY = PAPER_HEIGHT_MM;
  return machineY;
}

void handleCoordinateInput(String line) {
  // 简化的坐标解析
  int digitStart = line.indexOf("Digit ");
  if (digitStart == -1) return;
  
  // 查找数字
  int spaceAfterDigit = line.indexOf(' ', digitStart + 6);
  if (spaceAfterDigit == -1) return;
  
  String digitStr = line.substring(digitStart + 6, spaceAfterDigit);
  digitStr.trim();
  int digit = digitStr.toInt();
  
  if (digit < 1 || digit > 5) {
    Serial.println(F("错误：数字必须在1-5之间"));
    return;
  }
  
  // 查找坐标
  int parenStart = line.indexOf('(');
  int commaPos = line.indexOf(',', parenStart);
  int parenEnd = line.indexOf(')', commaPos);
  
  if (parenStart == -1 || commaPos == -1 || parenEnd == -1) {
    Serial.println(F("错误：坐标格式不正确"));
    return;
  }
  
  // 提取坐标
  String xStr = line.substring(parenStart + 1, commaPos);
  String yStr = line.substring(commaPos + 1, parenEnd);
  xStr.trim();
  yStr.trim();
  
  int x = xStr.toInt();
  int y = yStr.toInt();
  
  // 保存坐标
  digits[digit - 1].x = x;
  digits[digit - 1].y = y;
  digits[digit - 1].set = true;
  receivedCount++;
  
  Serial.print(F("✓ 数字 "));
  Serial.print(digit);
  Serial.print(F("：("));
  Serial.print(x);
  Serial.print(F(", "));
  Serial.print(y);
  Serial.println(F(")"));
  
  // 应用所有转换：放大1.3倍 + 坐标偏移
  float targetX = convertToMachineX(x);
  float targetY = convertToMachineY(y);
  
  Serial.print(F("   机械坐标：("));
  Serial.print(targetX, 1);
  Serial.print(F(", "));
  Serial.print(targetY, 1);
  Serial.println(F(") mm"));
  
  // 显示转换详情
  Serial.println(F("   转换详情："));
  Serial.print(F("     原始转换："));
  Serial.print((float)x * PAPER_WIDTH_MM / IMAGE_WIDTH_PX, 1);
  Serial.print(F(", "));
  Serial.print((float)y * PAPER_HEIGHT_MM / IMAGE_HEIGHT_PX, 1);
  Serial.println(F(") mm"));
  Serial.print(F("     1.3倍放大后："));
  Serial.print((float)x * PAPER_WIDTH_MM / IMAGE_WIDTH_PX * SCALE_FACTOR, 1);
  Serial.print(F(", "));
  Serial.print((float)y * PAPER_HEIGHT_MM / IMAGE_HEIGHT_PX * SCALE_FACTOR, 1);
  Serial.println(F(") mm"));
  Serial.print(F("     坐标偏移后："));
  Serial.print(targetX, 1);
  Serial.print(F(", "));
  Serial.print(targetY, 1);
  Serial.println(F(") mm"));
  
  // 检查是否完成
  if (receivedCount == 5) {
    readyToDraw = true;
    Serial.println();
    Serial.println(F("========================================"));
    Serial.println(F("✓ 所有坐标已接收完成！"));
    Serial.println(F("========================================"));
    Serial.println();
    Serial.println(F("现在可以开始绘制："));
    Serial.println(F("  1-5 : 绘制单个数字"));
    Serial.println(F("  a   : 绘制所有数字"));
    Serial.println(F("  e   : 偶数○，奇数圆内接△"));
    Serial.println(F("  h   : 返回原点"));
    Serial.println(F("  l   : 查看坐标列表"));
    Serial.println(F("  r   : 重置坐标"));
    Serial.println(F("  x   : 切换X轴方向"));
    Serial.println(F("  y   : 切换Y轴方向"));
    Serial.println(F("  d   : 显示当前方向"));
    Serial.println(F("  t   : 测试方向"));
    Serial.println(F("  ?   : 显示帮助"));
    Serial.println(F("========================================"));
  } else {
    Serial.print(F("进度："));
    Serial.print(receivedCount);
    Serial.println(F("/5"));
  }
}

void drawDigit(byte index) {
  if (!readyToDraw) {
    Serial.println(F("错误：请先输入5个坐标"));
    return;
  }
  
  if (!digits[index].set) {
    Serial.print(F("错误：数字 "));
    Serial.print(index + 1);
    Serial.println(F(" 的坐标未设置"));
    return;
  }
  
  Serial.println();
  Serial.println(F("========================================"));
  Serial.print(F("开始绘制数字 "));
  Serial.println(index + 1);
  Serial.println(F("========================================"));
  
  // 应用所有转换：放大1.3倍 + 坐标偏移
  float targetX = convertToMachineX(digits[index].x);
  float targetY = convertToMachineY(digits[index].y);
  
  Serial.print(F("像素坐标：("));
  Serial.print(digits[index].x);
  Serial.print(F(", "));
  Serial.print(digits[index].y);
  Serial.println(F(")"));
  
  Serial.print(F("机械坐标（应用所有变换）：("));
  Serial.print(targetX, 1);
  Serial.print(F(" mm, "));
  Serial.print(targetY, 1);
  Serial.println(F(" mm)"));
  
  // 移动到目标位置
  liftPen();
  Serial.println(F("移动到目标位置..."));
  moveTo(targetX, targetY, MOVE_SPEED);
  
  // 绘制圆圈
  Serial.print(F("绘制圆圈（半径"));
  Serial.print(CIRCLE_DIAMETER);
  Serial.println(F("mm）..."));
  drawCircle(targetX, targetY, CIRCLE_DIAMETER);
  
  // 返回原点
  Serial.println(F("返回原点..."));
  goHome();
  
  Serial.println(F("✓ 绘制完成！"));
  Serial.println(F("========================================"));
  Serial.println();
}

// 绘制所有数字
void drawAll() {
  if (!readyToDraw) {
    Serial.println(F("错误：请先输入5个坐标"));
    return;
  }
  
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("开始绘制所有数字"));
  Serial.println(F("========================================"));
  
  for (byte i = 0; i < 5; i++) {
    if (digits[i].set) {
      Serial.print(F("绘制数字 "));
      Serial.println(i + 1);
      
      // 应用所有转换：放大1.3倍 + 坐标偏移
      float targetX = convertToMachineX(digits[i].x);
      float targetY = convertToMachineY(digits[i].y);
      
      // 移动到目标位置
      liftPen();
      moveTo(targetX, targetY, MOVE_SPEED);
      
      // 绘制圆圈
      drawCircle(targetX, targetY, CIRCLE_DIAMETER);
    }
  }
  
  // 返回原点
  Serial.println(F("返回原点..."));
  goHome();
  
  Serial.println(F("✓ 所有数字绘制完成！"));
  Serial.println(F("========================================"));
  Serial.println();
}

// 绘制奇偶数不同形状
void drawEvenOdd() {
  if (!readyToDraw) {
    Serial.println(F("错误：请先输入5个坐标"));
    return;
  }
  
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("开始绘制：偶数○，奇数△"));
  Serial.println(F("========================================"));
  
  for (byte i = 0; i < 5; i++) {
    if (digits[i].set) {
      byte digitNum = i + 1;
      Serial.print(F("绘制数字 "));
      Serial.print(digitNum);
      
      // 应用所有转换：放大1.3倍 + 坐标偏移
      float targetX = convertToMachineX(digits[i].x);
      float targetY = convertToMachineY(digits[i].y);
      
      // 移动到目标位置
      liftPen();
      moveTo(targetX, targetY, MOVE_SPEED);
      
      // 判断奇偶并绘制不同形状
      if (digitNum % 2 == 0) { // 偶数
        Serial.println(F(" (偶数 ○)"));
        drawCircle(targetX, targetY, CIRCLE_DIAMETER);
      } else { // 奇数
        Serial.println(F(" (奇数 △)"));
        // 绘制圆内接等边三角形
        drawTriangleInCircle(targetX, targetY, CIRCLE_DIAMETER);
      }
    }
  }
  
  // 返回原点
  Serial.println(F("返回原点..."));
  goHome();
  
  Serial.println(F("✓ 奇偶数区分绘制完成！"));
  Serial.println(F("========================================"));
  Serial.println();
}

// 步进电机基础函数
void stepMotor(byte stepPin, byte dirPin, int steps, bool direction, int delayTime = 600) {
  digitalWrite(dirPin, direction ? HIGH : LOW);
  for (int i = 0; i < steps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(delayTime);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(delayTime);
  }
}

void liftPen() {
  if (!penIsDown) return;
  stepMotor(stepPinZ, dirPinZ, penLiftSteps / 4, true, 800);
  penIsDown = false;
}

void lowerPen() {
  if (penIsDown) return;
  stepMotor(stepPinZ, dirPinZ, penLiftSteps / 4, false, 800);
  penIsDown = true;
}

void moveTo(float targetX, float targetY, int speed) {
  long curStepsX = (long)(currentX * stepsPerMM);
  long curStepsY = (long)(currentY * stepsPerMM);
  long targetStepsX = (long)(targetX * stepsPerMM);
  long targetStepsY = (long)(targetY * stepsPerMM);
  
  long dx = abs(targetStepsX - curStepsX);
  long dy = abs(targetStepsY - curStepsY);
  
  // 计算基本方向
  bool baseDirX = targetStepsX > curStepsX;
  bool baseDirY = targetStepsY > curStepsY;
  
  // 应用方向翻转
  bool dirX = xReversed ? !baseDirX : baseDirX;
  bool dirY = yReversed ? !baseDirY : baseDirY;
  
  digitalWrite(dirPinX, dirX ? HIGH : LOW);
  digitalWrite(dirPinY, dirY ? HIGH : LOW);
  
  long err = dx - dy;
  
  // 安全计数器，防止无限循环
  unsigned long maxSteps = max(dx, dy) * 2;
  unsigned long stepCount = 0;
  
  while ((curStepsX != targetStepsX || curStepsY != targetStepsY) && stepCount < maxSteps) {
    long e2 = err * 2;
    
    if (e2 > -dy) {
      err -= dy;
      curStepsX += dirX ? 1 : -1;
      digitalWrite(stepPinX, HIGH);
      delayMicroseconds(speed / 2);
      digitalWrite(stepPinX, LOW);
      delayMicroseconds(speed / 2);
    }
    
    if (e2 < dx) {
      err += dx;
      curStepsY += dirY ? 1 : -1;
      digitalWrite(stepPinY, HIGH);
      delayMicroseconds(speed / 2);
      digitalWrite(stepPinY, LOW);
      delayMicroseconds(speed / 2);
    }
    
    stepCount++;
  }
  
  // 更新当前位置
  currentX = targetX;
  currentY = targetY;
}

void drawCircle(float centerX, float centerY, float radius) {
  const byte segments = 36; // 增加分段数使圆形更平滑
  
  // 移动到起始点
  moveTo(centerX + radius, centerY, MOVE_SPEED);
  delay(50);
  
  // 落笔
  lowerPen();
  
  // 绘制圆形
  for (byte i = 1; i <= segments; i++) {
    float angle = 6.28318 * i / segments; // 2 * PI
    float x = centerX + radius * cos(angle);
    float y = centerY + radius * sin(angle);
    moveTo(x, y, DRAW_SPEED);
  }
  
  // 抬笔
  liftPen();
}

// 绘制圆内接等边三角形
void drawTriangleInCircle(float centerX, float centerY, float radius) {
  // 圆内接等边三角形的边长 = 半径 * √3
  float side = radius * 1.732; // √3 ≈ 1.732
  
  // 计算三个顶点（三角形顶部在正上方）
  // 顶点1：正上方
  float x1 = centerX;
  float y1 = centerY + radius * 0.5; // 内接三角形顶点到圆心的距离为半径的一半
  
  // 顶点2：左下角
  float x2 = centerX - side / 2;
  float y2 = centerY - radius * 0.866; // 底部到圆心的距离
  
  // 顶点3：右下角
  float x3 = centerX + side / 2;
  float y3 = centerY - radius * 0.866;
  
  // 移动到起始点
  moveTo(x1, y1, MOVE_SPEED);
  delay(50);
  
  // 落笔
  lowerPen();
  
  // 绘制三角形
  moveTo(x2, y2, DRAW_SPEED);
  moveTo(x3, y3, DRAW_SPEED);
  moveTo(x1, y1, DRAW_SPEED); // 回到起点闭合三角形
  
  // 抬笔
  liftPen();
  
  // 显示三角形信息
  Serial.print(F("   圆半径："));
  Serial.print(radius);
  Serial.print(F("mm，三角形边长："));
  Serial.print(side, 1);
  Serial.println(F("mm"));
}

void goHome() {
  liftPen();
  moveTo(0, 0, MOVE_SPEED);  
  currentX = 0;
  currentY = 0;
}

void listCoordinates() {
  Serial.println();
  Serial.println(F("坐标列表（应用所有变换）："));
  Serial.println(F("------------------------"));
  
  for (byte i = 0; i < 5; i++) {
    if (digits[i].set) {
      Serial.print(F("数字 "));
      Serial.print(i + 1);
      Serial.print(F(": ("));
      Serial.print(digits[i].x);
      Serial.print(F(", "));
      Serial.print(digits[i].y);
      Serial.print(F(")"));
      
      // 应用所有转换
      float mx = convertToMachineX(digits[i].x);
      float my = convertToMachineY(digits[i].y);
      
      Serial.print(F(" -> ("));
      Serial.print(mx, 1);
      Serial.print(F(", "));
      Serial.print(my, 1);
      Serial.println(F(") mm"));
    } else {
      Serial.print(F("数字 "));
      Serial.print(i + 1);
      Serial.println(F(": [未设置]"));
    }
  }
  
  Serial.print(F("进度："));
  Serial.print(receivedCount);
  Serial.print(F("/5，"));
  Serial.println(readyToDraw ? F("就绪") : F("等待中"));
  Serial.print(F("放大比例："));
  Serial.print(SCALE_FACTOR);
  Serial.println(F("倍"));
  Serial.print(F("坐标偏移：X:"));
  Serial.print(X_OFFSET);
  Serial.print(F("mm, Y:"));
  Serial.print(Y_OFFSET);
  Serial.println(F("mm"));
  Serial.print(F("圆直径："));
  Serial.print(CIRCLE_DIAMETER);
  Serial.println(F("mm"));
  Serial.print(F("Y轴镜像："));
  Serial.println(MIRROR_Y_AXIS ? F("启用") : F("禁用"));
  Serial.println(F("------------------------"));
  Serial.println();
}

void resetCoordinates() {
  for (byte i = 0; i < 5; i++) {
    digits[i].x = -1;
    digits[i].y = -1;
    digits[i].set = false;
  }
  receivedCount = 0;
  readyToDraw = false;
  Serial.println(F("所有坐标已重置"));
}

// 显示当前方向设置
void showDirection() {
  Serial.println();
  Serial.println(F("当前方向设置："));
  Serial.println(F("------------------------"));
  Serial.print(F("X轴方向："));
  Serial.println(xReversed ? F("翻转") : F("正常"));
  Serial.print(F("Y轴方向："));
  Serial.println(yReversed ? F("翻转") : F("正常"));
  Serial.print(F("Y轴镜像："));
  Serial.println(MIRROR_Y_AXIS ? F("启用") : F("禁用"));
  Serial.print(F("放大比例："));
  Serial.print(SCALE_FACTOR);
  Serial.println(F("倍"));
  Serial.print(F("坐标偏移：X:"));
  Serial.print(X_OFFSET);
  Serial.print(F("mm, Y:"));
  Serial.print(Y_OFFSET);
  Serial.println(F("mm"));
  Serial.print(F("圆直径："));
  Serial.print(CIRCLE_DIAMETER);
  Serial.println(F("mm"));
  
  // 说明当前状态下的移动方向
  Serial.println(F("当前移动方向："));
  Serial.print(F("  X轴："));
  Serial.println(xReversed ? F("向右移动时电机反转") : F("向右移动时电机正转"));
  Serial.print(F("  Y轴："));
  Serial.println(yReversed ? F("向上移动时电机反转") : F("向上移动时电机正转"));
  if (MIRROR_Y_AXIS) {
    Serial.println(F("  Y轴镜像：坐标沿Y轴中心线翻转"));
  }
  Serial.print(F("  X偏移：所有X坐标减少"));
  Serial.print(-X_OFFSET);
  Serial.println(F("mm"));
  Serial.print(F("  Y偏移：所有Y坐标减少"));
  Serial.print(-Y_OFFSET);
  Serial.println(F("mm"));
  Serial.println(F("使用 'x' 或 'y' 命令切换方向"));
  Serial.println(F("------------------------"));
  Serial.println();
}

// 测试方向
void testDirection() {
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("方向测试"));
  Serial.println(F("========================================"));
  Serial.println(F("注意：请确保笔已抬起"));
  Serial.println(F("注意：Y轴镜像已启用"));
  Serial.println(F("注意：所有坐标已应用1.6倍放大和偏移"));
  Serial.println();
  
  // 记录当前位置
  float savedX = currentX;
  float savedY = currentY;
  
  // 测试X轴
  Serial.println(F("1. 测试X轴"));
  Serial.println(xReversed ? F("   期望：向右移动30mm（电机反转）") : F("   期望：向右移动30mm（电机正转）"));
  Serial.print(F("   从 "));
  Serial.print(currentX, 1);
  Serial.println(F("mm 开始"));
  delay(1000);
  
  // 向右移动30mm
  float testX = currentX + 30;
  if (testX > PAPER_WIDTH_MM) testX = PAPER_WIDTH_MM;
  moveTo(testX, currentY, MOVE_SPEED);
  
  Serial.print(F("   现在位置："));
  Serial.print(currentX, 1);
  Serial.println(F("mm"));
  Serial.println();
  delay(1000);
  
  // 测试Y轴（注意镜像效果）
  Serial.println(F("2. 测试Y轴"));
  if (MIRROR_Y_AXIS) {
    Serial.println(F("   注意：Y轴镜像已启用，像素坐标与实际移动方向相反"));
  }
  Serial.println(yReversed ? F("   期望：向上移动30mm（电机反转）") : F("   期望：向上移动30mm（电机正转）"));
  Serial.print(F("   从 "));
  Serial.print(currentY, 1);
  Serial.println(F("mm 开始"));
  delay(1000);
  
  // 向上移动30mm
  float testY = currentY + 30;
  if (testY > PAPER_HEIGHT_MM) testY = PAPER_HEIGHT_MM;
  moveTo(currentX, testY, MOVE_SPEED);
  
  Serial.print(F("   现在位置："));
  Serial.print(currentY, 1);
  Serial.println(F("mm"));
  Serial.println();
  delay(1000);
  
  // 返回原位置
  Serial.println(F("3. 返回测试前位置"));
  moveTo(savedX, savedY, MOVE_SPEED);
  
  Serial.println(F("✓ 测试完成！"));
  Serial.println(F("========================================"));
  Serial.println();
}

void showHelp() {
  Serial.println();
  Serial.println(F("帮助信息："));
  Serial.println(F("------------------------"));
  Serial.println(F("系统特性："));
  Serial.println(F("  - X轴和Y轴电机方向已翻转"));
  Serial.println(F("  - 启用Y轴镜像"));
  Serial.println(F("  - 坐标放大1.3倍"));
  Serial.println(F("  - 所有机械坐标x减少50mm，y减少15mm"));
  Serial.println(F("  - 圆直径16mm"));
  Serial.println(F("  - 奇数绘制圆内接等边三角形"));
  Serial.println();
  Serial.println(F("1. 输入坐标格式："));
  Serial.println(F("   Digit 1: Center at (128, 138)"));
  Serial.println(F("   需要输入5个数字(1-5)"));
  Serial.println();
  Serial.println(F("2. 绘制命令："));
  Serial.println(F("   1,2,3,4,5 - 绘制单个数字的圆圈"));
  Serial.println(F("   a/A       - 绘制所有数字的圆圈"));
  Serial.println(F("   e/E       - 偶数用○，奇数用圆内接△"));
  Serial.println();
  Serial.println(F("3. 方向控制命令："));
  Serial.println(F("   x/X       - 切换X轴方向"));
  Serial.println(F("   y/Y       - 切换Y轴方向"));
  Serial.println(F("   d/D       - 显示当前方向"));
  Serial.println(F("   t/T       - 测试方向"));
  Serial.println();
  Serial.println(F("4. 其他命令："));
  Serial.println(F("   h - 返回原点"));
  Serial.println(F("   l - 查看坐标列表"));
  Serial.println(F("   r - 重置所有坐标"));
  Serial.println(F("   s - 查看系统状态"));
  Serial.println(F("   ? - 显示帮助"));
  Serial.println(F("------------------------"));
  Serial.println();
}

void showStatus() {
  Serial.print(F("系统状态："));
  Serial.print(receivedCount);
  Serial.print(F("/5 坐标，"));
  Serial.println(readyToDraw ? F("可以绘制") : F("等待坐标"));
  Serial.print(F("当前位置：("));
  Serial.print(currentX, 1);
  Serial.print(F(", "));
  Serial.print(currentY, 1);
  Serial.println(F(") mm"));
  Serial.print(F("X轴方向："));
  Serial.print(xReversed ? F("翻转") : F("正常"));
  Serial.print(F("，Y轴方向："));
  Serial.println(yReversed ? F("翻转") : F("正常"));
  Serial.print(F("Y轴镜像："));
  Serial.println(MIRROR_Y_AXIS ? F("启用") : F("禁用"));
  Serial.print(F("放大比例："));
  Serial.print(SCALE_FACTOR);
  Serial.println(F("倍"));
  Serial.print(F("坐标偏移：X:"));
  Serial.print(X_OFFSET);
  Serial.print(F("mm, Y:"));
  Serial.print(Y_OFFSET);
  Serial.println(F("mm"));
  Serial.print(F("圆直径："));
  Serial.print(CIRCLE_DIAMETER);
  Serial.println(F("mm"));
  Serial.print(F("移动速度："));
  Serial.print(MOVE_SPEED);
  Serial.print(F("μs，绘制速度："));
  Serial.print(DRAW_SPEED);
  Serial.println(F("μs (值越小越快)"));
}