#include "proto.h"

int8_t ICACHE_FLASH_ATTR 
PROTO_Init(PROTO_PARSER* parser, PROTO_PARSE_CALLBACK* completeCallback, uint8_t* buf, uint16_t bufSize) {
  parser->buf = buf;
  parser->bufSize = bufSize;
  parser->dataLen = 0;
  parser->callback = completeCallback;
  parser->isEsc = 0;
  return 0;
}

int8_t ICACHE_FLASH_ATTR 
PROTO_ParseByte(PROTO_PARSER* parser, uint8_t value) {
  switch (value) {
    case 0x7D:
      parser->isEsc = 1;
      break;

    case 0x7E:
      parser->dataLen = 0;
      parser->isEsc = 0;
      parser->isBegin = 1;
      break;

    case 0x7F:
      if (parser->callback != NULL)
        parser->callback();
      parser->isBegin = 0;
      return 0;
      break;

    default:
      if (parser->isBegin == 0) break;

      if (parser->isEsc) {
        value ^= 0x20;
        parser->isEsc = 0;
      }

      if (parser->dataLen < parser->bufSize)
        parser->buf[parser->dataLen++] = value;

      break;
  }
  return -1;
}

int16_t ICACHE_FLASH_ATTR 
PROTO_ParseRb(RINGBUF* rb, uint8_t* bufOut, uint16_t* len, uint16_t maxBufLen) {
  uint8_t c;

  PROTO_PARSER proto;
  PROTO_Init(&proto, NULL, bufOut, maxBufLen);
  while (RINGBUF_Get(rb, &c) == 0) {
    if (PROTO_ParseByte(&proto, c) == 0) {
      *len = proto.dataLen;
      return 0;
    }
  }
  return -1;
}

int16_t ICACHE_FLASH_ATTR 
PROTO_AddRb(RINGBUF* rb, const uint8_t* packet, int16_t len) {
  uint16_t i = 2;
  if (RINGBUF_Put(rb, 0x7E) == -1) return -1;
  while (len--) {
    switch (*packet) {
      case 0x7D:
      case 0x7E:
      case 0x7F:
        if (RINGBUF_Put(rb, 0x7D) == -1) return -1;
        if (RINGBUF_Put(rb, *packet++ ^ 0x20) == -1) return -1;
        i += 2;
        break;
      default:
        if (RINGBUF_Put(rb, *packet++) == -1) return -1;
        i++;
        break;
    }
  }
  if (RINGBUF_Put(rb, 0x7F) == -1) return -1;

  return i;
}