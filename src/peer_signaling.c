#include <string.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <cJSON.h>
#include <pthread.h>

//#include <core_mqtt.h>
//#include <core_http_client.h>

#include "config.h"
#include "base64.h"
#include "utils.h"
//#include "ssl_transport.h"
#include "peer_signaling.h"
#include "utils/darray.h"

#define KEEP_ALIVE_TIMEOUT_SECONDS 60
#define CONNACK_RECV_TIMEOUT_MS 1000

#define JRPC_PEER_STATE "state"
#define JRPC_PEER_OFFER "offer"
#define JRPC_PEER_ANSWER "answer"
#define JRPC_PEER_CLOSE "close"

#define BUF_SIZE 4096
#define TOPIC_SIZE 128

static const char *ca_cert = \
{
	\
        "-----BEGIN CERTIFICATE-----\r\n"
        "MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\r\n" \
        "A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\r\n" \
        "b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\r\n" \
        "MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\r\n" \
        "YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\r\n" \
        "aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\r\n" \
        "jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\r\n" \
        "xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\r\n" \
        "1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\r\n" \
        "snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\r\n" \
        "U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\r\n" \
        "9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\r\n" \
        "BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\r\n" \
        "AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\r\n" \
        "yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\r\n" \
        "38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\r\n" \
        "AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\r\n" \
        "DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\r\n" \
        "HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\r\n" \
        "-----END CERTIFICATE-----"
};

static PeerConfiguration peerConnectionConfig = {
        .ice_servers = {
                { .urls = "stun:stun.l.google.com:19302" },
        },
        .datachannel = DATA_CHANNEL_NONE,
        .video_codec = CODEC_H264,
        .audio_codec = CODEC_PCMA
};

typedef struct PeerSignaling {
	char subtopic[TOPIC_SIZE];
	char pubtopic[TOPIC_SIZE];
	
	uint16_t packet_id;
	int id;
    char *client_id;
    struct mg_connection *mg_connection;
    struct mg_mgr *mg_manager;

    pthread_mutex_t peer_mutex;
} PeerSignaling;

typedef struct PeerSignalChannel {
    char *user_id;
    struct mg_connection *mg_connection;
    PeerConnection *pc;
	int peer_closed;
	int peer_connected;
	pthread_t peer_thread;
} PeerSignalChannel;

typedef struct HTTPRequest {
    char *url;
    char *post_body;
} HTTPRequest;

static PeerSignaling g_ps;
static int g_interrupted = 0;

DARRAY(struct PeerSignalChannel *)g_channels;

static int peer_signaling_loop() {
    if (g_ps.mg_manager) {
        mg_mgr_poll(g_ps.mg_manager, 1000);
    }
    return 0;
}

static void* peer_signaling_task(void *data) {
    while (!g_interrupted) {
        peer_signaling_loop();
        usleep(1000);
    }
    return NULL;
}

static void* peer_connection_task(void *data) {
	struct PeerSignalChannel *channel = (struct PeerSignalChannel *)data;
    while (channel->pc && !channel->peer_closed) {
        peer_connection_loop(channel->pc);
        usleep(1000);
    }
    return NULL;
}

static void peer_signaling_ws_send_message(struct mg_connection *connection, cJSON *json) {
    char *payload = cJSON_PrintUnformatted(json);
    if (payload) {
        mg_ws_send(connection, payload, strlen(payload), WEBSOCKET_OP_TEXT);
        free(payload);
    }
}

static cJSON* peer_signaling_ws_create_response(const char *user_id) {
	cJSON *json = cJSON_CreateObject();
	cJSON_AddStringToObject(json, "channelName", "xsh");
	cJSON_AddStringToObject(json, "userId", user_id);
	return json;
}

static void peer_signaling_ws_process_response(struct mg_connection *connection, const char *type, cJSON *body) {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json, "body", body);
    cJSON_AddStringToObject(json, "type", type);
    peer_signaling_ws_send_message(connection, json);
}

