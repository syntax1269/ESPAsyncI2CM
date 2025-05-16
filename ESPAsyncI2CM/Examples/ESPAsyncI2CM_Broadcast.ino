#include <ESPAsyncI2CM.h>

ESPAsyncI2CM i2c;

void onBroadcastComplete(uint8_t status, void *arg) {
  Serial.print("Broadcast complete, status: ");
  Serial.println(status);
}

void setup() {
  Serial.begin(115200);
  i2c.begin();
  uint8_t data[] = {0xA5, 0x5A};
  // 0x00 is the general call address for broadcast
  i2c.broadcast(data, sizeof(data), onBroadcastComplete);
}

void loop() {
  i2c.loop();
}