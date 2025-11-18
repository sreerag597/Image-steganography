#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "types.h"
#include "common.h"

/* Get image size for 24-bit BMP */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;

    fseek(fptr_image, 18, SEEK_SET);
    fread(&width, sizeof(int), 1, fptr_image);
    printf("Width = %u\n",width);

    fread(&height, sizeof(int), 1, fptr_image);
    printf("Height = %u\n",height);

    return width * height * 3;
}

/* Get file size */
uint get_file_size(FILE *fptr)
{
    fseek(fptr, 0, SEEK_END);
    long size = ftell(fptr);
    rewind(fptr);
    return size;
}

/* Validate encode arguments */
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    char *point1 = strrchr(argv[2], '.');
    if (point1 == NULL || strcmp(point1, ".bmp") != 0)
    {
        printf("Error! File must end with .bmp\n");
        return e_failure;
    }
    encInfo->src_image_fname = argv[2];

    char *point2 = strrchr(argv[3], '.');
    if (point2 == NULL)
    {
        printf("Error! Secret file must have an extension\n");
        return e_failure;
    }
    encInfo->secret_fname = argv[3];

    if (argv[4] != NULL)
    {
        char *point3 = strrchr(argv[4], '.');
        if (point3 == NULL || strcmp(point3, ".bmp") != 0)
        {
            printf("Error! Output file must end with .bmp\n");
            return e_failure;
        }
        encInfo->stego_image_fname = argv[4];
    }
    else
    {
        encInfo->stego_image_fname = "default.bmp";
    }

    return e_success;
}

/* Open files */
Status open_files(EncodeInfo *encInfo)
{
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");
    if (encInfo->fptr_src_image == NULL)
    {
        perror("fopen");
        printf("Error! Unable to open file %s\n", encInfo->src_image_fname);
        return e_failure;
    }

    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    if (encInfo->fptr_secret == NULL)
    {
        perror("fopen");
        printf("Error! Unable to open file %s\n", encInfo->secret_fname);
        return e_failure;
    }

    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
    if (encInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        printf("Error! Unable to open file %s\n", encInfo->stego_image_fname);
        return e_failure;
    }

    return e_success;
}

/* Check image capacity */
Status check_capacity(EncodeInfo *encInfo)
{
    long image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);
    long file_size = get_file_size(encInfo->fptr_secret);
    long total_bytes = 16 + 32 + 32 + 32 + (8 * file_size);

    if (image_capacity > total_bytes)
        return e_success;
    else
        return e_failure;
}

/* Copy BMP header */
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    unsigned char image_buffer[54];
    rewind(fptr_src_image);

    if (fread(image_buffer, 1, 54, fptr_src_image) != 54)
    {
        return e_failure;
    }

    if (fwrite(image_buffer, 1, 54, fptr_dest_image) != 54)
    {
        return e_failure;
    }

    return e_success;
}

/* Encode byte into LSBs */
Status encode_byte_to_lsb(char data, unsigned char *image_buffer)
{
    for (int i = 0; i < 8; i++)
    {
        int bit = (data >> (7 - i)) & 1;
        image_buffer[i] = (image_buffer[i] & 0xFE) | bit;
    }
    return e_success;
}

/* Encode 32-bit size */
Status encode_size_to_lsb(unsigned int size, unsigned char *imageBuffer)
{
    for (int i = 0; i < 32; i++)
    {
        int bit = (size >> (31 - i)) & 1;
        imageBuffer[i] = (imageBuffer[i] & 0xFE) | bit;
    }

    return e_success;
}

/* Encode magic string */
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    unsigned char image_buffer[8];

    for (int i = 0; i < strlen(magic_string); i++)
    {
        if (fread(image_buffer, 1, 8, encInfo->fptr_src_image) != 8)
            return e_failure;

        encode_byte_to_lsb(magic_string[i], image_buffer);

        if (fwrite(image_buffer, 1, 8, encInfo->fptr_stego_image) != 8)
            return e_failure;
    }

    return e_success;
}

/* Encode secret file extension size */
Status encode_secret_file_extn_size(int size, EncodeInfo *encInfo)
{
    unsigned char image_buffer[32];

    if (fread(image_buffer, 1, 32, encInfo->fptr_src_image) != 32)
        return e_failure;

    encode_size_to_lsb(size, image_buffer);

    if (fwrite(image_buffer, 1, 32, encInfo->fptr_stego_image) != 32)
        return e_failure;

    return e_success;
}

