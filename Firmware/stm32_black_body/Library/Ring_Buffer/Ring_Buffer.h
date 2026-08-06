#pragma once
#include "stm32g4xx.h"
#include <string.h>

// Ring buffer stuff
#define BUFFER_SIZE 20

typedef struct {
    double buffer[BUFFER_SIZE];
    uint8_t bufferHead;
    uint8_t bufferCount;
} ring_buffer;


void ringBufferInit(ring_buffer *ring);
double bufferAddAverage(double data, ring_buffer *ring);