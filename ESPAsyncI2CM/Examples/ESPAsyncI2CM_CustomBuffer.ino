#define MASTER_BUFFER_SIZE 256
#include <ESPAsyncI2CM.h>

ESPAsyncI2CM i2c;

void onSendComplete(uint8_t status, void *arg) {
  Serial.print("Large buffer send complete, status: ");
  Serial.println(status);
}

void setup() {
  Serial.begin(115200);
  i2c.begin();
  uint8_t data[200];
  for (uint8_t i = 0; i < 200; ++i) data[i] = i;
  i2c.send(0x3C, data, sizeof(data), onSendComplete);
}

void loop() {
  i2c.loop();
}