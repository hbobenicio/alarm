#include "wav-parser.h"

// posix
#include <arpa/inet.h>

static void fix_endianess(WAVHeader* header)
{
    // header->riff.id = ntohl(header->riff.id);
    header->riff.format = ntohl(header->riff.format);
    header->fmt.id = ntohl(header->fmt.id);
    header->data.id = ntohl(header->data.id);
}

WAVParser wav_parser_from_file(FILE* stream)
{
    return (WAVParser) {
        .input = stream,
    };
}

WAVHeader wav_parser_parse_header(WAVParser* parser)
{
    WAVHeader header;

    // TODO error handling
    // TODO better non-happy cases handling
    fread(&header, sizeof(WAVHeader), 1, parser->input);
    fix_endianess(&header);

    return header;
}
