/*
 * Copyright 2006-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

# include "openssl/opensslconf.h"

#ifdef OPENSSL_FIPS
# include "openssl/fips.h"
# include "openssl/err.h"
#endif

#include <openssl/camellia.h>
#include "cmll_local.h"

void Camellia_ecb_encrypt(const unsigned char *in, unsigned char *out,
                          const CAMELLIA_KEY *key, const int enc)
{
    if (FIPS_mode()) {
        FIPSerr(ERR_LIB_FIPS, FIPS_R_NON_FIPS_METHOD);
        OpenSSLDie(__FILE__, __LINE__, "FATAL FIPS Unapproved algorithm called");
        return;
    }

    if (CAMELLIA_ENCRYPT == enc)
        Camellia_encrypt(in, out, key);
    else
        Camellia_decrypt(in, out, key);
}
