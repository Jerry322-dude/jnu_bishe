import cv2
import numpy as np
from collections import defaultdict
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers
import os
import math
import time

# 设置TensorFlow日志级别
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'
os.environ['TF_ENABLE_ONEDNN_OPTS'] = '0'

# 定义不同数字对应的颜色 (BGR格式)
color_map = {
    1: (0, 0, 255),  # 红色 - 数字1
    2: (0, 255, 0),  # 绿色 - 数字2
    3: (255, 0, 0),  # 蓝色 - 数字3
    4: (0, 255, 255),  # 黄色 - 数字4
    5: (255, 0, 255)  # 品红色 - 数字5
}


def create_improved_model(input_shape=(28, 28, 1), num_classes=5):
    """创建改进的CNN模型，提高识别准确性"""
    model = keras.Sequential([
        layers.Input(shape=input_shape),

        # 第一卷积层
        layers.Conv2D(32, (3, 3), activation='relu', padding='same'),
        layers.BatchNormalization(),
        layers.Conv2D(32, (3, 3), activation='relu', padding='same'),
        layers.BatchNormalization(),
        layers.MaxPooling2D((2, 2)),
        layers.Dropout(0.2),

        # 第二卷积层
        layers.Conv2D(64, (3, 3), activation='relu', padding='same'),
        layers.BatchNormalization(),
        layers.Conv2D(64, (3, 3), activation='relu', padding='same'),
        layers.BatchNormalization(),
        layers.MaxPooling2D((2, 2)),
        layers.Dropout(0.3),

        # 第三卷积层
        layers.Conv2D(128, (3, 3), activation='relu', padding='same'),
        layers.BatchNormalization(),
        layers.MaxPooling2D((2, 2)),
        layers.Dropout(0.4),

        # 全连接层
        layers.Flatten(),
        layers.Dense(256, activation='relu'),
        layers.BatchNormalization(),
        layers.Dropout(0.5),
        layers.Dense(128, activation='relu'),
        layers.BatchNormalization(),
        layers.Dropout(0.4),
        layers.Dense(num_classes, activation='softmax')
    ])

    # 使用学习率调度
    optimizer = keras.optimizers.Adam(learning_rate=0.001)
    model.compile(optimizer=optimizer,
                  loss='sparse_categorical_crossentropy',
                  metrics=['accuracy'])

    return model


def load_mnist_data():
    """加载并预处理MNIST数据（只取1-5）"""
    (x_train, y_train), (x_test, y_test) = keras.datasets.mnist.load_data()

    train_mask = np.isin(y_train, [1, 2, 3, 4, 5])
    test_mask = np.isin(y_test, [1, 2, 3, 4, 5])

    x_train = x_train[train_mask]
    y_train = y_train[train_mask]
    x_test = x_test[test_mask]
    y_test = y_test[test_mask]

    y_train = y_train - 1
    y_test = y_test - 1

    x_train = x_train.reshape(-1, 28, 28, 1).astype('float32') / 255.0
    x_test = x_test.reshape(-1, 28, 28, 1).astype('float32') / 255.0

    return (x_train, y_train), (x_test, y_test)


def simple_augmentation(images, labels):
    """简单的数据增强函数，不依赖scipy"""
    augmented_images = []
    augmented_labels = []

    for img, label in zip(images, labels):
        # 原始图像
        augmented_images.append(img)
        augmented_labels.append(label)

        # 轻微旋转（±5度）
        for angle in [5, -5]:
            M = cv2.getRotationMatrix2D((14, 14), angle, 1.0)
            rotated = cv2.warpAffine(img.squeeze(), M, (28, 28),
                                     flags=cv2.INTER_LINEAR,
                                     borderMode=cv2.BORDER_REPLICATE)
            rotated = rotated.reshape(28, 28, 1)
            augmented_images.append(rotated)
            augmented_labels.append(label)

        # 轻微平移
        for shift_x, shift_y in [(2, 0), (-2, 0), (0, 2), (0, -2)]:
            M = np.float32([[1, 0, shift_x], [0, 1, shift_y]])
            shifted = cv2.warpAffine(img.squeeze(), M, (28, 28),
                                     flags=cv2.INTER_LINEAR,
                                     borderMode=cv2.BORDER_REPLICATE)
            shifted = shifted.reshape(28, 28, 1)
            augmented_images.append(shifted)
            augmented_labels.append(label)

    return np.array(augmented_images), np.array(augmented_labels)


