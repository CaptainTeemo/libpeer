#ifndef RTC_RTCP_NACK_RESPONDER_H
#define RTC_RTCP_NACK_RESPONDER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define DEFAULT_MAX_SIZE 512

// Forward declarations
struct RtcpNackResponder;
struct Storage;

// Simplified message structure
typedef struct {
  int type;
  uint8_t *data;
  size_t size;
} Message;

// Function pointer type for the message callback
typedef void (*message_callback)(const Message *message);

// Create a new RtcpNackResponder
struct RtcpNackResponder *rtcp_nack_responder_create(size_t max_size);

// Destroy the RtcpNackResponder
void rtcp_nack_responder_destroy(struct RtcpNackResponder *responder);

// Handle incoming messages
void rtcp_nack_responder_incoming(struct RtcpNackResponder *responder,
                                  Message *messages[], size_t message_count,
                                  message_callback send);

// Handle outgoing messages
void rtcp_nack_responder_outgoing(struct RtcpNackResponder *responder,
                                  Message *messages[], size_t message_count);

// Storage functions
struct Storage *storage_create(size_t max_size);
void storage_destroy(struct Storage *storage);
uint8_t *storage_get(struct Storage *storage, uint16_t sequence_number,
                     size_t *size);
void storage_store(struct Storage *storage, const uint8_t *packet,
                   size_t packet_size);

#endif // RTC_RTCP_NACK_RESPONDER_H