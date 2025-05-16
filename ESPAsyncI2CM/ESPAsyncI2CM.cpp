#include "ESPAsyncI2CM.h"
#include "Arduino.h"

#if defined(ESP8266)
#include "interrupts.h"
#define DISABLED_IRQ_SECTION_START { esp8266::InterruptLock irqLock;
#define DISABLED_IRQ_SECTION_LEAVE }
#elif defined(ESP32)
#define DISABLED_IRQ_SECTION_START {
#define DISABLED_IRQ_SECTION_LEAVE }
#else
#include <avr/io.h>
#include <avr/interrupt.h>
#define DISABLED_IRQ_SECTION_START { uint8_t sreg = SREG; cli();
#define DISABLED_IRQ_SECTION_LEAVE SREG = sreg; }
#endif

enum
{
  OPERATION_READ,
  OPERATION_REQUEST,
  OPERATION_SEND,
  OPERATION_REQUEST_READ,
  OPERATION_PENDING,
};

enum
{
  I2C_STATUS_PENDING=253,
  I2C_STATUS_CLOSING=254,
  I2C_STATUS_FREE=255,
};

struct I2CTransaction
{
  uint8_t operation;
  uint8_t readLen;
  uint8_t writeLen;
  uint8_t i2cAddress;
  union {
    I2CSendCallback    sendCb;
    I2CRequestCallback requestCb;
    I2CReadCallback    readCb;
  } cb;
  void * arg;
  uint8_t data[0];
};

void ESPAsyncI2CM::begin()
{
  bufferPtr = 0;
  status = I2C_STATUS_FREE;
  dataPtr = 0;
  tries = 0;
  retryCount = 0;
  I2C_init();
}

struct I2CTransaction * ESPAsyncI2CM::allocateTransaction(uint8_t i2cAddress, uint8_t wlen, uint8_t rlen, void * arg)
{
  uint8_t len = ((wlen > rlen) ? wlen : rlen) + sizeof(struct I2CTransaction);
  if( len & 3 ) { len |= 3; len++; }
  if( len + bufferPtr > MASTER_BUFFER_SIZE ) return 0;
  struct I2CTransaction * t = (struct I2CTransaction *)(buffer + bufferPtr);
  t->operation = OPERATION_PENDING;
  t->readLen = rlen;
  t->writeLen = wlen;
  t->i2cAddress = i2cAddress;
  t->arg = arg;
  bufferPtr += len;
  return t;
}

void ESPAsyncI2CM::startTransaction()
{
  if( (bufferPtr != 0) && (status == I2C_STATUS_FREE) )
  {
    status = I2C_STATUS_PENDING;
    tries = 0;
    I2C_startTransaction( (struct I2CTransaction *)buffer );
  }
}

uint8_t ESPAsyncI2CM::send(uint8_t i2cAddress, uint8_t * data, uint8_t dataLen, I2CSendCallback callback, void * arg)
{
  uint8_t ret;
  DISABLED_IRQ_SECTION_START;
  struct I2CTransaction * t = allocateTransaction(i2cAddress, dataLen, 0, arg);
  if( t != 0 )
  {
    t->cb.sendCb = callback;
    memcpy(t->data, data, dataLen);
    t->operation = OPERATION_SEND;
    startTransaction();
    ret = I2C_STATUS_OK;
  }
  else
    ret = I2C_STATUS_OUT_OF_MEMORY;
  DISABLED_IRQ_SECTION_LEAVE;
  return ret;
}

uint8_t ESPAsyncI2CM::broadcast(uint8_t * data, uint8_t dataLen, I2CSendCallback callback, void * arg)
{
  return send(0, data, dataLen, callback, arg);
}

uint8_t ESPAsyncI2CM::request(uint8_t i2cAddress, uint8_t * data, uint8_t dataLen, uint8_t receiveLen, I2CRequestCallback callback, void * arg)
{
  uint8_t ret;
  DISABLED_IRQ_SECTION_START;
  struct I2CTransaction * t = allocateTransaction(i2cAddress, dataLen, receiveLen, arg );
  if( t == 0 )
    ret = I2C_STATUS_OUT_OF_MEMORY;
  else
  {
    t->cb.requestCb = callback;
    memcpy(t->data, data, dataLen);
    t->operation = OPERATION_REQUEST;
    startTransaction();
    ret = I2C_STATUS_OK;
  }
  DISABLED_IRQ_SECTION_LEAVE;
  return ret;
}

uint8_t ESPAsyncI2CM::read(uint8_t i2cAddress, uint8_t receiveLen, I2CReadCallback callback, void * arg)
{
  uint8_t ret;
  DISABLED_IRQ_SECTION_START;
  struct I2CTransaction * t = allocateTransaction(i2cAddress, 0, receiveLen, arg);
  if( t == 0 )
    ret = I2C_STATUS_OUT_OF_MEMORY;
  else
  {
    t->cb.readCb = callback;
    t->operation = OPERATION_READ;
    startTransaction();
    ret = I2C_STATUS_OK;
  }
  DISABLED_IRQ_SECTION_LEAVE;
  return ret;
}

