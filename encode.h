#ifndef ENCODE_H
#define ENCODE_H

#include <stdio.h>
#include "types.h" /* Contains user defined types */

/*
 * Structure to store information required for
 * encoding secret file into source Image.
 * Info about output and intermediate data is
 * also stored here.
 */
typedef struct _EncodeInfo
{
	/* Source Image info */
	char *src_image_fname;
	FILE *fptr_src_image;
	uint image_capacity;

	/* Secret File Info */
	char *secret_fname;
	FILE *fptr_secret;
	char extn_secret_file[16];   /* enough for ".longext" + NUL */
	unsigned char secret_data[1024]; /* consider dynamic allocation for large secrets */
	long size_secret_file;

	/* Stego Image Info */
	char *stego_image_fname;
	FILE *fptr_stego_image;
} EncodeInfo;

/* Encoding function prototypes */

/* Read and validate Encode args from argv */
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo);

/* Perform the encoding workflow */
Status do_encoding(EncodeInfo *encInfo);

/* Open required files */
Status open_files(EncodeInfo *encInfo);

/* Check the image capacity is sufficient for encoding */
Status check_capacity(EncodeInfo *encInfo);

/* Get image size (width * height * bytes_per_pixel) for BMP */
uint get_image_size_for_bmp(FILE *fptr_image);

/* Get file size in bytes */
uint get_file_size(FILE *fptr);

/* Copy BMP image header (54 bytes) */
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image);

/* Store Magic String */
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo);

/* Encode extension size */
Status encode_secret_file_extn_size(int size, EncodeInfo *encInfo);

/* Encode secret file extension */
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo);

/* Encode secret file size */
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo);

/* Encode secret file data */
Status encode_secret_file_data(EncodeInfo *encInfo);

/* Encode a byte into LSBs of an 8-byte image buffer */
Status encode_byte_to_lsb(char data, unsigned char *image_buffer);

/* Encode a 32-bit size into 32 bytes (LSBs) */
Status encode_size_to_lsb(unsigned int size, unsigned char *imageBuffer);

/* Copy remaining image bytes from src to stego image after encoding */
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest);

#endif