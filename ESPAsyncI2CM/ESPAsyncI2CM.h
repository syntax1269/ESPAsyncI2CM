#ifndef ESP_ASYNC_I2C_MASTER_H
#define ESP_ASYNC_I2C_MASTER_H

#include <inttypes.h>

#ifndef MASTER_BUFFER_SIZE
#define MASTER_BUFFER_SIZE 128
#endif
#ifndef I2C_FREQ
#define I2C_FREQ    100000
#endif

#if defined(ESP8266)
#define PIN_SCL 5
#define PIN_SDA 4
#endif

#if defined(ESP32)
#include <Wire.h>
#define ESP32_DEFAULT_SDA 21
#define ESP32_DEFAULT_SCL 22
#endif

enum I2CStatus {
  I2C_STATUS_OK=0,
  I2C_STATUS_OUT_OF_MEMORY=1,
  I2C_STATUS_DEVICE_NOT_PRESENT=2,
  I2C_STATUS_TRANSMIT_ERROR=3,
  I2C_STATUS_NEGATIVE_ACKNOWLEDGE=4,
};

typedef void (*I2CSendCallback)(uint8_t status, void * arg);
typedef void (*I2CRequestCallback)(uint8_t status, void * arg, uint8_t * data, uint8_t datalen);
typedef void (*I2CReadCallback)(uint8_t status, void * arg, uint8_t * data, uint8_t datalen);

struct I2CTransaction;

class ESPAsyncI2CM
{
  private:
#if defined(ESP8266)
    uint8_t   pinSDA = PIN_SDA;
    uint8_t   pinSCL = PIN_SCL;
    uint32_t  cycleToWait = 0;
    uint32_t  savedCycle = 0;
    const void * bitBangPtr = 0;
    const void * scenarioPtr = 0;
    int       dataRead;
    int       dataWrite;
    int       expectedStatus;
#elif defined(ESP32)
    uint8_t   pinSDA = ESP32_DEFAULT_SDA;
    uint8_t   pinSCL = ESP32_DEFAULT_SCL;
    bool      wireInitialized = false;
#endif
    uint8_t   buffer[MASTER_BUFFER_SIZE];
    uint8_t   bufferPtr;
    uint8_t   status;
    uint8_t   dataPtr;
    uint8_t   tries;
    uint8_t   retryCount;

    struct I2CTransaction * allocateTransaction(uint8_t i2cAddress, uint8_t wlen, uint8_t rlen, void * arg);
    void                    startTransaction();

    void                    I2C_init();
    void                    I2C_handle(struct I2CTransaction *);
    void                    I2C_startTransaction(struct I2CTransaction *);
    bool                    I2C_isStopped(struct I2CTransaction *);
    void                    I2C_close(struct I2CTransaction *);
#if defined(ESP8266)
    void                    I2C_playScenario(const void *scenario);
    void                    I2C_sendBitBang(const void *bitBang);
    void                    I2C_bitBangCompleted();
#endif

  public:
#if defined(ESP8266) || defined(ESP32)
    void begin(int sda, int scl) { pinSDA = sda; pinSCL = scl; begin(); }
#endif
    void begin();
    void loop();

    uint8_t send(uint8_t i2cAddress, uint8_t * data, uint8_t dataLen, I2CSendCallback callback, void * arg = 0);
    uint8_t request(uint8_t i2cAddress, uint8_t * data, uint8_t dataLen, uint8_t receiveLen, I2CRequestCallback callback, void * arg = 0);
    uint8_t read(uint8_t i2cAddress, uint8_t receiveLen, I2CReadCallback callback, void * arg = 0);
    uint8_t broadcast(uint8_t * data, uint8_t dataLen, I2CSendCallback callback, void * arg = 0);
    uint8_t setRetryCount(uint8_t count) { retryCount = count; return retryCount; }
};

#endif /* ESP_ASYNC_I2C_MASTER_H */