static void peer_signaling_onicecandidate(char *description, void *userdata) {
//    LOGD("offer: %s", description);

//    const char *url = "http://127.0.0.1/whip";
//    HTTPRequest request;
//    request.url = (char *)url;
//    request.post_body = description;
//    g_ps.mg_connection = mg_http_connect(g_ps.mg_manager, url, peer_signaling_http_callback, &request);

    cJSON *json = peer_signaling_ws_create_response(g_ps.client_id);
    cJSON *sdp = cJSON_CreateObject();
    cJSON_AddStringToObject(sdp, "type", "offer");
    cJSON_AddStringToObject(sdp, "sdp", description);
    cJSON_AddItemToObject(json, "sdp", sdp);
    peer_signaling_ws_process_response(g_ps.mg_connection, "send_offer", json);
    g_ps.id = 0;
}

static void peer_signaling_create_channel(struct mg_connection *c, const char *user_id) {
	PeerConnection *peerConnection = peer_connection_create(&peerConnectionConfig);
	peer_connection_onicecandidate(peerConnection, peer_signaling_onicecandidate);
	
	struct PeerSignalChannel *channel = (struct PeerSignalChannel *)calloc(1, sizeof(struct PeerSignalChannel));
	channel->mg_connection = c;
	channel->pc = peerConnection;
	channel->peer_closed = 0;
	channel->peer_connected = 0;

	pthread_t peer_connection_thread;
	pthread_create(&peer_connection_thread, NULL, peer_connection_task, channel);
	
	channel->peer_thread = peer_connection_thread;
	channel->user_id = strdup(user_id);
	
	da_push_back(g_channels, &channel);
}

static void peer_signaling_ws_process_request(struct mg_connection *c, const char *msg, size_t size) {
	cJSON *request = NULL;
	cJSON *type, *body;
	
	request = cJSON_ParseWithLength(msg, size);
	
	if (!request) {
		LOGW("Parse json failed");
		return;
	}
	
	do {
		type = cJSON_GetObjectItem(request, "type");
        if (!type || !cJSON_IsString(type)) {
            LOGW("Cannot find type");
            break;
        }

		body = cJSON_GetObjectItem(request, "body");

		if (strcmp(type->valuestring, "offer_sdp_received") == 0) {

		} else if (strcmp(type->valuestring, "answer_sdp_received") == 0) {
            const char *offer = cJSON_GetObjectItem(body, "sdp")->valuestring;
			for (int i = 0; i < g_channels.num; i++) {
				struct PeerSignalChannel *channel = g_channels.array[i];
				if (strcmp(g_ps.client_id, channel->user_id) != 0 && !channel->peer_connected) {
					peer_connection_set_remote_description(channel->pc, offer);
					channel->peer_connected = 1;
				}
			}
		} else if (strcmp(type->valuestring, "joined") == 0) {
            cJSON *user_id = cJSON_GetObjectItem(body, "userId");
			PeerConnection *peerConnection = NULL;
			for (int i = 0; i < g_channels.num; i++) {
				struct PeerSignalChannel *channel = g_channels.array[i];
				if (user_id && strcmp(user_id->valuestring, channel->user_id) == 0) {
					peerConnection = channel->pc;
					break;
				}
			}
            if (strcmp(g_ps.client_id, user_id->valuestring) != 0) {
                peer_connection_create_offer(peerConnection);
            }
        } else if (strcmp(type->valuestring, "quit") == 0) {
			for (int i = 0; i < g_channels.num; i++) {
				struct PeerSignalChannel *channel = g_channels.array[i];
				peer_connection_close(channel->pc);
			}
		} else {

		}
	} while (0);
	
	cJSON_Delete(request);
}

static void peer_signaling_ws_join_channel() {
    cJSON *json = peer_signaling_ws_create_response(g_ps.client_id);
    peer_signaling_ws_process_response(g_ps.mg_connection, "join", json);
}

