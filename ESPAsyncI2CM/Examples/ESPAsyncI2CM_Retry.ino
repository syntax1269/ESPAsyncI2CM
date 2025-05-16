#include <ESPAsyncI2CM.h>

ESPAsyncI2CM i2c;

void onSendComplete(uint8_t status, void *arg) {
  Serial.print("Send with retry complete, status: ");
  Serial.println(status);
  if (status != I2C_STATUS_OK) {
    Serial.println("Failed after retries.");
  }
}

void setup() {
  Serial.begin(115200);
  i2c.begin();
  i2c.setRetryCount(3); // Retry up to 3 times on failure
  uint8_t data[] = {0x10, 0x20};
  i2c.send(0x3C, data, sizeof(data), onSendComplete);
}

void loop() {
  i2c.loop();
}