#ifndef ALARM_WAV_PARSER_H
#define ALARM_WAV_PARSER_H

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

typedef struct {
    uint32_t id;
    uint32_t size;
    uint32_t format;
} WAVHeaderRiffSection;

static_assert(sizeof(WAVHeaderRiffSection) == 12, "wrong wav riff header size");

typedef struct {
    uint32_t   id;
    uint32_t   size;
    uint16_t   audio_fmt;
    uint16_t   num_channels;
    uint32_t   sample_rate;
    uint32_t   byte_rate;
    uint16_t   block_align;
    uint16_t   bits_per_sample;
} WAVHeaderFormatSection;

static_assert(sizeof(WAVHeaderFormatSection) == 24, "wrong wav format header size");

typedef struct {
    uint32_t id;
    uint32_t size;
    // unsigned char* data;
} WAVHeaderDataSection;

static_assert(sizeof(WAVHeaderDataSection) == 8, "wrong wav data header size");

typedef struct {
    WAVHeaderRiffSection riff;
    WAVHeaderFormatSection fmt;
    WAVHeaderDataSection data;
} WAVHeader;

static_assert(sizeof(WAVHeader) == sizeof(WAVHeaderRiffSection) + sizeof(WAVHeaderFormatSection) + sizeof(WAVHeaderDataSection), "wrong wav header size");

typedef struct {
    FILE* input;
} WAVParser;

WAVParser wav_parser_from_file(FILE* stream);
WAVHeader wav_parser_parse_header(WAVParser* parser);

#endif