static const uint64_t s_timeout_ms = 1500;
static void peer_signaling_http_callback(struct mg_connection *c, int ev, void *ev_data) {
//    HTTPRequest *request = (HTTPRequest *)c->fn_data;
//    struct mg_str ca_data;
    if (ev == MG_EV_OPEN) {
        // Connection created. Store connect expiration time in c->data
        *(uint64_t *) c->data = mg_millis() + s_timeout_ms;
    } else if (ev == MG_EV_POLL) {
        if (mg_millis() > *(uint64_t *) c->data &&
            (c->is_connecting || c->is_resolving)) {
            mg_error(c, "Connect timeout");
        }
    } else if (ev == MG_EV_CONNECT) {
        // Connected to server. Extract host name from URL
//        struct mg_str host = mg_url_host(request->url);
//        if (mg_url_is_ssl(request->url)) {
//            psa_crypto_init();
//
//            ca_data = mg_file_read(&mg_fs_posix, "/Users/naver/Documents/GitHub/libpeer/src/certs/ca.pem");
//            struct mg_tls_opts opts = {
//                    .ca = ca_data,
//                    .name = host
//            };
//            mg_tls_init(c, &opts);
//        }

        // Send request
//        size_t content_length = request->post_body ? strlen(request->post_body) : 0;
//        mg_printf(c,
//                  "%s %s HTTP/1.1\r\n"
//                  "Host: %.*s\r\n"
//                  "Content-Type: application/sdp\r\n"
//                  "Content-Length: %d\r\n"
//                  "\r\n",
//                  "POST", mg_url_uri(request->url), (int) host.len,
//                  host.buf, content_length);
//        mg_send(c, request->post_body, content_length);
    } else if (ev == MG_EV_HTTP_MSG) {
        // Response is received. Print it
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
//        printf("%.*s", (int) hm->message.len, hm->message.buf);
        if (mg_match(hm->uri, mg_str("/websocket"), NULL)) {
            // Upgrade to websocket. From now on, a connection is a full-duplex
            // Websocket connection, which will receive MG_EV_WS_MSG events.
            mg_ws_upgrade(c, hm, NULL);
        }
    } else if (ev == MG_EV_WS_MSG) {
		pthread_mutex_lock(&g_ps.peer_mutex);
        // Got websocket frame. Received data is wm->data. Echo it back!
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
        cJSON *request = cJSON_Parse(wm->data.buf);
        if (!request) {
            LOGW("Parse json failed");
			pthread_mutex_unlock(&g_ps.peer_mutex);
            return;
        }

        do {
            cJSON *type = cJSON_GetObjectItem(request, "type");
            if (!type || !cJSON_IsString(type)) {
                LOGW("Cannot find type");
                break;
            }

            cJSON *body = cJSON_GetObjectItem(request, "body");
            cJSON *json_sdp = cJSON_GetObjectItem(body, "sdp");
            cJSON *sdp = cJSON_Duplicate(json_sdp, cJSON_True);

            cJSON *user_id = cJSON_GetObjectItem(body, "userId");
            char *user_id_s = strdup(user_id->valuestring);

            if (strcmp(type->valuestring, "send_offer") == 0) {
                for (int i = 0; i < g_channels.num; i++) {
                    struct PeerSignalChannel *channel = g_channels.array[i];
                    if (strcmp(channel->user_id, user_id_s) != 0 && !channel->peer_connected) {
                        peer_signaling_ws_process_response(channel->mg_connection, "offer_sdp_received", sdp);
                    }
                }
            } else if (strcmp(type->valuestring, "send_answer") == 0) {
                for (int i = 0; i < g_channels.num; i++) {
                    struct PeerSignalChannel *channel = g_channels.array[i];
                    if (strcmp(channel->user_id, user_id_s) != 0 && !channel->peer_connected) {
                        peer_signaling_ws_process_response(channel->mg_connection, "answer_sdp_received", sdp);
                    }
                }
            } else if (strcmp(type->valuestring, "join") == 0) {
				peer_signaling_create_channel(c, user_id_s);
                for (struct mg_connection *wc = c->mgr->conns; wc != NULL; wc = wc->next) {
                    cJSON *response = peer_signaling_ws_create_response(user_id_s);
                    peer_signaling_ws_process_response(wc, "joined", response);
                }
            } else if (strcmp(type->valuestring, "quit") == 0) {
                for (int i = 0; i < g_channels.num; i++) {
                    struct PeerSignalChannel *channel = g_channels.array[i];
                    if (!strcmp(channel->user_id, user_id_s)) {
                        da_erase(g_channels, i);
                        break;
                    }
                }
            } else {

            }

            free(user_id_s);
        } while (0);

        cJSON_Delete(request);
		pthread_mutex_unlock(&g_ps.peer_mutex);
    } else if (ev == MG_EV_CLOSE) {
        LOGI("closed %p\r\n", &c);
		pthread_mutex_lock(&g_ps.peer_mutex);
        for (int i = 0; i < g_channels.num; i++) {
            struct PeerSignalChannel *channel = g_channels.array[i];
            if (channel->mg_connection == c) {
				channel->peer_closed = 1;
				channel->peer_connected = 0;
				if (channel->peer_thread) {
					pthread_join(channel->peer_thread, NULL);
				}
                peer_connection_close(channel->pc);
                peer_connection_destroy(channel->pc);
                free(channel->user_id);

                channel->mg_connection = NULL;

                da_erase(g_channels, i);
				
				free(channel);
                break;
            }
        }
		pthread_mutex_unlock(&g_ps.peer_mutex);
    } else if (ev == MG_EV_ERROR) {
        LOGW("error \r\n");
    }
}

