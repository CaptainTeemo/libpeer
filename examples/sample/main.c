#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

#include "reader.h"
#include "peer.h"
#include "mbedtls/debug.h"
#include "ffmpeg_reader.h"

int g_interrupted = 0;
PeerConnection *g_pc = NULL;
PeerConnectionState g_state;

static void onconnectionstatechange(PeerConnectionState state, void *data) {
	printf("state is changed: %s\n", peer_connection_state_to_string(state));
	g_state = state;
}

static void onopen(void *user_data) {
	
}

static void onclose(void *user_data) {
	
}

static void onmessasge(char *msg, size_t len, void *user_data) {
	printf("on message: %s", msg);
	if (strncmp(msg, "ping", 4) == 0) {
		printf(", send pong\n");
		peer_connection_datachannel_send(g_pc, "pong", 4);
	}
}

static void signal_handler(int signal) {
	g_interrupted = 1;
}

static uint64_t get_timestamp() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int main(int argc, char *argv[]) {
	uint64_t curr_time, video_time, audio_time;
	uint8_t buf[2048000];
	int size;
	
	pthread_t peer_singaling_thread;
	pthread_t peer_connection_thread;
	
	signal(SIGINT, signal_handler);
	
	psa_crypto_init();
	
	snprintf((char*)buf, sizeof(buf), "test_%d", getpid());

	peer_init();

    struct mg_mgr ws_mgr;
    mg_mgr_init(&ws_mgr);
    mg_log_set(MG_LL_NONE);
    peer_signaling_join_channel((const char*)buf, &ws_mgr);
	
	reader_init();
//    ffmpeg_reader_init();

    while (!g_interrupted) {
        curr_time = get_timestamp();

        // FPS 25
        if (curr_time - video_time > 32.5) {
            video_time = curr_time;
//			if (ffmpeg_reader_get_frame(buf, &size, 0) == 0) {
            if (reader_get_video_frame(buf, &size) == 0) {
                peer_signaling_send_video(buf, size);
            }
        }

        if (curr_time - audio_time > 20) {
//            if (ffmpeg_reader_get_frame(buf, &size, 1) == 0) {
            if (reader_get_audio_frame(buf, &size) == 0) {
                peer_signaling_send_audio(buf, size);
            }
            audio_time = curr_time;
        }

        usleep(1000);
    }
	
//	pthread_join(peer_singaling_thread, NULL);
//	pthread_join(peer_connection_thread, NULL);

    mg_mgr_free(&ws_mgr);
	
	reader_deinit();
	
	peer_signaling_leave_channel();
	peer_connection_destroy(g_pc);
	peer_deinit();
	
	return 0;
}