def train_model():
    """训练模型，使用自定义数据增强"""
    print("Training improved digit recognition model...")
    (x_train, y_train), (x_test, y_test) = load_mnist_data()

    # 应用简单的数据增强
    print("Applying data augmentation...")
    x_train_aug, y_train_aug = simple_augmentation(x_train, y_train)
    print(f"Training samples increased from {len(x_train)} to {len(x_train_aug)}")

    model = create_improved_model()

    # 简单回调函数
    early_stopping = keras.callbacks.EarlyStopping(
        monitor='val_loss',
        patience=5,
        restore_best_weights=True
    )

    # 划分验证集
    val_size = int(0.15 * len(x_train_aug))
    x_val = x_train_aug[:val_size]
    y_val = y_train_aug[:val_size]
    x_train_final = x_train_aug[val_size:]
    y_train_final = y_train_aug[val_size:]

    # 训练模型
    history = model.fit(
        x_train_final, y_train_final,
        batch_size=32,
        epochs=10,
        validation_data=(x_val, y_val),
        callbacks=[early_stopping],
        verbose=1
    )

    test_loss, test_acc = model.evaluate(x_test, y_test, verbose=0)
    print(f"Model test accuracy: {test_acc:.4f}")

    return model


def preprocess_image_for_detection(image):
    """预处理图像以检测数字"""
    # 转换为灰度图
    if len(image.shape) == 3:
        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    else:
        gray = image.copy()

    # 高斯模糊
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)

    # 自适应阈值
    binary = cv2.adaptiveThreshold(blurred, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
                                   cv2.THRESH_BINARY_INV, 11, 2)

    # 形态学操作
    kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (3, 3))
    cleaned = cv2.morphologyEx(binary, cv2.MORPH_CLOSE, kernel)
    cleaned = cv2.morphologyEx(cleaned, cv2.MORPH_OPEN, kernel)

    return image, cleaned, gray


