//
// Created by NAVER on 6/7/24.
//

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>

static const char *video_file = "/Users/naver/Movies/output.mkv";
static AVFormatContext *format_context;
static int video_stream_index = -1;
static int audio_stream_index = -1;
static AVCodecContext *codec_context;

int ffmpeg_reader_init() {
    format_context = avformat_alloc_context();
    if (avformat_open_input(&format_context, video_file, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open input file.\n");
        return -1;
    }

    if (avformat_find_stream_info(format_context, NULL) < 0) {
        fprintf(stderr, "Could not find stream information.\n");
        return -1;
    }

    av_dump_format(format_context, 0, video_file, 0);

    for (int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
        }
		if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream_index = i;
		}
    }

    if (video_stream_index == -1) {
        fprintf(stderr, "Could not find a video stream.\n");
        return -1;
    }
	
    AVCodecParameters *codec_params = format_context->streams[video_stream_index]->codecpar;
    AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
        fprintf(stderr, "Unsupported codec.\n");
        return -1;
    }

    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        fprintf(stderr, "Could not allocate codec context.\n");
        return -1;
    }

    if (avcodec_parameters_to_context(codec_context, codec_params) < 0) {
        fprintf(stderr, "Could not copy codec parameters.\n");
        return -1;
    }

    if (avcodec_open2(codec_context, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec.\n");
        return -1;
    }

    return 0;
}

int ffmpeg_reader_get_frame(uint8_t *buf, int *size, int audio) {
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate frame.\n");
        return -1;
    }

    AVPacket packet;
    if (av_read_frame(format_context, &packet) >= 0) {
		int target_stream_index = -1;
		if (audio) {
			target_stream_index = audio_stream_index;
		} else {
			target_stream_index = video_stream_index;
		}
        if (packet.stream_index == target_stream_index) {
            if (avcodec_send_packet(codec_context, &packet) == 0) {
                if (avcodec_receive_frame(codec_context, frame) == 0) {
//                    int key_frame = packet.flags & AV_PKT_FLAG_KEY;
                    *size = packet.size;
                    memcpy(buf, packet.data, packet.size);
//                    printf("Frame %d (type=%c, size=%d bytes, format=%d) pts %" PRId64 " key_frame %d [DTS %" PRId64 "]\n",
//                           frame_index,
//                           av_get_picture_type_char(frame->pict_type),
//                           packet.size,
//                           frame->format,
//                           frame->best_effort_timestamp,
//                           key_frame,
//                           packet.dts);
                }
            }
        }
        av_packet_unref(&packet);
    }

    av_frame_free(&frame);

    return 0;
}

int ffmpeg_reader_deinit() {
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);
    return 0;
}
