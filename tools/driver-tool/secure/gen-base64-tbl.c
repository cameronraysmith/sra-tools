#include <stdio.h>
#include <stdbool.h>

/* from binary 0..63 to standard BASE64 encoding */
static
const char encode_std_table [] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "+/";
    
/* from binary 0..63 to BASE64-URL encoding */
static
const char encode_url_table [] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "-_";

static
void gen_encode_tbl ( const char encode_tbl [ 64 ], const char * name )
{
    unsigned int i;

    printf ( "    static\n" );
    printf ( "    const char %s [] =\n", name );
    printf ( "        \"" );

    for ( i = 0; i < 26; ++ i )
        putchar ( encode_tbl [ i ] );

    printf ( "\"\n" );
    printf ( "        \"" );

    for ( ; i < 52; ++ i )
        putchar ( encode_tbl [ i ] );

    printf ( "\"\n" );
    printf ( "        \"" );

    for ( ; i < 62; ++ i )
        putchar ( encode_tbl [ i ] );

    printf ( "\"\n" );
    printf ( "        \"" );

    for ( ; i < 64; ++ i )
        putchar ( encode_tbl [ i ] );

    printf ( "\";\n\n" );
}

static
void gen_decode_tbl ( const char encode_table [ 64 ], const char * name, bool accept_white_space )
{
    unsigned int i;
    unsigned char table [ 256 ];

    /* all values are initially illegal: -1 = 0xff */
    for ( i = 0; i < sizeof table; ++ i )
        table [ i ] = -1;

    /* characters to ignore get -3 = 0xfd */
    if ( accept_white_space )
    {
        const char whitespace_table [] = "\t \r\n";
        for ( i = 0; i < sizeof whitespace_table -1; ++ i )
            table [ whitespace_table [ i ] ] = -3;
    }

    /* padding character gets -2 = 0xfe */
    table [ '=' ] = -2;

    /* base64 codes get their binary values */
    for ( i = 0; i < 64; ++ i )
        table [ encode_table [ i ] ] = i;

    printf ( "    static\n" );
    printf ( "    const char %s [] =\n", name );
    for ( i = 0; i < sizeof table; )
    {
        printf ( "        \"" );
        do
        {
            printf ( "\\x%02x", table [ i ] );
        }
        while ( ++ i % 16 != 0 );
        printf ( "\" // \\x%02X .. \\x%02X\n", i - 16, i - 1 );
    }

    printf ( "        ;\n\n" );
}

int main ()
{
    printf ( "// AUTO-GENERATED FILE\n"
             "// generated by %s\n\n", __FILE__ );

    printf ( "#pragma once\n\n" );

    printf ( "namespace ncbi\n{\n\n" );

    printf ( "    // from binary 0..63 to standard BASE64 encoding\n" );

    gen_encode_tbl ( encode_std_table, "encode_std_table" );

    printf ( "    // from octet stream ( presumed standard BASE64 encoding )\n"
             "    // to binary, where\n"
             "    //   0..63 is valid output\n"
             "    //  -1 ( \\xff ) is invalid output\n"
             "    //  -2 ( \\xfe ) is padding\n" );

    gen_decode_tbl ( encode_std_table, "decode_std_table", false );

    printf ( "    // from octet stream ( presumed standard BASE64 encoding )\n"
             "    // to binary, where\n"
             "    //   0..63 is valid output\n"
             "    //  -1 ( \\xff ) is invalid output\n"
             "    //  -2 ( \\xfe ) is padding\n"
             "    //  -3 ( \\xfd ) means ignore whitespace\n" );

    gen_decode_tbl ( encode_std_table, "decode_std_table_ws", true );


    printf ( "    // from binary 0..63 to BASE64-URL encoding\n" );

    gen_encode_tbl ( encode_url_table, "encode_url_table" );

    printf ( "    // from octet stream ( presumed BASE64-URL encoding )\n"
             "    // to binary, where\n"
             "    //   0..63 is valid output\n"
             "    //  -1 ( \\xff ) is invalid output\n"
             "    //  -2 ( \\xfe ) is padding\n" );

    gen_decode_tbl ( encode_url_table, "decode_url_table", false );

    printf ( "    // from octet stream ( presumed BASE64-URL encoding )\n"
             "    // to binary, where\n"
             "    //   0..63 is valid output\n"
             "    //  -1 ( \\xff ) is invalid output\n"
             "    //  -2 ( \\xfe ) is padding\n"
             "    //  -3 ( \\xfd ) means ignore whitespace\n" );

    gen_decode_tbl ( encode_url_table, "decode_url_table_ws", true );

    printf ( "}\n" );
}
