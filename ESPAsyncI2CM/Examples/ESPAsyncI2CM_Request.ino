#include <ESPAsyncI2CM.h>

ESPAsyncI2CM i2c;

void onRequestComplete(uint8_t status, void *arg, uint8_t *data, uint8_t len) {
  Serial.print("Request complete, status: ");
  Serial.println(status);
  if (status == I2C_STATUS_OK) {
    Serial.print("Received: ");
    for (uint8_t i = 0; i < len; ++i) {
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  i2c.begin();
  uint8_t reg = 0x00; // Register to read from
  i2c.request(0x3C, &reg, 1, 4, onRequestComplete);
}

void loop() {
  i2c.loop();
}