/* Encode secret file extension */
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    unsigned char image_buffer[8];

    for (int i = 0; i < strlen(file_extn); i++)
    {
        if (fread(image_buffer, 1, 8, encInfo->fptr_src_image) != 8)
            return e_failure;

        encode_byte_to_lsb(file_extn[i], image_buffer);

        if (fwrite(image_buffer, 1, 8, encInfo->fptr_stego_image) != 8)
            return e_failure;
    }

    return e_success;
}

/* Encode secret file size */
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    unsigned char image_buffer[32];

    if (fread(image_buffer, 1, 32, encInfo->fptr_src_image) != 32)
        return e_failure;

    encode_size_to_lsb((unsigned int)file_size, image_buffer);

    if (fwrite(image_buffer, 1, 32, encInfo->fptr_stego_image) != 32)
        return e_failure;

    return e_success;
}

/* Encode secret file data */
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    unsigned char image_buffer[8];
    unsigned char ch;

    while (fread(&ch, 1, 1, encInfo->fptr_secret) == 1)
    {
        if (fread(image_buffer, 1, 8, encInfo->fptr_src_image) != 8)
            return e_failure;

        encode_byte_to_lsb(ch, image_buffer);

        if (fwrite(image_buffer, 1, 8, encInfo->fptr_stego_image) != 8)
            return e_failure;
    }

    return e_success;
}

/* Copy remaining bytes */
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    unsigned char buffer[1024];
    size_t bytesread;

    while ((bytesread = fread(buffer, 1, sizeof(buffer), fptr_src)) > 0)
    {
        if (fwrite(buffer, 1, bytesread, fptr_dest) != bytesread)
            return e_failure;
    }

    return e_success;
}

/* Main encoding process */
Status do_encoding(EncodeInfo *encInfo)
{
    printf("\n-----ENCODING-----\n\n");

    if (open_files(encInfo) == e_failure)
    {
        printf("Error! Failed to open files.\n");
        return e_failure;
    }
    printf("Files opened successfully.\n");

    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);
    rewind(encInfo->fptr_secret);

    if (check_capacity(encInfo) == e_failure)
    {
        printf("Error! Image capacity not accurate.\n");
        return e_failure;
    }
    printf("Image capacity checked successfully.\n");

    if (copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
    {
        printf("Error! Failed to copy BMP header.\n");
        return e_failure;
    }
    printf("BMP header copied successfully.\n");

    if (encode_magic_string(MAGIC_STRING, encInfo) == e_failure)
    {
        printf("Error! Failed to encode magic string.\n");
        return e_failure;
    }
    printf("Encoded magic string successfully.\n");

    char *file_extn = strrchr(encInfo->secret_fname, '.');
    if (file_extn == NULL)
    {
        printf("Error! Unable to find extension in secret file name.\n");
        return e_failure;
    }

    int extn_size = strlen(file_extn);

    if (encode_secret_file_extn_size(extn_size, encInfo) == e_failure)
    {
        printf("Error! Failed to encode secret file extension size.\n");
        return e_failure;
    }
    printf("Encoded secret file extension size successfully.\n");

    if (encode_secret_file_extn(file_extn, encInfo) == e_failure)
    {
        printf("Error! Failed to encode secret file extension.\n");
        return e_failure;
    }
    printf("Encoded secret file extension successfully.\n");

    if (encode_secret_file_size(encInfo->size_secret_file, encInfo) == e_failure)
    {
        printf("Error! Failed to encode secret file size.\n");
        return e_failure;
    }
    printf("Encoded secret file size successfully.\n");

    if (encode_secret_file_data(encInfo) == e_failure)
    {
        printf("Error! Failed to encode secret file data.\n");
        return e_failure;
    }
    printf("Encoded secret file data successfully.\n");

    if (copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
    {
        printf("Error! Failed to copy remaining image data.\n");
        return e_failure;
    }
    printf("Remaining image data copied successfully.\n");

    printf("\n ✅ ENCODING COMPLETED SUCCESSFULLY!\n");
    return e_success;
}