#include <ESPAsyncI2CM.h>

ESPAsyncI2CM i2c;

void onReadComplete(uint8_t status, void *arg, uint8_t *data, uint8_t len) {
  Serial.print("Read complete, status: ");
  Serial.println(status);
  if (status == I2C_STATUS_OK) {
    Serial.print("Data: ");
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
  i2c.read(0x3C, 4, onReadComplete);
}

void loop() {
  i2c.loop();
}