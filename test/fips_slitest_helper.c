#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

#include "testutil.h"
#include "fips_slitest_helper.h"

int make_hmac_keys(EVP_PKEY** skey, EVP_PKEY** vkey) {
    byte hkey[EVP_MAX_MD_SIZE];

    int result = -1;

    if (!skey || !vkey)
        return -1;

    if (*skey != NULL) {
        EVP_PKEY_free(*skey);
        *skey = NULL;
    }

    if (*vkey != NULL) {
        EVP_PKEY_free(*vkey);
        *vkey = NULL;
    }

    do {
        const EVP_MD* md = EVP_sha256();
        int size = EVP_MD_size(md);
        assert(size >= 16);
        if (!(size >= 16)) {
            printf("EVP_MD_size failed, error 0x%lx\n", ERR_get_error());
            break; /* failed */
        }

        assert(size <= sizeof(hkey));
        if (!(size <= sizeof(hkey))) {
            printf("EVP_MD_size is too large\n");
            break; /* failed */
        }

        /* Generate bytes */
        int rc = RAND_bytes(hkey, size);
        assert(rc == 1);
        if (rc != 1) {
            printf("RAND_bytes failed, error 0x%lx\n", ERR_get_error());
            break;
        }

        //print_it("HMAC key", hkey, size);

        *skey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, NULL, hkey, size);
        assert(*skey != NULL);
        if (*skey == NULL) {
            printf("EVP_PKEY_new_mac_key 1st failed, error 0x%lx\n", ERR_get_error());
            break;
        }

        *vkey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, NULL, hkey, size);
        assert(*vkey != NULL);
        if (*vkey == NULL) {
            printf("EVP_PKEY_new_mac_key 2nd failed, error 0x%lx\n", ERR_get_error());
            break;
        }

        result = 0;

    } while (0);

    OPENSSL_cleanse(hkey, sizeof(hkey));

    /* Convert to 0/1 result */
    return !!result;
}
static const uint8_t msg[16] = { 0x71, 0x50, 0x00, 0x00,
                                 0xc0, 0xff, 0xee, 0x00,
                                 0xde, 0xad, 0xbe, 0xef,
                                 0x01, 0x02, 0x03, 0x04
                               };
static const uint8_t msg128[128] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                                    13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
                                    24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
                                    35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
                                    46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
                                    57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
                                    68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78,
                                    79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
                                    90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
                                    101, 102, 103, 104, 105, 106, 107, 108, 109,
                                    110, 111, 112, 113, 114, 115, 116, 117, 118,
                                    119, 120, 121, 122, 123, 124, 125, 126, 127
                                   };

static const uint8_t key16[16] = { 0xaa, 0xbb, 0xcc, 0xdd,
                                   0x01, 0x02, 0x03, 0x04,
                                   0xaa, 0xbb, 0xcc, 0xdd,
                                   0xde, 0xad, 0xbe, 0xef
                                 };
static const uint8_t key32[32] = { 0xaa, 0xbb, 0xcc, 0xdd,
                                   0x01, 0x02, 0x03, 0x04,
                                   0xaa, 0xbb, 0xcc, 0xdd,
                                   0xde, 0xad, 0xbe, 0xef,
                                   0xaa, 0xbb, 0xcc, 0xdd,
                                   0x01, 0x02, 0x03, 0x04,
                                   0xaa, 0xbb, 0xcc, 0xdd,
                                   0xde, 0xad, 0xbe, 0xef
                                 };

const uint8_t* get_msg_16() {
    return msg;
}

const uint8_t* get_msg_128() {
    return msg128;
}

const uint8_t* get_key_16() {
    return key16;
}

const uint8_t* get_key_32() {
    return key32;
}

void print_it(const char* label, const byte* buff, size_t len) {
    if (!buff || !len)
        return;

    if (label)
        printf("%s: ", label);

    for (size_t i = 0; i < len; ++i)
        printf("%02X", buff[i]);

    printf("\n");
}

