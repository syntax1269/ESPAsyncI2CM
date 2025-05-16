#include <ESPAsyncI2CM.h>

ESPAsyncI2CM i2c;

void onSendComplete(uint8_t status, void *arg) {
  Serial.print("Send complete, status: ");
  Serial.println(status);
}

void setup() {
  Serial.begin(115200);
  i2c.begin(); // For ESP32/ESP8266 default pins, or i2c.begin(sda, scl);
  uint8_t data[] = {0x01, 0x02, 0x03};
  i2c.send(0x3C, data, sizeof(data), onSendComplete);
}

void loop() {
  i2c.loop();
}