void ESPAsyncI2CM::loop()
{
  if( bufferPtr == 0 ) return;
  struct I2CTransaction * t = (struct I2CTransaction *)buffer;
  I2C_handle(t);
  switch(status)
  {
    case I2C_STATUS_OK:
      {
        if( t->cb.sendCb != 0 )
        {
          switch(t->operation)
          {
            case OPERATION_SEND:
              t->cb.sendCb(status, t->arg);
              break;
            case OPERATION_READ:
              t->cb.readCb(status, t->arg, t->data, t->readLen);
              break;
            case OPERATION_REQUEST:
            case OPERATION_REQUEST_READ:
              t->cb.requestCb(status, t->arg, t->data, t->readLen);
              break;
          }
        }
        status = I2C_STATUS_CLOSING;
      }
      break;
    case I2C_STATUS_DEVICE_NOT_PRESENT:
      if( ++tries <= retryCount )
      {
        status = I2C_STATUS_PENDING;
        I2C_startTransaction(t);
        break;
      }
    case I2C_STATUS_TRANSMIT_ERROR:
    case I2C_STATUS_NEGATIVE_ACKNOWLEDGE:
      {
        if( t->cb.sendCb != 0 )
        {
          switch(t->operation)
          {
            case OPERATION_SEND:
              t->cb.sendCb(status, t->arg);
              break;
            case OPERATION_READ:
              t->cb.readCb(status, t->arg, 0, 0);
              break;
            case OPERATION_REQUEST:
            case OPERATION_REQUEST_READ:
              t->cb.requestCb(status, t->arg, 0, 0);
              break;
          }
        }
        status = I2C_STATUS_CLOSING;
      }
      break;
    case I2C_STATUS_CLOSING:
      {
        if( I2C_isStopped(t) )
        {
          status = I2C_STATUS_FREE;
          I2C_close(t);
          uint8_t len = ((t->writeLen > t->readLen) ? t->writeLen : t->readLen) + sizeof(struct I2CTransaction);
          if( len & 3 ) { len |= 3; len++; }
          DISABLED_IRQ_SECTION_START;
          if( bufferPtr > len ) {
            memmove(buffer, buffer+len, bufferPtr - len);
            bufferPtr -= len;
          } else
            bufferPtr = 0;
          DISABLED_IRQ_SECTION_LEAVE;
          startTransaction();
        }
      }
      break;
  }
}

#if defined(ESP8266)
// ... existing ESP8266 implementation (bit-bang) ...
// ... existing code ...
#elif defined(ESP32)

void ESPAsyncI2CM::I2C_init()
{
  if (!wireInitialized) {
    Wire.begin(pinSDA, pinSCL, I2C_FREQ);
    wireInitialized = true;
  }
}

void ESPAsyncI2CM::I2C_handle(struct I2CTransaction *t)
{
  switch (t->operation) {
    case OPERATION_SEND:
      Wire.beginTransmission(t->i2cAddress);
      Wire.write(t->data, t->writeLen);
      t->cb.sendCb(Wire.endTransmission() == 0 ? I2C_STATUS_OK : I2C_STATUS_TRANSMIT_ERROR, t->arg);
      status = I2C_STATUS_OK;
      break;
    case OPERATION_READ:
      Wire.requestFrom(t->i2cAddress, t->readLen);
      for (uint8_t i = 0; i < t->readLen && Wire.available(); ++i) {
        t->data[i] = Wire.read();
      }
      t->cb.readCb(I2C_STATUS_OK, t->arg, t->data, t->readLen);
      status = I2C_STATUS_OK;
      break;
    case OPERATION_REQUEST:
      Wire.beginTransmission(t->i2cAddress);
      Wire.write(t->data, t->writeLen);
      if (Wire.endTransmission(false) == 0) {
        Wire.requestFrom(t->i2cAddress, t->readLen);
        for (uint8_t i = 0; i < t->readLen && Wire.available(); ++i) {
          t->data[i] = Wire.read();
        }
        t->cb.requestCb(I2C_STATUS_OK, t->arg, t->data, t->readLen);
        status = I2C_STATUS_OK;
      } else {
        t->cb.requestCb(I2C_STATUS_TRANSMIT_ERROR, t->arg, nullptr, 0);
        status = I2C_STATUS_TRANSMIT_ERROR;
      }
      break;
    default:
      break;
  }
}

void ESPAsyncI2CM::I2C_startTransaction(struct I2CTransaction *t)
{
  I2C_handle(t);
}

bool ESPAsyncI2CM::I2C_isStopped(struct I2CTransaction *)
{
  return true;
}

void ESPAsyncI2CM::I2C_close(struct I2CTransaction *)
{
  // No special cleanup needed for ESP32 Wire
}

#else

#define TWI_TWBR  ((F_CPU / I2C_FREQ) - 16) / 2

void ESPAsyncI2CM::I2C_init()
{
  TWSR = 0;
  TWBR = TWI_TWBR;
  TWDR = 0xFF;
  TWCR = (0<<TWEN)|(0<<TWIE)|(0<<TWINT)|(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|(0<<TWWC);
  pinMode(SDA, INPUT_PULLUP);
  pinMode(SCL, INPUT_PULLUP);
}

void ESPAsyncI2CM::I2C_startTransaction(struct I2CTransaction *)
{
  TWCR = (1<<TWEN)|(0<<TWIE)|(1<<TWINT)|(0<<TWEA)|(1<<TWSTA)|(0<<TWSTO)|(0<<TWWC);
}

bool ESPAsyncI2CM::I2C_isStopped(struct I2CTransaction *)
{
  return !( TWCR & _BV(TWSTO) );
}

void ESPAsyncI2CM::I2C_close(struct I2CTransaction *)
{
  // No special closing logic needed
}

#endif