int get_cmac_key(int cipher_nid, EVP_PKEY** out) {
    int success = 0;
    EVP_PKEY_CTX *ctx = NULL;
    const size_t key_len = 32;
    const uint8_t *key = get_key_32();
    const EVP_CIPHER *cipher = EVP_get_cipherbynid(cipher_nid);

    if (!(ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_CMAC, NULL)))
        goto end;
    if (EVP_PKEY_keygen_init(ctx) != 1)
        goto end;
    if (cipher == NULL)
        goto end;
    if (EVP_PKEY_CTX_ctrl(ctx, -1, EVP_PKEY_OP_KEYGEN, EVP_PKEY_CTRL_CIPHER, 0, (void*)cipher) <= 0)
        goto end;
    if (EVP_PKEY_CTX_ctrl(ctx, -1, EVP_PKEY_OP_KEYGEN, EVP_PKEY_CTRL_SET_MAC_KEY, key_len, (void*)key) <= 0)
        goto end;
    if (!EVP_PKEY_keygen(ctx, out))
        goto end;

    success = 1;
end:

    if (ctx)
        EVP_PKEY_CTX_free(ctx);
    return success;
}

#ifndef OPENSSL_NO_RSA
# include <openssl/rsa.h>

/* adapted from openssl/test/rsa_test.c */
# define SetKey \
    RSA_set0_key(key,                                           \
                 BN_bin2bn(n, sizeof(n)-1, NULL),               \
                 BN_bin2bn(e, sizeof(e)-1, NULL),               \
                 BN_bin2bn(d, sizeof(d)-1, NULL));              \
    RSA_set0_factors(key,                                       \
                     BN_bin2bn(p, sizeof(p)-1, NULL),           \
                     BN_bin2bn(q, sizeof(q)-1, NULL));          \
    RSA_set0_crt_params(key,                                    \
                        BN_bin2bn(dmp1, sizeof(dmp1)-1, NULL),  \
                        BN_bin2bn(dmq1, sizeof(dmq1)-1, NULL),  \
                        BN_bin2bn(iqmp, sizeof(iqmp)-1, NULL));

void get_rsa_key1(RSA *key) {
    static unsigned char n[] =
        "\x00\xAA\x36\xAB\xCE\x88\xAC\xFD\xFF\x55\x52\x3C\x7F\xC4\x52\x3F"
        "\x90\xEF\xA0\x0D\xF3\x77\x4A\x25\x9F\x2E\x62\xB4\xC5\xD9\x9C\xB5"
        "\xAD\xB3\x00\xA0\x28\x5E\x53\x01\x93\x0E\x0C\x70\xFB\x68\x76\x93"
        "\x9C\xE6\x16\xCE\x62\x4A\x11\xE0\x08\x6D\x34\x1E\xBC\xAC\xA0\xA1"
        "\xF5";

    static unsigned char e[] = "\x11";

    static unsigned char d[] =
        "\x0A\x03\x37\x48\x62\x64\x87\x69\x5F\x5F\x30\xBC\x38\xB9\x8B\x44"
        "\xC2\xCD\x2D\xFF\x43\x40\x98\xCD\x20\xD8\xA1\x38\xD0\x90\xBF\x64"
        "\x79\x7C\x3F\xA7\xA2\xCD\xCB\x3C\xD1\xE0\xBD\xBA\x26\x54\xB4\xF9"
        "\xDF\x8E\x8A\xE5\x9D\x73\x3D\x9F\x33\xB3\x01\x62\x4A\xFD\x1D\x51";

    static unsigned char p[] =
        "\x00\xD8\x40\xB4\x16\x66\xB4\x2E\x92\xEA\x0D\xA3\xB4\x32\x04\xB5"
        "\xCF\xCE\x33\x52\x52\x4D\x04\x16\xA5\xA4\x41\xE7\x00\xAF\x46\x12"
        "\x0D";

    static unsigned char q[] =
        "\x00\xC9\x7F\xB1\xF0\x27\xF4\x53\xF6\x34\x12\x33\xEA\xAA\xD1\xD9"
        "\x35\x3F\x6C\x42\xD0\x88\x66\xB1\xD0\x5A\x0F\x20\x35\x02\x8B\x9D"
        "\x89";

    static unsigned char dmp1[] =
        "\x59\x0B\x95\x72\xA2\xC2\xA9\xC4\x06\x05\x9D\xC2\xAB\x2F\x1D\xAF"
        "\xEB\x7E\x8B\x4F\x10\xA7\x54\x9E\x8E\xED\xF5\xB4\xFC\xE0\x9E\x05";

    static unsigned char dmq1[] =
        "\x00\x8E\x3C\x05\x21\xFE\x15\xE0\xEA\x06\xA3\x6F\xF0\xF1\x0C\x99"
        "\x52\xC3\x5B\x7A\x75\x14\xFD\x32\x38\xB8\x0A\xAD\x52\x98\x62\x8D"
        "\x51";

    static unsigned char iqmp[] =
        "\x36\x3F\xF7\x18\x9D\xA8\xE9\x0B\x1D\x34\x1F\x71\xD0\x9B\x76\xA8"
        "\xA9\x43\xE1\x1D\x10\xB2\x4D\x24\x9F\x2D\xEA\xFE\xF8\x0C\x18\x26";

    SetKey;
}

