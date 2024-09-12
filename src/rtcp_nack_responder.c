#include "rtcp_nack_responder.h"
#include <assert.h>
#include <string.h>

#define MESSAGE_CONTROL 1

typedef struct Element {
  uint8_t *packet;
  size_t packet_size;
  uint16_t sequence_number;
  struct Element *next;
} Element;

struct Storage {
  Element *oldest;
  Element *newest;
  Element **storage;
  size_t max_size;
  size_t current_size;
};

struct RtcpNackResponder {
  struct Storage *storage;
};

struct RtcpNackResponder *rtcp_nack_responder_create(size_t max_size) {
  struct RtcpNackResponder *responder =
      malloc(sizeof(struct RtcpNackResponder));
  responder->storage = storage_create(max_size);
  return responder;
}

void rtcp_nack_responder_destroy(struct RtcpNackResponder *responder) {
  storage_destroy(responder->storage);
  free(responder);
}

void rtcp_nack_responder_incoming(struct RtcpNackResponder *responder,
                                  Message *messages[], size_t message_count,
                                  message_callback send) {
  for (size_t i = 0; i < message_count; i++) {
    Message *message = messages[i];
    if (message->type != MESSAGE_CONTROL)
      continue;

    size_t p = 0;
    while (p + sizeof(uint32_t) * 3 <= message->size) {
      uint32_t *header = (uint32_t *)(message->data + p);
      uint8_t pt = (header[0] >> 16) & 0xFF;
      uint8_t rc = header[0] & 0x1F;
      uint16_t length = (header[0] >> 16) & 0xFFFF;
      p += (length + 1) * 4;

      if (pt != 205 || rc != 1)
        continue;

      uint16_t pid = ntohs(header[1] >> 16);
      uint16_t blp = ntohs(header[1] & 0xFFFF);

      uint16_t seq_nums[17];
      size_t seq_count = 0;
      seq_nums[seq_count++] = pid;

      for (int j = 0; j < 16; j++) {
        if (blp & (1 << j)) {
          seq_nums[seq_count++] = pid + j + 1;
        }
      }

      for (size_t j = 0; j < seq_count; j++) {
        size_t packet_size;
        uint8_t *packet =
            storage_get(responder->storage, seq_nums[j], &packet_size);
        if (packet) {
          Message resend_message = {MESSAGE_CONTROL, packet, packet_size};
          send(&resend_message);
        }
      }
    }
  }
}

void rtcp_nack_responder_outgoing(struct RtcpNackResponder *responder,
                                  Message *messages[], size_t message_count) {
  for (size_t i = 0; i < message_count; i++) {
    Message *message = messages[i];
    if (message->type != MESSAGE_CONTROL) {
      storage_store(responder->storage, message->data, message->size);
    }
  }
}

struct Storage *storage_create(size_t max_size) {
  struct Storage *storage = malloc(sizeof(struct Storage));
  storage->oldest = NULL;
  storage->newest = NULL;
  storage->storage = calloc(max_size, sizeof(Element *));
  storage->max_size = max_size;
  storage->current_size = 0;
  return storage;
}

void storage_destroy(struct Storage *storage) {
  Element *current = storage->oldest;
  while (current) {
    Element *next = current->next;
    free(current->packet);
    free(current);
    current = next;
  }
  free(storage->storage);
  free(storage);
}

uint8_t *storage_get(struct Storage *storage, uint16_t sequence_number,
                     size_t *size) {
  Element *element = storage->storage[sequence_number % storage->max_size];
  if (element && element->sequence_number == sequence_number) {
    *size = element->packet_size;
    return element->packet;
  }
  return NULL;
}

void storage_store(struct Storage *storage, const uint8_t *packet,
                   size_t packet_size) {
  if (!packet || packet_size < 12)
    return; // 12 is the size of RTP header

  uint16_t sequence_number = ntohs(*(uint16_t *)(packet + 2));

  Element *new_element = malloc(sizeof(Element));
  new_element->packet = malloc(packet_size);
  memcpy(new_element->packet, packet, packet_size);
  new_element->packet_size = packet_size;
  new_element->sequence_number = sequence_number;
  new_element->next = NULL;

  if (storage->current_size == 0) {
    storage->oldest = storage->newest = new_element;
  } else {
    storage->newest->next = new_element;
    storage->newest = new_element;
  }

  size_t index = sequence_number % storage->max_size;
  if (storage->storage[index]) {
    free(storage->storage[index]->packet);
    free(storage->storage[index]);
  }
  storage->storage[index] = new_element;

  if (storage->current_size < storage->max_size) {
    storage->current_size++;
  } else {
    Element *old = storage->oldest;
    storage->oldest = old->next;
    storage->storage[old->sequence_number % storage->max_size] = NULL;
    free(old->packet);
    free(old);
  }
}