static void peer_signaling_ws_callback(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_OPEN) {
//        c->is_hexdumping = 1;
    } else if (ev == MG_EV_ERROR) {
        // On error, log error message
        MG_ERROR(("%p %s", c->fd, (char *) ev_data));
    } else if (ev == MG_EV_WS_OPEN) {
        peer_signaling_ws_join_channel();
    } else if (ev == MG_EV_WS_MSG) {
        // When we get echo response, print it
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
        LOGI("GOT ECHO REPLY: [%.*s]\n", (int) wm->data.len, wm->data.buf);
        pthread_mutex_lock(&g_ps.peer_mutex);
        peer_signaling_ws_process_request(c, wm->data.buf, wm->data.len);
        pthread_mutex_unlock(&g_ps.peer_mutex);
    }
}

int peer_signaling_join_channel(const char *client_id, struct mg_mgr *connection_mgr) {
	da_init(g_channels);
	
    g_ps.mg_manager = connection_mgr;

    g_ps.client_id = strdup(client_id);

//    peer_connection_create_offer(pc);
    const char *ws_url = "ws://0.0.0.0:9527/websocket";
    mg_http_listen(connection_mgr, ws_url, peer_signaling_http_callback, NULL);

    g_ps.mg_connection = mg_ws_connect(connection_mgr, "ws://127.0.0.1:9527/websocket", peer_signaling_ws_callback, NULL, NULL);

    pthread_mutex_init(&g_ps.peer_mutex, PTHREAD_MUTEX_DEFAULT);

    pthread_t peer_signaling_thread;
    pthread_create(&peer_signaling_thread, NULL, peer_signaling_task, NULL);

	return 0;
}

void peer_signaling_leave_channel() {

}

void peer_signaling_send_video(uint8_t *buf, size_t len) {
    pthread_mutex_lock(&g_ps.peer_mutex);
    for (int i = 0; i < g_channels.num; i++) {
        struct PeerSignalChannel *channel = g_channels.array[i];
        peer_connection_send_video(channel->pc, buf, len);
    }
    pthread_mutex_unlock(&g_ps.peer_mutex);
}

void peer_signaling_send_audio(uint8_t *buf, size_t len) {
    pthread_mutex_lock(&g_ps.peer_mutex);
    for (int i = 0; i < g_channels.num; i++) {
        struct PeerSignalChannel *channel = g_channels.array[i];
		peer_connection_send_audio(channel->pc, buf, len);
    }
    pthread_mutex_unlock(&g_ps.peer_mutex);
}

