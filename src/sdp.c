#include <stdio.h>
#include <stdarg.h>

#include "sdp.h"

int sdp_append(Sdp *sdp, const char *format, ...) {

    va_list argptr;

    char attr[SDP_ATTR_LENGTH];

    memset(attr, 0, sizeof(attr));

    va_start(argptr, format);

    vsnprintf(attr, sizeof(attr), format, argptr);

    va_end(argptr);

    strcat(sdp->content, attr);
    strcat(sdp->content, "\r\n");
    return 0;
}

void sdp_reset(Sdp *sdp) {
    memset(sdp->content, 0, sizeof(sdp->content));
}

void sdp_append_h264(Sdp *sdp) {
    sdp_append(sdp, "m=video 9 UDP/TLS/RTP/SAVPF 96 127 112");

//    sdp_append(sdp, "m=video 9 UDP/TLS/RTP/SAVPF 102 104 96 98 108 127 39 112");
//    sdp_append(sdp, "a=rtpmap:102 H264/90000");
//    sdp_append(sdp, "a=rtcp-fb:102 goog-remb");
//    sdp_append(sdp, "a=rtcp-fb:102 transport-cc");
//    sdp_append(sdp, "a=rtcp-fb:102 ccm fir");
//    sdp_append(sdp, "a=rtcp-fb:102 nack");
//    sdp_append(sdp, "a=rtcp-fb:102 nack pli");
//    sdp_append(sdp, "a=fmtp:102 profile-level-id=42001f;packetization-mode=1;level-asymmetry-allowed=1");
//
//    sdp_append(sdp, "a=rtpmap:104 H264/90000");
//    sdp_append(sdp, "a=rtcp-fb:104 goog-remb");
//    sdp_append(sdp, "a=rtcp-fb:104 transport-cc");
//    sdp_append(sdp, "a=rtcp-fb:104 ccm fir");
//    sdp_append(sdp, "a=rtcp-fb:104 nack");
//    sdp_append(sdp, "a=rtcp-fb:104 nack pli");
//    sdp_append(sdp, "a=fmtp:104 profile-level-id=42001f;packetization-mode=1;level-asymmetry-allowed=1");
//
    sdp_append(sdp, "a=rtpmap:96 H264/90000");
    sdp_append(sdp, "a=rtcp-fb:96 goog-remb");
    sdp_append(sdp, "a=rtcp-fb:96 transport-cc");
    sdp_append(sdp, "a=rtcp-fb:96 ccm fir");
    sdp_append(sdp, "a=rtcp-fb:96 nack");
    sdp_append(sdp, "a=rtcp-fb:96 nack pli");
    sdp_append(sdp, "a=fmtp:96 profile-level-id=42e01f;packetization-mode=1;level-asymmetry-allowed=1");
//
//    sdp_append(sdp, "a=rtpmap:98 H264/90000");
//    sdp_append(sdp, "a=rtcp-fb:98 goog-remb");
//    sdp_append(sdp, "a=rtcp-fb:98 transport-cc");
//    sdp_append(sdp, "a=rtcp-fb:98 ccm fir");
//    sdp_append(sdp, "a=rtcp-fb:98 nack");
//    sdp_append(sdp, "a=rtcp-fb:98 nack pli");
//    sdp_append(sdp, "a=fmtp:98 profile-level-id=42e01f;packetization-mode=1;level-asymmetry-allowed=1");
//
//    sdp_append(sdp, "a=rtpmap:108 H264/90000");
//    sdp_append(sdp, "a=rtcp-fb:108 goog-remb");
//    sdp_append(sdp, "a=rtcp-fb:108 transport-cc");
//    sdp_append(sdp, "a=rtcp-fb:108 ccm fir");
//    sdp_append(sdp, "a=rtcp-fb:108 nack");
//    sdp_append(sdp, "a=rtcp-fb:108 nack pli");
//    sdp_append(sdp, "a=fmtp:108 profile-level-id=42e01f;packetization-mode=1;level-asymmetry-allowed=1");
//
    sdp_append(sdp, "a=rtpmap:127 H264/90000");
    sdp_append(sdp, "a=rtcp-fb:127 goog-remb");
    sdp_append(sdp, "a=rtcp-fb:127 transport-cc");
    sdp_append(sdp, "a=rtcp-fb:127 ccm fir");
    sdp_append(sdp, "a=rtcp-fb:127 nack");
    sdp_append(sdp, "a=rtcp-fb:127 nack pli");
    sdp_append(sdp, "a=fmtp:127 profile-level-id=4d001f;packetization-mode=1;level-asymmetry-allowed=1");
//
//    sdp_append(sdp, "a=rtpmap:39 H264/90000");
//    sdp_append(sdp, "a=rtcp-fb:39 goog-remb");
//    sdp_append(sdp, "a=rtcp-fb:39 transport-cc");
//    sdp_append(sdp, "a=rtcp-fb:39 ccm fir");
//    sdp_append(sdp, "a=rtcp-fb:39 nack");
//    sdp_append(sdp, "a=rtcp-fb:39 nack pli");
//    sdp_append(sdp, "a=fmtp:39 profile-level-id=4d001f;packetization-mode=1;level-asymmetry-allowed=1");

    sdp_append(sdp, "a=rtpmap:112 H264/90000");
    sdp_append(sdp, "a=rtcp-fb:112 goog-remb");
    sdp_append(sdp, "a=rtcp-fb:112 transport-cc");
    sdp_append(sdp, "a=rtcp-fb:112 ccm fir");
    sdp_append(sdp, "a=rtcp-fb:112 nack");
    sdp_append(sdp, "a=rtcp-fb:112 nack pli");
    sdp_append(sdp, "a=fmtp:112 profile-level-id=64001f;packetization-mode=1;level-asymmetry-allowed=1");

    sdp_append(sdp, "a=ssrc:1 cname:webrtc-h264");
    sdp_append(sdp, "a=mid:video");
//    sdp_append(sdp, "a=msid:stream video");
    sdp_append(sdp, "c=IN IP4 0.0.0.0");
    sdp_append(sdp, "a=sendonly");
    sdp_append(sdp, "a=rtcp-mux");
    sdp_append(sdp, "b=AS:20000");
}

