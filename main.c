#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "decode.h"
#include "types.h"

void print_usage()
{
    printf("\nUsage:\n\n");
    printf("For Encoding : ./a.out -e <source_image.bmp> <secret.txt> <output_image.bmp> (OPTIONAL)\n");
    printf("For Decoding : ./a.out -d <stego_image.bmp> <output_file>\n");
}

OperationType check_operation_type(char *);

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf(" ❌ Error ! Insufficient arguments\n");
        print_usage();
        return 1;
    }

    OperationType op = check_operation_type(argv[1]);

    switch(op)
    {
        case e_encode:
            printf("Selected operation : Encode\n");
            EncodeInfo encInfo;
            if (read_and_validate_encode_args(argv, &encInfo) == e_success)
            {
                if (do_encoding(&encInfo) != e_success)
                    printf(" ❌ ERROR: Encoding failed\n");
            }
            else
            {
                printf(" ❌ ERROR ! Validation failed\n");
                print_usage();
            }
            break;

        case e_decode:
            printf("Selected operation : Decode\n");
            DecodeInfo decInfo;
            if (read_and_validate_decode_args(argv, &decInfo) == e_success)
            {
                if (do_decoding(&decInfo) != e_success)
                    printf(" ❌ Error ! Decoding failed\n");
            }
            else
            {
                printf(" ❌ Error ! Validation failed\n");
                print_usage();
            }
            break;

        default:
            printf(" ❌ ERROR ! Unsupported operation\n");
            print_usage();
    }
    return 0;
}

OperationType check_operation_type(char *symbol)
{
    if(strcmp(symbol, "-e") == 0)
        return e_encode;

    if(strcmp(symbol, "-d") == 0)
        return e_decode;

    return e_unsupported;
}
