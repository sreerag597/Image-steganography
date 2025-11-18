#ifndef DECODE_H
#define DECODE_H

#include <stdio.h>
#include "types.h"

#define MAGIC_STRING "#*"

typedef struct _DecodeInfo
{
    char *stego_image_fname;
    char *secret_fname;

    FILE *fptr_stego_image;
    FILE *fptr_secret;

    int extn_size;
    char extn_secret_file[10];

    int size_secret_file;

} DecodeInfo;

Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);
Status open_decode_files(DecodeInfo *decInfo);

Status decode_magic_string(DecodeInfo *decInfo);
Status decode_secret_file_extn_size(DecodeInfo *decInfo);
Status decode_secret_file_extn(DecodeInfo *decInfo);
Status decode_secret_file_size(DecodeInfo *decInfo);
Status decode_secret_file_data(DecodeInfo *decInfo);

unsigned char decode_byte_from_lsb(const unsigned char *buffer);
unsigned int decode_size_from_lsb(const unsigned char *buffer);


Status do_decoding(DecodeInfo *decInfo);

#endif
