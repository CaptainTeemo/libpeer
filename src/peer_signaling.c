#include <string.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <cJSON.h>

//#include <core_mqtt.h>
//#include <core_http_client.h>

#include "config.h"
#include "base64.h"
#include "utils.h"
//#include "ssl_transport.h"
#include "peer_signaling.h"

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

typedef struct PeerSignaling {
	char subtopic[TOPIC_SIZE];
	char pubtopic[TOPIC_SIZE];
	
	uint16_t packet_id;
	int id;
    char *client_id;
	PeerConnection *pc;
    struct mg_connection *mg_connection;
    struct mg_mgr *mg_manager;
} PeerSignaling;

typedef struct HTTPRequest {
    char *url;
    char *post_body;
} HTTPRequest;

static PeerSignaling g_ps;

static void peer_signaling_send_ws_message(cJSON *json) {
    char *payload = cJSON_PrintUnformatted(json);
    if (payload) {
        mg_ws_send(g_ps.mg_connection, payload, strlen(payload), WEBSOCKET_OP_TEXT);
        free(payload);
    }
    cJSON_Delete(json);
}

static cJSON* peer_signaling_create_response() {
	cJSON *json = cJSON_CreateObject();
	cJSON_AddStringToObject(json, "channelName", "xsh");
	cJSON_AddStringToObject(json, "userId", g_ps.client_id);
	return json;
}

static void peer_signaling_process_response(const char *type, cJSON *body) {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json, "body", body);
    cJSON_AddStringToObject(json, "type", type);
    peer_signaling_send_ws_message(json);
}

static void peer_signaling_process_request(const char *msg, size_t size) {
	cJSON *request = NULL;
	cJSON *type, *body;
	
	request = cJSON_Parse(msg);
	
	if (!request) {
		LOGW("Parse json failed");
		return;
	}
	
	do {
		type = cJSON_GetObjectItem(request, "type");
		body = cJSON_GetObjectItem(request, "body");

		if (!type || !cJSON_IsString(type)) {
			LOGW("Cannot find type");
			break;
		}
		
//		PeerConnectionState state = peer_connection_get_state(g_ps.pc);

		if (strcmp(type->valuestring, "offer_sdp_received") == 0) {

		} else if (strcmp(type->valuestring, "answer_sdp_received") == 0) {
            const char *offer = cJSON_GetObjectItem(body, "sdp")->valuestring;
            peer_connection_set_remote_description(g_ps.pc, offer);
		} else if (strcmp(type->valuestring, "joined") == 0) {
            peer_connection_create_offer(g_ps.pc);
        } else if (strcmp(type->valuestring, "quit") == 0) {
			peer_connection_close(g_ps.pc);
		} else {

		}
		
	} while (0);
	
	cJSON_Delete(request);
}

static void peer_signaling_ws_join_channel() {
    cJSON *json = peer_signaling_create_response(g_ps.client_id);
    peer_signaling_process_response("join", json);
}

static const uint64_t s_timeout_ms = 1500;
static void peer_signaling_http_callback(struct mg_connection *c, int ev, void *ev_data) {
    HTTPRequest *request = (HTTPRequest *)c->fn_data;
    struct mg_str ca_data;
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
        struct mg_str host = mg_url_host(request->url);

        if (mg_url_is_ssl(request->url)) {
            psa_crypto_init();

            ca_data = mg_file_read(&mg_fs_posix, "/Users/naver/Documents/GitHub/libpeer/src/certs/ca.pem");
            struct mg_tls_opts opts = {
                    .ca = ca_data,
                    .name = host
            };
            mg_tls_init(c, &opts);
        }

        // Send request
        size_t content_length = request->post_body ? strlen(request->post_body) : 0;
        mg_printf(c,
                  "%s %s HTTP/1.1\r\n"
                  "Host: %.*s\r\n"
                  "Content-Type: application/json\r\n"
                  "Content-Length: %d\r\n"
                  "\r\n",
                  "POST", mg_url_uri(request->url), (int) host.len,
                  host.buf, content_length);
        mg_send(c, request->post_body, content_length);
    } else if (ev == MG_EV_HTTP_MSG) {
        // Response is received. Print it
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        printf("%.*s", (int) hm->message.len, hm->message.buf);
        cJSON *root = cJSON_Parse(hm->body.buf);
        if (root) {
            cJSON *sdp = cJSON_GetObjectItem(root, "sdp");
            peer_connection_set_remote_description(g_ps.pc, sdp->valuestring);
            cJSON_Delete(root);
        }
        c->is_draining = 1;        // Tell mongoose to close this connection
    } else if (ev == MG_EV_ERROR) {
        printf("error \r\n");
    }
}

static void peer_signaling_onicecandidate(char *description, void *userdata) {
	LOGD("offer: %s", description);

//    const char *url = "http://127.0.0.1/index/api/webrtc?app=live&stream=test&type=push";
//    HTTPRequest request;
//    request.url = (char *)url;
//    request.post_body = description;
//    g_ps.mg_connection = mg_http_connect(g_ps.mg_manager, url, peer_signaling_http_callback, &request);

	cJSON *json = peer_signaling_create_response();
    cJSON *sdp = cJSON_CreateObject();
    cJSON_AddStringToObject(sdp, "type", "offer");
    cJSON_AddStringToObject(sdp, "sdp", description);
	cJSON_AddItemToObject(json, "sdp", sdp);
	peer_signaling_process_response("send_offer", json);
	g_ps.id = 0;
}

static void peer_signaling_ws_callback(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_OPEN) {
        c->is_hexdumping = 1;
    } else if (ev == MG_EV_ERROR) {
        // On error, log error message
        MG_ERROR(("%p %s", c->fd, (char *) ev_data));
    } else if (ev == MG_EV_WS_OPEN) {
        peer_signaling_ws_join_channel();
    } else if (ev == MG_EV_WS_MSG) {
        // When we get echo response, print it
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
        printf("GOT ECHO REPLY: [%.*s]\n", (int) wm->data.len, wm->data.buf);
        peer_signaling_process_request(wm->data.buf, wm->data.len);
    }

    if (ev == MG_EV_ERROR || ev == MG_EV_CLOSE || ev == MG_EV_WS_MSG) {
        *(bool *) c->fn_data = true;  // Signal that we're done
    }
}

int peer_signaling_join_channel(const char *client_id, PeerConnection *pc, struct mg_mgr *connection_mgr) {
    g_ps.mg_manager = connection_mgr;
	g_ps.pc = pc;
    g_ps.client_id = (char *)client_id;
	peer_connection_onicecandidate(pc, peer_signaling_onicecandidate);

//    peer_connection_create_offer(pc);

    bool done;
    g_ps.mg_connection = mg_ws_connect(connection_mgr, "ws://127.0.0.1:8090", peer_signaling_ws_callback, &done, NULL);
	
	return 0;
}

int peer_signaling_loop() {
    if (g_ps.mg_connection && g_ps.mg_connection->mgr) {
        mg_mgr_poll(g_ps.mg_connection->mgr, 1000);
    }
    return 0;
}

void peer_signaling_leave_channel() {

}