def find_digit_contours(binary_image, min_area=100):
    """找到可能是数字的轮廓"""
    # 使用RETR_EXTERNAL只获取外部轮廓
    contours, _ = cv2.findContours(binary_image, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    valid_contours = []
    valid_boxes = []

    for contour in contours:
        x, y, w, h = cv2.boundingRect(contour)
        area = cv2.contourArea(contour)

        # 面积限制
        if area < min_area:
            continue

        # 宽高比限制
        aspect_ratio = w / h
        if aspect_ratio < 0.3 or aspect_ratio > 2.5:
            continue

        # 尺寸限制
        if h < 20 or w < 10:
            continue

        valid_contours.append(contour)
        valid_boxes.append((x, y, w, h))

    # 按面积排序，最多只保留5个最大的
    if len(valid_boxes) > 0:
        # 计算面积
        areas = [w * h for (x, y, w, h) in valid_boxes]

        # 按面积排序（从大到小）
        sorted_indices = np.argsort(areas)[::-1]

        # 只保留前5个最大的（最多5个数字）
        max_digits = 5
        if len(sorted_indices) > max_digits:
            sorted_indices = sorted_indices[:max_digits]

        valid_contours = [valid_contours[i] for i in sorted_indices]
        valid_boxes = [valid_boxes[i] for i in sorted_indices]

    return valid_contours, valid_boxes


def extract_digit_images(binary_image, contours, boxes):
    """从轮廓中提取数字图像"""
    digit_images = []

    for contour, (x, y, w, h) in zip(contours, boxes):
        mask = np.zeros_like(binary_image)
        cv2.drawContours(mask, [contour], -1, 255, -1)

        roi = binary_image[y:y + h, x:x + w]
        mask_roi = mask[y:y + h, x:x + w]
        digit = cv2.bitwise_and(roi, mask_roi)

        # 添加边框
        border_size = max(int(min(w, h) * 0.2), 10)
        digit_with_border = cv2.copyMakeBorder(digit,
                                               border_size, border_size,
                                               border_size, border_size,
                                               cv2.BORDER_CONSTANT,
                                               value=0)

        # 调整大小到28x28
        digit_resized = cv2.resize(digit_with_border, (28, 28), interpolation=cv2.INTER_AREA)

        # 标准化
        digit_normalized = digit_resized / 255.0
        digit_normalized = digit_normalized.reshape(28, 28, 1)

        digit_images.append(digit_normalized)

    return digit_images


def predict_digits(model, digit_images):
    """预测数字"""
    if not digit_images:
        return [], []

    digit_array = np.array(digit_images)

    # 预测
    predictions = model.predict(digit_array, verbose=0)

    predicted_classes = np.argmax(predictions, axis=1) + 1
    confidences = np.max(predictions, axis=1)

    return predicted_classes, confidences


def convert_to_top_left_coordinates(x, y, w, h, img_width, img_height):
    """将坐标原点设在画面左上角，返回矩形中心坐标"""
    # 计算矩形中心坐标（以左上角为原点）
    center_x = x + w // 2
    center_y = y + h // 2

    return center_x, center_y


def enforce_exactly_5_digits(detected_info, boxes, predictions, confidences):
    """确保最多只识别5个数字，每个数字只出现一次"""
    if not detected_info:
        return [], [], [], []

    # 第一步：按置信度排序
    combined = list(zip(detected_info, predictions, confidences, boxes))
    combined.sort(key=lambda x: x[0]['confidence'], reverse=True)

    # 第二步：确保每个数字只出现一次（保留置信度最高的）
    digit_dict = {}
    for info, pred, conf, box in combined:
        digit = info['digit']
        if digit not in digit_dict or conf > digit_dict[digit]['confidence']:
            digit_dict[digit] = {
                'info': info,
                'prediction': pred,
                'confidence': conf,
                'box': box
            }

    # 第三步：只保留前5个（如果超过5个）
    digit_items = list(digit_dict.items())
    if len(digit_items) > 5:
        digit_items.sort(key=lambda x: x[1]['confidence'], reverse=True)
        digit_items = digit_items[:5]

    # 第四步：重新组织数据
    final_info = []
    final_predictions = []
    final_confidences = []
    final_boxes = []

    for digit, data in digit_items:
        final_info.append(data['info'])
        final_predictions.append(data['prediction'])
        final_confidences.append(data['confidence'])
        final_boxes.append(data['box'])

    return final_info, final_predictions, final_confidences, final_boxes


def draw_digit_positions(image, detected_info):
    """在图像左下角标注数字位置，按数字1-5顺序排列，简化显示"""
    if not detected_info:
        return image

    height, width = image.shape[:2]

    # 按数字1-5的顺序排序
    detected_info_sorted = sorted(detected_info, key=lambda x: x['digit'])

    # 显示位置信息（没有灰色背景框）
    y_offset = height - 30

    # 从下往上显示
    for info in reversed(detected_info_sorted):
        digit = info['digit']
        center_x, center_y = info['center_position']
        conf = info['confidence']

        # 获取该数字对应的颜色
        color = color_map[digit]

        # 创建显示文本
        text = f"Digit {digit}: ({center_x}, {center_y})"
        if conf < 0.7:  # 如果置信度较低，显示置信度
            text += f" ({conf:.2f})"

        # 绘制文本（直接绘制，没有背景）
        cv2.putText(image, text, (10, y_offset),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)

        y_offset -= 30

    return image


def create_detection_image(frame, model):
    """处理摄像头帧，返回检测结果"""
    if frame is None:
        return frame, []

    # 预处理图像
    processed_img, binary_img, _ = preprocess_image_for_detection(frame)

    # 查找数字轮廓（最多5个）
    contours, boxes = find_digit_contours(binary_img)

    if not contours:
        return processed_img, []

    # 提取数字图像
    digit_images = extract_digit_images(binary_img, contours, boxes)

    if not digit_images:
        return processed_img, []

    # 预测数字
    predictions, confidences = predict_digits(model, digit_images)

    # 创建结果图像
    result_image = processed_img.copy()
    height, width = result_image.shape[:2]
    detected_info = []

    # 绘制检测框和收集信息
    for i, ((x, y, w, h), pred, conf) in enumerate(zip(boxes, predictions, confidences)):
        # 只识别1-5的数字
        if pred in color_map and conf > 0.3:
            color = color_map[pred]

            # 记录信息（使用左上角为原点的坐标系）
            info = {
                'digit': pred,
                'confidence': conf,
                'center_position': convert_to_top_left_coordinates(x, y, w, h, width, height),
                'original_bbox': (x, y, w, h),
                'index': i
            }
            detected_info.append(info)

            # 绘制矩形框
            cv2.rectangle(result_image, (x, y), (x + w, y + h), color, 2)

            # 在框上方显示数字
            box_center_x = x + w // 2
            label_text = f"Digit {pred}"
            if conf < 0.7:
                label_text += f" ({conf:.2f})"

            text_size = cv2.getTextSize(label_text, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)[0]
            text_x = x + (w - text_size[0]) // 2
            text_y = y - 10 if y > 20 else y + h + 15

            cv2.putText(result_image, label_text, (text_x, text_y),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 1)

    # 确保最多只识别5个数字，每个数字只出现一次
    if detected_info:
        detected_info, predictions, confidences, boxes = enforce_exactly_5_digits(
            detected_info, boxes, predictions, confidences
        )

    # 在左下角标注数字位置（简化显示，无背景框）
    result_image = draw_digit_positions(result_image, detected_info)

    return result_image, detected_info


def main():
    print("=" * 70)
    print("REAL-TIME DIGIT RECOGNITION FROM CAMERA (1-5 DIGITS ONLY)")
    print("=" * 70)

    # 加载或训练模型
    model_path = 'digit_model_improved.h5'

    if os.path.exists(model_path):
        print("\nLoading pre-trained model...")
        try:
            model = keras.models.load_model(model_path)
            print("✓ Model loaded successfully")
        except Exception as e:
            print(f"✗ Error loading model: {e}")
            print("\nTraining improved model...")
            model = train_model()
            model.save(model_path)
            print(f"✓ Model saved: {model_path}")
    else:
        print("\nTraining improved model...")
        model = train_model()
        model.save(model_path)
        print(f"✓ Model saved: {model_path}")

    # 打开摄像头
    print("\nOpening camera...")
    cap = cv2.VideoCapture(0)  # 0 表示默认摄像头

    # 设置摄像头分辨率
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)

    if not cap.isOpened():
        print("✗ Cannot open camera!")
        return False

    print("✓ Camera opened successfully")
    print("\nSystem will detect ONLY 1-5 digits")
    print("Coordinates origin: TOP-LEFT corner")
    print("Press 's' to save current frame")
    print("Press 'q' to quit")

    frame_count = 0
    fps = 0
    start_time = time.time()
    save_count = 0

    # 创建窗口
    cv2.namedWindow('Real-time Digit Detection', cv2.WINDOW_NORMAL)
    cv2.resizeWindow('Real-time Digit Detection', 1200, 800)

    while True:
        ret, frame = cap.read()
        if not ret:
            print("✗ Cannot read frame from camera!")
            break

        # 计算FPS
        frame_count += 1
        if frame_count % 30 == 0:
            end_time = time.time()
            fps = 30 / (end_time - start_time)
            start_time = end_time

        # 处理当前帧
        result_img, detected_info = create_detection_image(frame, model)

        # 显示FPS
        fps_text = f"FPS: {fps:.1f}"
        cv2.putText(result_img, fps_text, (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        cv2.putText(result_img, fps_text, (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 1)

        # 显示检测到的数字数量
        detected_count = len(detected_info)
        count_text = f"Detected: {detected_count} digits"
        cv2.putText(result_img, count_text, (10, 60),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
        cv2.putText(result_img, count_text, (10, 60),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 1)

        # 显示坐标系说明
        coord_text = "Coordinates: Top-left origin"
        cv2.putText(result_img, coord_text, (10, 90),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
        cv2.putText(result_img, coord_text, (10, 90),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 1)

        # 显示图像
        cv2.imshow('Real-time Digit Detection', result_img)

        # 键盘操作
        key = cv2.waitKey(1) & 0xFF
        if key == ord('q'):
            print("\nQuitting...")
            break
        elif key == ord('s'):
            # 保存当前帧
            save_count += 1
            filename = f"captured_Frame_{save_count:03d}.jpg"
            cv2.imwrite(filename, result_img)
            print(f"✓ Frame saved as: {filename}")

            # 同时保存位置信息
            if detected_info:
                position_file = f"digit_Positions_{save_count:03d}.txt"
                with open(position_file, 'w', encoding='utf-8') as f:
                    f.write("Digit Recognition Results\n")
                    f.write("=" * 50 + "\n\n")
                    f.write(f"Frame saved: {filename}\n")
                    f.write(f"Total digits detected: {len(detected_info)}\n")
                    f.write(f"Coordinate system: Top-left origin\n\n")

                    # 按数字1-5顺序排序
                    detected_info_sorted = sorted(detected_info, key=lambda x: x['digit'])
                    for info in detected_info_sorted:
                        digit = info['digit']
                        center_x, center_y = info['center_position']
                        conf = info['confidence']
                        f.write(f"Digit {digit}: Center at ({center_x}, {center_y})")
                        if conf < 0.7:
                            f.write(f"  [Confidence: {conf:.3f}]")
                        f.write("\n")
                print(f"✓ Position data saved as: {position_file}")

    # 释放资源
    cap.release()
    cv2.destroyAllWindows()

    print("\n" + "=" * 70)
    print("✓ Processing completed!")
    print(f"✓ Total frames saved: {save_count}")

    return True


if __name__ == "__main__":
    try:
        success = main()
        if not success:
            print("\nProcessing completed")
    except KeyboardInterrupt:
        print("\nProgram interrupted by user")
    except Exception as e:
        print(f"\nError: {e}")
        import traceback

        traceback.print_exc()