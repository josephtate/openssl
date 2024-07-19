/*
 * Copyright 1995-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <stdio.h>
#include <string.h>
#include <openssl/blowfish.h>
#include "bf_local.h"
#include "bf_pi.h"

void BF_set_key(BF_KEY *key, int len, const unsigned char *data)
{
    int i;
    BF_LONG *p, ri, in[2];
    const unsigned char *d, *end;

    if (FIPS_mode()) {
        FIPSerr(ERR_LIB_FIPS, FIPS_R_NON_FIPS_METHOD);
        OpenSSLDie(__FILE__, __LINE__, "FATAL FIPS Unapproved algorithm called");
        return;
    }

    memcpy(key, &bf_init, sizeof(BF_KEY));
    p = key->P;

    if (len > ((BF_ROUNDS + 2) * 4))
        len = (BF_ROUNDS + 2) * 4;

    d = data;
    end = &(data[len]);
    for (i = 0; i < (BF_ROUNDS + 2); i++) {
        ri = *(d++);
        if (d >= end)
            d = data;

        ri <<= 8;
        ri |= *(d++);
        if (d >= end)
            d = data;

        ri <<= 8;
        ri |= *(d++);
        if (d >= end)
            d = data;

        ri <<= 8;
        ri |= *(d++);
        if (d >= end)
            d = data;

        p[i] ^= ri;
    }

    in[0] = 0L;
    in[1] = 0L;
    for (i = 0; i < (BF_ROUNDS + 2); i += 2) {
        BF_encrypt(in, key);
        p[i] = in[0];
        p[i + 1] = in[1];
    }

    p = key->S;
    for (i = 0; i < 4 * 256; i += 2) {
        BF_encrypt(in, key);
        p[i] = in[0];
        p[i + 1] = in[1];
    }
}