void sdp_append_pcma(Sdp *sdp) {
    sdp_append(sdp, "m=audio 9 UDP/TLS/RTP/SAVP 8");
    sdp_append(sdp, "a=rtpmap:8 PCMA/8000");
    sdp_append(sdp, "a=ssrc:4 cname:webrtc-pcma");
    sdp_append(sdp, "a=sendonly");
    sdp_append(sdp, "a=mid:audio");
//    sdp_append(sdp, "a=msid:stream audio");
    sdp_append(sdp, "c=IN IP4 0.0.0.0");
    sdp_append(sdp, "a=rtcp-mux");
}

void sdp_append_pcmu(Sdp *sdp) {
    sdp_append(sdp, "m=audio 9 UDP/TLS/RTP/SAVP 0");
    sdp_append(sdp, "a=rtpmap:0 PCMU/8000");
    sdp_append(sdp, "a=ssrc:5 cname:webrtc-pcmu");
    sdp_append(sdp, "a=sendrecv");
    sdp_append(sdp, "a=mid:audio");
    sdp_append(sdp, "a=msid:stream audio");
    sdp_append(sdp, "c=IN IP4 0.0.0.0");
    sdp_append(sdp, "a=rtcp-mux");
}

void sdp_append_opus(Sdp *sdp) {
    sdp_append(sdp, "m=audio 9 UDP/TLS/RTP/SAVP 111");
    sdp_append(sdp, "a=rtpmap:111 opus/48000/2");
    sdp_append(sdp, "a=ssrc:6 cname:webrtc-opus");
    sdp_append(sdp, "a=sendrecv");
    sdp_append(sdp, "a=mid:audio");
    sdp_append(sdp, "a=msid:stream audio");
    sdp_append(sdp, "c=IN IP4 0.0.0.0");
    sdp_append(sdp, "a=rtcp-mux");
}

void sdp_append_datachannel(Sdp *sdp) {
    sdp_append(sdp, "m=application 50712 UDP/DTLS/SCTP webrtc-datachannel");
    sdp_append(sdp, "a=mid:datachannel");
    sdp_append(sdp, "a=sctp-port:5000");
    sdp_append(sdp, "c=IN IP4 0.0.0.0");
    sdp_append(sdp, "a=max-message-size:262144");
}

void sdp_create(Sdp *sdp, int b_video, int b_audio, int b_datachannel) {
    char bundle[64];
    sdp_append(sdp, "v=0");
    sdp_append(sdp, "o=- 1495799811084970 1495799811084970 IN IP4 0.0.0.0");
    sdp_append(sdp, "s=-");
    sdp_append(sdp, "t=0 0");
    sdp_append(sdp, "a=msid-semantic: WMS *");

    memset(bundle, 0, sizeof(bundle));

    strcat(bundle, "a=group:BUNDLE");

    if (b_video) {
        strcat(bundle, " video");
    }

    if (b_audio) {
        strcat(bundle, " audio");
    }

    if (b_datachannel) {
        strcat(bundle, " datachannel");
    }

    sdp_append(sdp, bundle);
}

