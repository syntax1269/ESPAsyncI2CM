# ESPAsyncI2CM
Asynchronous I2C master library for ESP8266, ESP32, and AVR. Supports non-blocking, queued I2C transactions with callback support. Optimized for ESP32, ESP8266, and AVR.

# ESPAsyncI2CM Library User Guide

## Overview

ESPAsyncI2CM is an asynchronous, non-blocking I2C master library optimized for ESP8266, ESP32, and AVR platforms. It allows you to queue multiple I2C transactions and handle them efficiently using callbacks, making it ideal for responsive, event-driven applications.

---

## Installation

1. Place the file in your current library under `ESPAsyncI2CM` directory with `ESPAsyncI2CM.h` and `ESPAsyncI2CM.cpp`.
2. Update your sketches to:
   ```cpp
   #include <ESPAsyncI2CM.h>
   ```
   and use `ESPAsyncI2CM`.

---

## Initialization

```cpp
ESPAsyncI2CM i2c;
```

- **Default pins:**  
  ```cpp
  i2c.begin();
  ```
- **Custom pins (ESP32/ESP8266):**  
  ```cpp
  i2c.begin(sda, scl);
  ```

---

## Main API

- `uint8_t send(uint8_t i2cAddress, uint8_t *data, uint8_t dataLen, I2CSendCallback callback, void *arg = 0);`  
  Asynchronously send data to a device.

- `uint8_t read(uint8_t i2cAddress, uint8_t receiveLen, I2CReadCallback callback, void *arg = 0);`  
  Asynchronously read data from a device.

- `uint8_t request(uint8_t i2cAddress, uint8_t *data, uint8_t dataLen, uint8_t receiveLen, I2CRequestCallback callback, void *arg = 0);`  
  Write data, then read from the device (common for register reads).

- `uint8_t broadcast(uint8_t *data, uint8_t dataLen, I2CSendCallback callback, void *arg = 0);`  
  Send data to all devices (general call address 0x00).

- `uint8_t setRetryCount(uint8_t count);`  
  Set the number of retries for failed transactions.

- `void loop();`  
  Must be called regularly in your main loop to process transactions and invoke callbacks.

---

## Callback Types

- `typedef void (*I2CSendCallback)(uint8_t status, void *arg);`
- `typedef void (*I2CReadCallback)(uint8_t status, void *arg, uint8_t *data, uint8_t datalen);`
- `typedef void (*I2CRequestCallback)(uint8_t status, void *arg, uint8_t *data, uint8_t datalen);`

---

## Status Codes

- `I2C_STATUS_OK (0)`: Success
- `I2C_STATUS_OUT_OF_MEMORY (1)`: Buffer full
- `I2C_STATUS_DEVICE_NOT_PRESENT (2)`: No device found
- `I2C_STATUS_TRANSMIT_ERROR (3)`: Transmission error
- `I2C_STATUS_NEGATIVE_ACKNOWLEDGE (4)`: NACK received

---

## Basic Usage Examples

### 1. Basic Write (Send)

```cpp
#include <ESPAsyncI2CM.h>
ESPAsyncI2CM i2c;

void onSendComplete(uint8_t status, void *arg) {
  Serial.print("Send complete, status: ");
  Serial.println(status);
}

void setup() {
  Serial.begin(115200);
  i2c.begin();
  uint8_t data[] = {0x01, 0x02, 0x03};
  i2c.send(0x3C, data, sizeof(data), onSendComplete);
}

void loop() {
  i2c.loop();
}
```

### 2. Basic Read

```cpp
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
```

### 3. Write-Then-Read (Request)

```cpp
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
```

---

## Advanced Usage Examples

### 4. Broadcast Write

```cpp
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
  i2c.broadcast(data, sizeof(data), onBroadcastComplete);
}

void loop() {
  i2c.loop();
}
```

### 5. Retry Logic

```cpp
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
```

### 6. Custom Buffer Size

```cpp
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
```

---

## Notes & Tips

- **Always call `i2c.loop();` in your main loop.**  
  This is required for processing queued transactions and invoking callbacks.

- **Callbacks are invoked from the context of `loop()`.**

- **You can queue multiple transactions;** they will be processed in order.

- **Custom Pins:**  
  For ESP32/ESP8266, use `i2c.begin(sda, scl);` to select pins.

- **Custom Buffer Size:**  
  Define `MASTER_BUFFER_SIZE` before including the library to increase the transaction buffer.

- **Retry Logic:**  
  Use `setRetryCount(n)` to automatically retry failed transactions.

- **Broadcast:**  
  Use `broadcast()` to send to all devices (address 0x00). Not all devices will respond.

---

## Troubleshooting

- **I2C_STATUS_OUT_OF_MEMORY:**  
  Increase `MASTER_BUFFER_SIZE` if you need to queue larger or more transactions.

- **I2C_STATUS_DEVICE_NOT_PRESENT:**  
  Check wiring and device address. Use retry logic if needed.

- **I2C_STATUS_NEGATIVE_ACKNOWLEDGE:**  
  Device did not acknowledge. Check device readiness and address.

---

## Contributions

Pull requests and suggestions are welcome! Please provide example sketches and documentation for new features.

---

## Contact

For questions, bug reports, or feature requests, please open an issue or contact the maintainer.

---