void get_rsa_key2(RSA *key) {
    static unsigned char n[] =
        "\x00\xA3\x07\x9A\x90\xDF\x0D\xFD\x72\xAC\x09\x0C\xCC\x2A\x78\xB8"
        "\x74\x13\x13\x3E\x40\x75\x9C\x98\xFA\xF8\x20\x4F\x35\x8A\x0B\x26"
        "\x3C\x67\x70\xE7\x83\xA9\x3B\x69\x71\xB7\x37\x79\xD2\x71\x7B\xE8"
        "\x34\x77\xCF";

    static unsigned char e[] = "\x3";

    static unsigned char d[] =
        "\x6C\xAF\xBC\x60\x94\xB3\xFE\x4C\x72\xB0\xB3\x32\xC6\xFB\x25\xA2"
        "\xB7\x62\x29\x80\x4E\x68\x65\xFC\xA4\x5A\x74\xDF\x0F\x8F\xB8\x41"
        "\x3B\x52\xC0\xD0\xE5\x3D\x9B\x59\x0F\xF1\x9B\xE7\x9F\x49\xDD\x21"
        "\xE5\xEB";

    static unsigned char p[] =
        "\x00\xCF\x20\x35\x02\x8B\x9D\x86\x98\x40\xB4\x16\x66\xB4\x2E\x92"
        "\xEA\x0D\xA3\xB4\x32\x04\xB5\xCF\xCE\x91";

    static unsigned char q[] =
        "\x00\xC9\x7F\xB1\xF0\x27\xF4\x53\xF6\x34\x12\x33\xEA\xAA\xD1\xD9"
        "\x35\x3F\x6C\x42\xD0\x88\x66\xB1\xD0\x5F";

    static unsigned char dmp1[] =
        "\x00\x8A\x15\x78\xAC\x5D\x13\xAF\x10\x2B\x22\xB9\x99\xCD\x74\x61"
        "\xF1\x5E\x6D\x22\xCC\x03\x23\xDF\xDF\x0B";

    static unsigned char dmq1[] =
        "\x00\x86\x55\x21\x4A\xC5\x4D\x8D\x4E\xCD\x61\x77\xF1\xC7\x36\x90"
        "\xCE\x2A\x48\x2C\x8B\x05\x99\xCB\xE0\x3F";

    static unsigned char iqmp[] =
        "\x00\x83\xEF\xEF\xB8\xA9\xA4\x0D\x1D\xB6\xED\x98\xAD\x84\xED\x13"
        "\x35\xDC\xC1\x08\xF3\x22\xD0\x57\xCF\x8D";

    SetKey;
}

