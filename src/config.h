#ifndef CONFIG_H_
#define CONFIG_H_

#define SCTP_MTU (1200)
#define CONFIG_MTU (1300)
#define RSA_KEY_LENGTH 1024

#ifdef ESP32
#define VIDEO_RB_DATA_LENGTH (CONFIG_MTU * 64)
#define AUDIO_RB_DATA_LENGTH (CONFIG_MTU * 64)
#define DATA_RB_DATA_LENGTH (SCTP_MTU * 128)
#define AUDIO_LATENCY 40 // ms
#else
#define HAVE_USRSCTP
#define VIDEO_RB_DATA_LENGTH (CONFIG_MTU * 1024)
#define AUDIO_RB_DATA_LENGTH (CONFIG_MTU * 256)
#define DATA_RB_DATA_LENGTH (SCTP_MTU * 128)
#define AUDIO_LATENCY 20 // ms
#endif

#ifndef CONFIG_MQTT
#define CONFIG_MQTT 1
#endif

#ifndef CONFIG_WHIP
#define CONFIG_WHIP 0
#endif

// siganling
#define MQTT_HOST "test.mosquitto.org"
#define MQTT_PORT 8883

#define WHIP_HOST "default.zlmediakit.com"
#define WHIP_PATH "/index/api/webrtc?app=live&stream=test&type=push"
#define WHIP_PORT 443

#define KEEPALIVE_CONNCHECK 0
#define CONFIG_IPV6 0
// default use wifi interface
#define IFR_NAME "w"

//#define LOG_LEVEL LEVEL_DEBUG

#endif // CONFIG_H_
