#ifndef PEER_SIGNALING_H_
#define PEER_SIGNALING_H_

#include "peer_connection.h"
#include "mongoose.h"

#ifdef __cplusplus
extern "C" {
#endif

int peer_signaling_join_channel(const char *client_id, struct mg_mgr *ws_mgr);

void peer_signaling_leave_channel();

void peer_signaling_send_video(uint8_t *buf, size_t len);
void peer_signaling_send_audio(uint8_t *buf, size_t len);

#ifdef __cplusplus
} 
#endif

#endif //PEER_SIGNALING_H_

