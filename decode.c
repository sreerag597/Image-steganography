#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "decode.h"
#include "common.h"

/* Validate decode arguments */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    char *pt = strrchr(argv[2], '.');
    if (pt == NULL || strcmp(pt, ".bmp") != 0)
    {
        printf("Error! File must end with .bmp\n");
        return e_failure;
    }
    decInfo->stego_image_fname = argv[2];

    if (argv[3] == NULL)
    {
        printf("Error! Output file not provided.\n");
        return e_failure;
    }
    decInfo->secret_fname = argv[3];

    return e_success;
}

/* Open files required for decoding */
Status open_decode_files(DecodeInfo *decInfo)
{
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "rb");
    if (decInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        printf("Error! Unable to open stego image file: %s\n", decInfo->stego_image_fname);
        return e_failure;
    }

    /* Skip BMP header */
    if (fseek(decInfo->fptr_stego_image, 54, SEEK_SET) != 0)
    {
        printf("Error! Failed to seek stego image.\n");
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }

    decInfo->fptr_secret = fopen(decInfo->secret_fname, "wb");
    if (decInfo->fptr_secret == NULL)
    {
        perror("fopen");
        printf("Error! Unable to create output secret file: %s\n", decInfo->secret_fname);
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }

    return e_success;
}

/* Decode one byte from LSBs of 8 bytes */
unsigned char decode_byte_from_lsb(const unsigned char *buffer)
{
    unsigned char byte = 0;
    for (int i = 0; i < 8; i++)
    {
        byte = (byte << 1) | (buffer[i] & 1);
    }
    return byte;
}

/* Decode 32-bit unsigned size from 32 bytes */
unsigned int decode_size_from_lsb(const unsigned char *buffer)
{
    unsigned int size = 0;
    for (int i = 0; i < 32; i++)
    {
        size = (size << 1) | (buffer[i] & 1);
    }
    return size;
}

/* Decode magic string "#*" */
Status decode_magic_string(DecodeInfo *decInfo)
{
    unsigned char buffer[8];
    char decoded_magic[3] = {0};

    const size_t magic_len = strlen(MAGIC_STRING);

    for (size_t i = 0; i < magic_len; i++)
    {
        if (fread(buffer, 1, 8, decInfo->fptr_stego_image) != 8)
        {
            printf("Error! Failed to read image buffer while decoding magic string.\n");
            return e_failure;
        }

        decoded_magic[i] = (char)decode_byte_from_lsb(buffer);
    }

    if (strcmp(decoded_magic, MAGIC_STRING) != 0)
    {
        printf("Error! Magic string mismatch.\n");
        return e_failure;
    }

    return e_success;
}

/* Decode extension size (32 bits) */
Status decode_secret_file_extn_size(DecodeInfo *decInfo)
{
    unsigned char buffer[32];

    if (fread(buffer, 1, 32, decInfo->fptr_stego_image) != 32)
    {
        printf("Error! Failed to read data while decoding extension size.\n");
        return e_failure;
    }

    unsigned int extn_size = decode_size_from_lsb(buffer);
    decInfo->extn_size = (int)extn_size;

    
    if (decInfo->extn_size <= 0 || decInfo->extn_size > (int)sizeof(decInfo->extn_secret_file) - 1)
    {
        printf("Error! Decoded extension size is invalid: %d\n", decInfo->extn_size);
        return e_failure;
    }

    return e_success;
}

/* Decode extension string into decInfo->extn_secret_file (assumes buffer in header) */
Status decode_secret_file_extn(DecodeInfo *decInfo)
{
    unsigned char buffer[8];

    for (int i = 0; i < decInfo->extn_size; i++)
    {
        if (fread(buffer, 1, 8, decInfo->fptr_stego_image) != 8)
        {
            printf("Error! Failed to read extension data.\n");
            return e_failure;
        }

        decInfo->extn_secret_file[i] = (char)decode_byte_from_lsb(buffer);
    }

    decInfo->extn_secret_file[decInfo->extn_size] = '\0';

    return e_success;
}

/* Decode secret file size (32 bits) */
Status decode_secret_file_size(DecodeInfo *decInfo)
{
    unsigned char buffer[32];

    if (fread(buffer, 1, 32, decInfo->fptr_stego_image) != 32)
    {
        printf("Error! Failed to read image buffer.\n");
        return e_failure;
    }

    unsigned int size = decode_size_from_lsb(buffer);
    decInfo->size_secret_file = (int)size;

    
    if (decInfo->size_secret_file < 0)
    {
        printf("Error! Invalid secret file size decoded: %d\n", decInfo->size_secret_file);
        return e_failure;
    }

    printf("Decoded secret size = %d\n", decInfo->size_secret_file);
    return e_success;
}

/* Decode secret file data and write to output */
Status decode_secret_file_data(DecodeInfo *decInfo)
{
    unsigned char buffer[8];
    unsigned char decoded_byte;

    for (int i = 0; i < decInfo->size_secret_file; i++)
    {
        if (fread(buffer, 1, 8, decInfo->fptr_stego_image) != 8)
        {
            printf("Error! Failed to read image data while decoding secret file data.\n");
            return e_failure;
        }

        decoded_byte = decode_byte_from_lsb(buffer);

        if (fwrite(&decoded_byte, 1, 1, decInfo->fptr_secret) != 1)
        {
            printf("Error! Failed to write output to secret file.\n");
            return e_failure;
        }
    }

    /* Close files */
    fclose(decInfo->fptr_stego_image);
    fclose(decInfo->fptr_secret);

    return e_success;
}

/*decoding steps */
Status do_decoding(DecodeInfo *decInfo)
{
    printf("\n-----DECODING-----\n\n");

    if (open_decode_files(decInfo) == e_failure)
    {
        printf("Failed to open files.\n");
        return e_failure;
    }
    printf("Opened files successfully.\n");

    if (decode_magic_string(decInfo) == e_failure)
    {
        printf("Failed to decode magic string.\n");
        return e_failure;
    }
    printf("Decoded magic string successfully.\n");

    if (decode_secret_file_extn_size(decInfo) == e_failure)
    {
        printf("Failed to decode secret file extension size.\n");
        return e_failure;
    }
    printf("Decoded secret file extension size successfully.\n");

    if (decode_secret_file_extn(decInfo) == e_failure)
    {
        printf("Failed to decode secret file extension.\n");
        return e_failure;
    }
    printf("Decoded secret file extension successfully.\n");

    if (decode_secret_file_size(decInfo) == e_failure)
    {
        printf("Failed to decode secret file size.\n");
        return e_failure;
    }
    printf("Decoded secret file size successfully.\n");

    if (decode_secret_file_data(decInfo) == e_failure)
    {
        printf("Failed to decode secret file data.\n");
        return e_failure;
    }
    printf("Decoded secret file data successfully.\n");

    printf("\n ✅ DECODING COMPLETED SUCCESSFULLY!\n");
    return e_success;
}