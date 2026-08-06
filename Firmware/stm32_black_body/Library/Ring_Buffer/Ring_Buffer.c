#include "Ring_Buffer.h"

// ─── Ring Buffer Implementation ────────────────────────────────────────────────────────

// Add new value to buffer
void bufferAppend(double data, ring_buffer *ring) {
  ring->buffer[ring->bufferHead] = data;
  ring->bufferHead = (ring->bufferHead + 1) % BUFFER_SIZE;
  if (ring->bufferCount < BUFFER_SIZE) {
    ring->bufferCount++;
  }
}

// Read specific values from buffer
double bufferRead(int position, ring_buffer *ring) {
  if (position < 0 || position >= ring->bufferCount) {
    return 0.0; // or handle error as needed
  }
  int index = (ring->bufferHead - 1 - position + BUFFER_SIZE) % BUFFER_SIZE;
  return ring->buffer[index];
}

//Get average of all real data in buffer
double bufferAddAverage(double data, ring_buffer *ring) {
  bufferAppend(data, ring);
  if (ring->bufferCount == 0) {
    return 0.0;
  }
  double sum = 0.0;
  for (int i = 0; i < ring->bufferCount; i++) {
    sum += bufferRead(i, ring);
  }
  return sum / ring->bufferCount;
}

void ringBufferInit(ring_buffer *ring){
    memset(ring->buffer, 0, sizeof(ring->buffer));
    ring->bufferCount = 0;
    ring->bufferHead = 0;
}