void get_rsa_key3(RSA *key) {
    static unsigned char n[] =
        "\x00\xBB\xF8\x2F\x09\x06\x82\xCE\x9C\x23\x38\xAC\x2B\x9D\xA8\x71"
        "\xF7\x36\x8D\x07\xEE\xD4\x10\x43\xA4\x40\xD6\xB6\xF0\x74\x54\xF5"
        "\x1F\xB8\xDF\xBA\xAF\x03\x5C\x02\xAB\x61\xEA\x48\xCE\xEB\x6F\xCD"
        "\x48\x76\xED\x52\x0D\x60\xE1\xEC\x46\x19\x71\x9D\x8A\x5B\x8B\x80"
        "\x7F\xAF\xB8\xE0\xA3\xDF\xC7\x37\x72\x3E\xE6\xB4\xB7\xD9\x3A\x25"
        "\x84\xEE\x6A\x64\x9D\x06\x09\x53\x74\x88\x34\xB2\x45\x45\x98\x39"
        "\x4E\xE0\xAA\xB1\x2D\x7B\x61\xA5\x1F\x52\x7A\x9A\x41\xF6\xC1\x68"
        "\x7F\xE2\x53\x72\x98\xCA\x2A\x8F\x59\x46\xF8\xE5\xFD\x09\x1D\xBD"
        "\xCB";

    static unsigned char e[] = "\x11";

    static unsigned char d[] =
        "\x00\xA5\xDA\xFC\x53\x41\xFA\xF2\x89\xC4\xB9\x88\xDB\x30\xC1\xCD"
        "\xF8\x3F\x31\x25\x1E\x06\x68\xB4\x27\x84\x81\x38\x01\x57\x96\x41"
        "\xB2\x94\x10\xB3\xC7\x99\x8D\x6B\xC4\x65\x74\x5E\x5C\x39\x26\x69"
        "\xD6\x87\x0D\xA2\xC0\x82\xA9\x39\xE3\x7F\xDC\xB8\x2E\xC9\x3E\xDA"
        "\xC9\x7F\xF3\xAD\x59\x50\xAC\xCF\xBC\x11\x1C\x76\xF1\xA9\x52\x94"
        "\x44\xE5\x6A\xAF\x68\xC5\x6C\x09\x2C\xD3\x8D\xC3\xBE\xF5\xD2\x0A"
        "\x93\x99\x26\xED\x4F\x74\xA1\x3E\xDD\xFB\xE1\xA1\xCE\xCC\x48\x94"
        "\xAF\x94\x28\xC2\xB7\xB8\x88\x3F\xE4\x46\x3A\x4B\xC8\x5B\x1C\xB3"
        "\xC1";

    static unsigned char p[] =
        "\x00\xEE\xCF\xAE\x81\xB1\xB9\xB3\xC9\x08\x81\x0B\x10\xA1\xB5\x60"
        "\x01\x99\xEB\x9F\x44\xAE\xF4\xFD\xA4\x93\xB8\x1A\x9E\x3D\x84\xF6"
        "\x32\x12\x4E\xF0\x23\x6E\x5D\x1E\x3B\x7E\x28\xFA\xE7\xAA\x04\x0A"
        "\x2D\x5B\x25\x21\x76\x45\x9D\x1F\x39\x75\x41\xBA\x2A\x58\xFB\x65"
        "\x99";

    static unsigned char q[] =
        "\x00\xC9\x7F\xB1\xF0\x27\xF4\x53\xF6\x34\x12\x33\xEA\xAA\xD1\xD9"
        "\x35\x3F\x6C\x42\xD0\x88\x66\xB1\xD0\x5A\x0F\x20\x35\x02\x8B\x9D"
        "\x86\x98\x40\xB4\x16\x66\xB4\x2E\x92\xEA\x0D\xA3\xB4\x32\x04\xB5"
        "\xCF\xCE\x33\x52\x52\x4D\x04\x16\xA5\xA4\x41\xE7\x00\xAF\x46\x15"
        "\x03";

    static unsigned char dmp1[] =
        "\x54\x49\x4C\xA6\x3E\xBA\x03\x37\xE4\xE2\x40\x23\xFC\xD6\x9A\x5A"
        "\xEB\x07\xDD\xDC\x01\x83\xA4\xD0\xAC\x9B\x54\xB0\x51\xF2\xB1\x3E"
        "\xD9\x49\x09\x75\xEA\xB7\x74\x14\xFF\x59\xC1\xF7\x69\x2E\x9A\x2E"
        "\x20\x2B\x38\xFC\x91\x0A\x47\x41\x74\xAD\xC9\x3C\x1F\x67\xC9\x81";

    static unsigned char dmq1[] =
        "\x47\x1E\x02\x90\xFF\x0A\xF0\x75\x03\x51\xB7\xF8\x78\x86\x4C\xA9"
        "\x61\xAD\xBD\x3A\x8A\x7E\x99\x1C\x5C\x05\x56\xA9\x4C\x31\x46\xA7"
        "\xF9\x80\x3F\x8F\x6F\x8A\xE3\x42\xE9\x31\xFD\x8A\xE4\x7A\x22\x0D"
        "\x1B\x99\xA4\x95\x84\x98\x07\xFE\x39\xF9\x24\x5A\x98\x36\xDA\x3D";

    static unsigned char iqmp[] =
        "\x00\xB0\x6C\x4F\xDA\xBB\x63\x01\x19\x8D\x26\x5B\xDB\xAE\x94\x23"
        "\xB3\x80\xF2\x71\xF7\x34\x53\x88\x50\x93\x07\x7F\xCD\x39\xE2\x11"
        "\x9F\xC9\x86\x32\x15\x4F\x58\x83\xB1\x67\xA9\x67\xBF\x40\x2B\x4E"
        "\x9E\x2E\x0F\x96\x56\xE6\x98\xEA\x36\x66\xED\xFB\x25\x79\x80\x39"
        "\xF7";

    SetKey;
}

