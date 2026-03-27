import tensorflow as tf
import numpy as np
import matplotlib.pyplot as plt
import time

print("TensorFlow version:", tf.__version__)
print("NumPy version:", np.__version__)

# 记录开始时间
start_time = time.time()

# 加载MNIST数据集
print("\nLoading MNIST dataset...")
mnist = tf.keras.datasets.mnist
(x_train, y_train), (x_test, y_test) = mnist.load_data()

print(f"Training data shape: {x_train.shape}")
print(f"Training labels shape: {y_train.shape}")
print(f"Test data shape: {x_test.shape}")
print(f"Test labels shape: {y_test.shape}")

# 显示数据集中的一些样本
plt.figure(figsize=(10, 4))
for i in range(10):
    plt.subplot(2, 5, i + 1)
    plt.imshow(x_train[i], cmap='gray')
    plt.title(f"Label: {y_train[i]}")
    plt.axis('off')
plt.suptitle('First 10 Training Samples', fontsize=14)
plt.tight_layout()
plt.show()

# 数据归一化
print("\nNormalizing data...")
x_train = x_train / 255.0
x_test = x_test / 255.0

# 构建模型
print("\nBuilding model...")
model = tf.keras.Sequential([
    tf.keras.layers.Flatten(input_shape=(28, 28)),  # Flatten 28x28 images to 784-dim vector
    tf.keras.layers.Dense(128, activation='relu'),  # Fully connected layer with 128 neurons
    tf.keras.layers.Dropout(0.2),  # Dropout layer to prevent overfitting
    tf.keras.layers.Dense(10, activation='softmax')  # Output layer for 10 classes (0-9)
])

# 编译模型
model.compile(optimizer='adam',
              loss='sparse_categorical_crossentropy',
              metrics=['accuracy'])

# 显示模型结构
print("\n" + "=" * 50)
print("Model Architecture:")
print("=" * 50)
model.summary()

# 训练模型
print("\n" + "=" * 50)
print("Training model...")
print("=" * 50)

history = model.fit(x_train, y_train,
                    validation_split=0.2,  # 使用20%的训练数据作为验证集
                    epochs=10,
                    batch_size=32,
                    verbose=1)

# 计算训练时间
training_time = time.time() - start_time
print(f"\nTotal training time: {training_time:.2f} seconds")

# 评估模型
print("\n" + "=" * 50)
print("Evaluating model on test set...")
print("=" * 50)
test_loss, test_acc = model.evaluate(x_test, y_test, verbose=0)
print(f"\nTest Accuracy: {test_acc:.4f} ({test_acc * 100:.2f}%)")
print(f"Test Loss: {test_loss:.4f}")

# 可视化训练过程
plt.figure(figsize=(14, 5))

plt.subplot(1, 2, 1)
plt.plot(history.history['accuracy'], label='Training Accuracy', linewidth=2)
plt.plot(history.history['val_accuracy'], label='Validation Accuracy', linewidth=2)
plt.title('Model Accuracy', fontsize=14, fontweight='bold')
plt.xlabel('Epoch', fontsize=12)
plt.ylabel('Accuracy', fontsize=12)
plt.legend(fontsize=11)
plt.grid(True, alpha=0.3)
plt.xticks(range(0, 10))

plt.subplot(1, 2, 2)
plt.plot(history.history['loss'], label='Training Loss', linewidth=2)
plt.plot(history.history['val_loss'], label='Validation Loss', linewidth=2)
plt.title('Model Loss', fontsize=14, fontweight='bold')
plt.xlabel('Epoch', fontsize=12)
plt.ylabel('Loss', fontsize=12)
plt.legend(fontsize=11)
plt.grid(True, alpha=0.3)
plt.xticks(range(0, 10))

plt.suptitle('Training History', fontsize=16, fontweight='bold')
plt.tight_layout()
plt.show()

# 进行预测
print("\n" + "=" * 50)
print("Making predictions on test set...")
print("=" * 50)
predictions = model.predict(x_test, verbose=0)

# 可视化前10个测试样本的预测结果
fig, axes = plt.subplots(2, 5, figsize=(14, 7))

