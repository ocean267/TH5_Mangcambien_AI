#include <Wire.h>
#include <BH1750.h>
#include <ocean5644-project-1_inferencing.h>  
#define I2C_SDA_PIN 32
#define I2C_SCL_PIN 26
BH1750 lightMeter;

#define BH1750_FREQ_HZ 10  
unsigned long lastBH1750_ms = 0;
float ei_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
size_t buffer_index = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("Không tìm thấy BH1750!");
    while (1);
  }

  Serial.println("Đã tìm thấy BH1750.");
}

void loop() {
  unsigned long now = millis();
  if (now - lastBH1750_ms >= (1000 / BH1750_FREQ_HZ)) {
    lastBH1750_ms = now;
    float lux = lightMeter.readLightLevel();
    if (!isnan(lux)) {
      ei_buffer[buffer_index++] = lux;
    } else {
      Serial.println("Lỗi đọc BH1750.");
    }
    if (buffer_index >= EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
      signal_t signal;
      int err = numpy::signal_from_buffer(ei_buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
      if (err != 0) {
        Serial.printf("Lỗi tạo tín hiệu: %d\n", err);
        buffer_index = 0;
        return;
      }
      ei_impulse_result_t result;
      EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
      if (res != EI_IMPULSE_OK) {
        Serial.printf("Lỗi phân loại: %d\n", res);
        buffer_index = 0;
        return;
      }
      Serial.println("Kết quả phân loại:");
      for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        Serial.printf("  %s: %.2f\n",
                      result.classification[i].label,
                      result.classification[i].value);
      }
      Serial.println("*********************");
      buffer_index = 0;
    }
  }
}