/* taken from openssl/test/rsa_mp_test.c */
#define NUM_EXTRA_PRIMES 1
/* Returns the public key length (256) on success */
int get_rsa_key2048p3(RSA *key) {
    /* C90 requires string should <= 509 bytes */
    static const unsigned char n[] =
        "\x92\x60\xd0\x75\x0a\xe1\x17\xee\xe5\x5c\x3f\x3d\xea\xba\x74\x91"
        "\x75\x21\xa2\x62\xee\x76\x00\x7c\xdf\x8a\x56\x75\x5a\xd7\x3a\x15"
        "\x98\xa1\x40\x84\x10\xa0\x14\x34\xc3\xf5\xbc\x54\xa8\x8b\x57\xfa"
        "\x19\xfc\x43\x28\xda\xea\x07\x50\xa4\xc4\x4e\x88\xcf\xf3\xb2\x38"
        "\x26\x21\xb8\x0f\x67\x04\x64\x43\x3e\x43\x36\xe6\xd0\x03\xe8\xcd"
        "\x65\xbf\xf2\x11\xda\x14\x4b\x88\x29\x1c\x22\x59\xa0\x0a\x72\xb7"
        "\x11\xc1\x16\xef\x76\x86\xe8\xfe\xe3\x4e\x4d\x93\x3c\x86\x81\x87"
        "\xbd\xc2\x6f\x7b\xe0\x71\x49\x3c\x86\xf7\xa5\x94\x1c\x35\x10\x80"
        "\x6a\xd6\x7b\x0f\x94\xd8\x8f\x5c\xf5\xc0\x2a\x09\x28\x21\xd8\x62"
        "\x6e\x89\x32\xb6\x5c\x5b\xd8\xc9\x20\x49\xc2\x10\x93\x2b\x7a\xfa"
        "\x7a\xc5\x9c\x0e\x88\x6a\xe5\xc1\xed\xb0\x0d\x8c\xe2\xc5\x76\x33"
        "\xdb\x26\xbd\x66\x39\xbf\xf7\x3c\xee\x82\xbe\x92\x75\xc4\x02\xb4"
        "\xcf\x2a\x43\x88\xda\x8c\xf8\xc6\x4e\xef\xe1\xc5\xa0\xf5\xab\x80"
        "\x57\xc3\x9f\xa5\xc0\x58\x9c\x3e\x25\x3f\x09\x60\x33\x23\x00\xf9"
        "\x4b\xea\x44\x87\x7b\x58\x8e\x1e\xdb\xde\x97\xcf\x23\x60\x72\x7a"
        "\x09\xb7\x75\x26\x2d\x7e\xe5\x52\xb3\x31\x9b\x92\x66\xf0\x5a\x25";

    static const unsigned char e[] = "\x01\x00\x01";

    static const unsigned char d[] =
        "\x6a\x7d\xf2\xca\x63\xea\xd4\xdd\xa1\x91\xd6\x14\xb6\xb3\x85\xe0"
        "\xd9\x05\x6a\x3d\x6d\x5c\xfe\x07\xdb\x1d\xaa\xbe\xe0\x22\xdb\x08"
        "\x21\x2d\x97\x61\x3d\x33\x28\xe0\x26\x7c\x9d\xd2\x3d\x78\x7a\xbd"
        "\xe2\xaf\xcb\x30\x6a\xeb\x7d\xfc\xe6\x92\x46\xcc\x73\xf5\xc8\x7f"
        "\xdf\x06\x03\x01\x79\xa2\x11\x4b\x76\x7d\xb1\xf0\x83\xff\x84\x1c"
        "\x02\x5d\x7d\xc0\x0c\xd8\x24\x35\xb9\xa9\x0f\x69\x53\x69\xe9\x4d"
        "\xf2\x3d\x2c\xe4\x58\xbc\x3b\x32\x83\xad\x8b\xba\x2b\x8f\xa1\xba"
        "\x62\xe2\xdc\xe9\xac\xcf\xf3\x79\x9a\xae\x7c\x84\x00\x16\xf3\xba"
        "\x8e\x00\x48\xc0\xb6\xcc\x43\x39\xaf\x71\x61\x00\x3a\x5b\xeb\x86"
        "\x4a\x01\x64\xb2\xc1\xc9\x23\x7b\x64\xbc\x87\x55\x69\x94\x35\x1b"
        "\x27\x50\x6c\x33\xd4\xbc\xdf\xce\x0f\x9c\x49\x1a\x7d\x6b\x06\x28"
        "\xc7\xc8\x52\xbe\x4f\x0a\x9c\x31\x32\xb2\xed\x3a\x2c\x88\x81\xe9"
        "\xaa\xb0\x7e\x20\xe1\x7d\xeb\x07\x46\x91\xbe\x67\x77\x76\xa7\x8b"
        "\x5c\x50\x2e\x05\xd9\xbd\xde\x72\x12\x6b\x37\x38\x69\x5e\x2d\xd1"
        "\xa0\xa9\x8a\x14\x24\x7c\x65\xd8\xa7\xee\x79\x43\x2a\x09\x2c\xb0"
        "\x72\x1a\x12\xdf\x79\x8e\x44\xf7\xcf\xce\x0c\x49\x81\x47\xa9\xb1";

    static const unsigned char p[] =
        "\x06\x77\xcd\xd5\x46\x9b\xc1\xd5\x58\x00\x81\xe2\xf3\x0a\x36\xb1"
        "\x6e\x29\x89\xd5\x2f\x31\x5f\x92\x22\x3b\x9b\x75\x30\x82\xfa\xc5"
        "\xf5\xde\x8a\x36\xdb\xc6\xe5\x8f\xef\x14\x37\xd6\x00\xf9\xab\x90"
        "\x9b\x5d\x57\x4c\xf5\x1f\x77\xc4\xbb\x8b\xdd\x9b\x67\x11\x45\xb2"
        "\x64\xe8\xac\xa8\x03\x0f\x16\x0d\x5d\x2d\x53\x07\x23\xfb\x62\x0d"
        "\xe6\x16\xd3\x23\xe8\xb3";

    static const unsigned char q[] =
        "\x06\x66\x9a\x70\x53\xd6\x72\x74\xfd\xea\x45\xc3\xc0\x17\xae\xde"
        "\x79\x17\xae\x79\xde\xfc\x0e\xf7\xa4\x3a\x8c\x43\x8f\xc7\x8a\xa2"
        "\x2c\x51\xc4\xd0\x72\x89\x73\x5c\x61\xbe\xfd\x54\x3f\x92\x65\xde"
        "\x4d\x65\x71\x70\xf6\xf2\xe5\x98\xb9\x0f\xd1\x0b\xe6\x95\x09\x4a"
        "\x7a\xdf\xf3\x10\x16\xd0\x60\xfc\xa5\x10\x34\x97\x37\x6f\x0a\xd5"
        "\x5d\x8f\xd4\xc3\xa0\x5b";

    static const unsigned char dmp1[] =
        "\x05\x7c\x9e\x1c\xbd\x90\x25\xe7\x40\x86\xf5\xa8\x3b\x7a\x3f\x99"
        "\x56\x95\x60\x3a\x7b\x95\x4b\xb8\xa0\xd7\xa5\xf1\xcc\xdc\x5f\xb5"
        "\x8c\xf4\x62\x95\x54\xed\x2e\x12\x62\xc2\xe8\xf6\xde\xce\xed\x8e"
        "\x77\x6d\xc0\x40\x25\x74\xb3\x5a\x2d\xaa\xe1\xac\x11\xcb\xe2\x2f"
        "\x0a\x51\x23\x1e\x47\xb2\x05\x88\x02\xb2\x0f\x4b\xf0\x67\x30\xf0"
        "\x0f\x6e\xef\x5f\xf7\xe7";

    static const unsigned char dmq1[] =
        "\x01\xa5\x6b\xbc\xcd\xe3\x0e\x46\xc6\x72\xf5\x04\x56\x28\x01\x22"
        "\x58\x74\x5d\xbc\x1c\x3c\x29\x41\x49\x6c\x81\x5c\x72\xe2\xf7\xe5"
        "\xa3\x8e\x58\x16\xe0\x0e\x37\xac\x1f\xbb\x75\xfd\xaf\xe7\xdf\xe9"
        "\x1f\x70\xa2\x8f\x52\x03\xc0\x46\xd9\xf9\x96\x63\x00\x27\x7e\x5f"
        "\x38\x60\xd6\x6b\x61\xe2\xaf\xbe\xea\x58\xd3\x9d\xbc\x75\x03\x8d"
        "\x42\x65\xd6\x6b\x85\x97";

    static const unsigned char iqmp[] =
        "\x03\xa1\x8b\x80\xe4\xd8\x87\x25\x17\x5d\xcc\x8d\xa9\x8a\x22\x2b"
        "\x6c\x15\x34\x6f\x80\xcc\x1c\x44\x04\x68\xbc\x03\xcd\x95\xbb\x69"
        "\x37\x61\x48\xb4\x23\x13\x08\x16\x54\x6a\xa1\x7c\xf5\xd4\x3a\xe1"
        "\x4f\xa4\x0c\xf5\xaf\x80\x85\x27\x06\x0d\x70\xc0\xc5\x19\x28\xfe"
        "\xee\x8e\x86\x21\x98\x8a\x37\xb7\xe5\x30\x25\x70\x93\x51\x2d\x49"
        "\x85\x56\xb3\x0c\x2b\x96";

    static const unsigned char ex_prime[] =
        "\x03\x89\x22\xa0\xb7\x3a\x91\xcb\x5e\x0c\xfd\x73\xde\xa7\x38\xa9"
        "\x47\x43\xd6\x02\xbf\x2a\xb9\x3c\x48\xf3\x06\xd6\x58\x35\x50\x56"
        "\x16\x5c\x34\x9b\x61\x87\xc8\xaa\x0a\x5d\x8a\x0a\xcd\x9c\x41\xd9"
        "\x96\x24\xe0\xa9\x9b\x26\xb7\xa8\x08\xc9\xea\xdc\xa7\x15\xfb\x62"
        "\xa0\x2d\x90\xe6\xa7\x55\x6e\xc6\x6c\xff\xd6\x10\x6d\xfa\x2e\x04"
        "\x50\xec\x5c\x66\xe4\x05";

    static const unsigned char ex_exponent[] =
        "\x02\x0a\xcd\xc3\x82\xd2\x03\xb0\x31\xac\xd3\x20\x80\x34\x9a\x57"
        "\xbc\x60\x04\x57\x25\xd0\x29\x9a\x16\x90\xb9\x1c\x49\x6a\xd1\xf2"
        "\x47\x8c\x0e\x9e\xc9\x20\xc2\xd8\xe4\x8f\xce\xd2\x1a\x9c\xec\xb4"
        "\x1f\x33\x41\xc8\xf5\x62\xd1\xa5\xef\x1d\xa1\xd8\xbd\x71\xc6\xf7"
        "\xda\x89\x37\x2e\xe2\xec\x47\xc5\xb8\xe3\xb4\xe3\x5c\x82\xaa\xdd"
        "\xb7\x58\x2e\xaf\x07\x79";

    static const unsigned char ex_coefficient[] =
        "\x00\x9c\x09\x88\x9b\xc8\x57\x08\x69\x69\xab\x2d\x9e\x29\x1c\x3c"
        "\x6d\x59\x33\x12\x0d\x2b\x09\x2e\xaf\x01\x2c\x27\x01\xfc\xbd\x26"
        "\x13\xf9\x2d\x09\x22\x4e\x49\x11\x03\x82\x88\x87\xf4\x43\x1d\xac"
        "\xca\xec\x86\xf7\x23\xf1\x64\xf3\xf5\x81\xf0\x37\x36\xcf\x67\xff"
        "\x1a\xff\x7a\xc7\xf9\xf9\x67\x2d\xa0\x9d\x61\xf8\xf6\x47\x5c\x2f"
        "\xe7\x66\xe8\x3c\x3a\xe8";

    BIGNUM **pris = NULL, **exps = NULL, **coeffs = NULL;
    int rv = 256; /* public key length */

    if (!TEST_int_eq(RSA_set0_key(key,
                                  BN_bin2bn(n, sizeof(n) - 1, NULL),
                                  BN_bin2bn(e, sizeof(e) - 1, NULL),
                                  BN_bin2bn(d, sizeof(d) - 1, NULL)), 1))
        goto err;

    if (!TEST_int_eq(RSA_set0_factors(key,
                                      BN_bin2bn(p, sizeof(p) - 1, NULL),
                                      BN_bin2bn(q, sizeof(q) - 1, NULL)), 1))
        goto err;

    if (!TEST_int_eq(RSA_set0_crt_params(key,
                                         BN_bin2bn(dmp1, sizeof(dmp1) - 1, NULL),
                                         BN_bin2bn(dmq1, sizeof(dmq1) - 1, NULL),
                                         BN_bin2bn(iqmp, sizeof(iqmp) - 1,
                                                 NULL)), 1))
        return 0;

    pris = OPENSSL_zalloc(sizeof(BIGNUM *));
    exps = OPENSSL_zalloc(sizeof(BIGNUM *));
    coeffs = OPENSSL_zalloc(sizeof(BIGNUM *));
    if (!TEST_ptr(pris) || !TEST_ptr(exps) || !TEST_ptr(coeffs))
        goto err;

    pris[0] = BN_bin2bn(ex_prime, sizeof(ex_prime) - 1, NULL);
    exps[0] = BN_bin2bn(ex_exponent, sizeof(ex_exponent) - 1, NULL);
    coeffs[0] = BN_bin2bn(ex_coefficient, sizeof(ex_coefficient) - 1, NULL);
    if (!TEST_ptr(pris[0]) || !TEST_ptr(exps[0]) || !TEST_ptr(coeffs[0]))
        goto err;

    if (!TEST_true(RSA_set0_multi_prime_params(key, pris, exps,
                   coeffs, NUM_EXTRA_PRIMES)))
        goto err;

ret:
    OPENSSL_free(pris);
    OPENSSL_free(exps);
    OPENSSL_free(coeffs);
    return rv;
err:
    if (pris != NULL)
        BN_free(pris[0]);
    if (exps != NULL)
        BN_free(exps[0]);
    if (coeffs != NULL)
        BN_free(coeffs[0]);
    rv = 0;
    goto ret;
}

#endif
