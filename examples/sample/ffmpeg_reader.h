//
// Created by NAVER on 6/7/24.
//

#ifndef PEER_FFMPEG_READER_H
#define PEER_FFMPEG_READER_H

#include <stdint.h>

int ffmpeg_reader_init();
int ffmpeg_reader_deinit();

int ffmpeg_reader_get_frame(uint8_t *buf, int *size);

#endif //PEER_FFMPEG_READER_H