for i in range(10):
    ax = axes[i // 5, i % 5]
    ax.imshow(x_test[i], cmap='gray')
    actual_label = y_test[i]
    predicted_label = np.argmax(predictions[i])
    confidence = np.max(predictions[i])

    # 设置标题颜色：绿色表示预测正确，红色表示预测错误
    color = 'green' if actual_label == predicted_label else 'red'
    ax.set_title(f"Actual: {actual_label}\nPredicted: {predicted_label}\nConfidence: {confidence:.2%}",
                 color=color, fontsize=8, pad=5)
    ax.axis('off')

plt.suptitle('MNIST Test Samples - First 10 Predictions', fontsize=16, fontweight='bold')
plt.tight_layout()
plt.show()

# 显示详细的预测结果统计
print("\n" + "=" * 50)
print("Detailed predictions for first 10 test samples:")
print("=" * 50)

correct_count = 0
for i in range(10):
    actual_label = y_test[i]
    predicted_label = np.argmax(predictions[i])
    confidence = np.max(predictions[i])
    is_correct = actual_label == predicted_label

    if is_correct:
        correct_count += 1
        status = "✓ CORRECT"
        status_color = "\033[92m"  # 绿色
    else:
        status = "✗ WRONG"
        status_color = "\033[91m"  # 红色

    print(f"Sample {i:2d}: Actual={actual_label}, Predicted={predicted_label}, "
          f"Confidence={confidence:.4f} {status_color}{status}\033[0m")

print(f"\nAccuracy on first 10 samples: {correct_count}/10 = {correct_count / 10:.0%}")

# 计算整个测试集的准确率
all_predictions = np.argmax(predictions, axis=1)
test_accuracy = np.mean(all_predictions == y_test)
print(f"Overall test accuracy: {test_accuracy:.4f} ({test_accuracy * 100:.2f}%)")

# 混淆矩阵（前100个样本）
print("\n" + "=" * 50)
print("Confusion Matrix (first 100 samples):")
print("=" * 50)

from sklearn.metrics import confusion_matrix
import seaborn as sns

# 使用前100个样本创建混淆矩阵
cm = confusion_matrix(y_test[:100], all_predictions[:100])

plt.figure(figsize=(10, 8))
sns.heatmap(cm, annot=True, fmt='d', cmap='Blues', cbar=False,
            xticklabels=range(10), yticklabels=range(10))
plt.title('Confusion Matrix (First 100 Samples)', fontsize=16, fontweight='bold')
plt.xlabel('Predicted Label', fontsize=12)
plt.ylabel('True Label', fontsize=12)
plt.tight_layout()
plt.show()

# 显示每个数字的准确率
print("\n" + "=" * 50)
print("Accuracy per digit:")
print("=" * 50)

for digit in range(10):
    indices = y_test == digit
    digit_accuracy = np.mean(all_predictions[indices] == digit)
    print(f"Digit {digit}: {digit_accuracy:.4f} ({digit_accuracy * 100:6.2f}%)")

# 找出预测错误的样本
print("\n" + "=" * 50)
print("Finding misclassified samples...")
print("=" * 50)

incorrect_indices = np.where(all_predictions != y_test)[0]
print(f"Number of misclassified samples: {len(incorrect_indices)} / {len(y_test)} "
      f"({len(incorrect_indices) / len(y_test):.2%})")

# 可视化一些错误分类的样本
if len(incorrect_indices) > 0:
    print(f"\nDisplaying first {min(8, len(incorrect_indices))} misclassified samples:")

    num_samples = min(8, len(incorrect_indices))
    fig, axes = plt.subplots(2, 4, figsize=(14, 7))

    for i, idx in enumerate(incorrect_indices[:num_samples]):
        ax = axes[i // 4, i % 4]
        ax.imshow(x_test[idx], cmap='gray')
        actual = y_test[idx]
        predicted = all_predictions[idx]
        confidence = np.max(predictions[idx])

        ax.set_title(f"Actual: {actual}\nPredicted: {predicted}\nConf: {confidence:.2%}",
                     color='red', fontsize=11, pad=10)
        ax.axis('off')

    plt.suptitle('Misclassified Samples', fontsize=16, fontweight='bold')
    plt.tight_layout()
    plt.show()

# 保存模型（可选）
print("\n" + "=" * 50)
print("Saving model...")
print("=" * 50)

model.save('mnist_model.h5')
print("Model saved as 'mnist_model.h5'")

# 加载模型进行验证（可选）
print("\n" + "=" * 50)
print("Loading model for verification...")
print("=" * 50)

loaded_model = tf.keras.models.load_model('mnist_model.h5')
loaded_test_loss, loaded_test_acc = loaded_model.evaluate(x_test, y_test, verbose=0)
print(f"Loaded model test accuracy: {loaded_test_acc:.4f} ({loaded_test_acc * 100:.2f}%)")
print(f"Original model test accuracy: {test_acc:.4f} ({test_acc * 100:.2f}%)")

if abs(loaded_test_acc - tet_acc) < 0.001:
    print("✓ Model saved and loaded successfully!")
else:
    print("⚠ There might be an issue with model saving/loading")

print("\n" + "=" * 50)
print("MNIST Classification Complete!")
print("=" * 50)