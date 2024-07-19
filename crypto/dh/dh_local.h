/*
 * Copyright 2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <openssl/dh.h>
#include "internal/refcount.h"

struct dh_st {
    /*
     * This first argument is used to pick up errors when a DH is passed
     * instead of a EVP_PKEY
     */
    int pad;
    int version;
    BIGNUM *p;
    BIGNUM *g;
    int32_t length;             /* optional */
    BIGNUM *pub_key;            /* g^x % p */
    BIGNUM *priv_key;           /* x */
    int flags;
    BN_MONT_CTX *method_mont_p;
    /* Place holders if we want to do X9.42 DH */
    BIGNUM *q;
    BIGNUM *j;
    unsigned char *seed;
    int seedlen;
    BIGNUM *counter;
    CRYPTO_REF_COUNT references;
    CRYPTO_EX_DATA ex_data;
    const DH_METHOD *meth;
    ENGINE *engine;
    CRYPTO_RWLOCK *lock;
    int nid;
};

struct dh_method {
    char *name;
    /* Methods here */
    int (*generate_key) (DH *dh);
    int (*compute_key) (unsigned char *key, const BIGNUM *pub_key, DH *dh);

    /* Can be null */
    int (*bn_mod_exp) (const DH *dh, BIGNUM *r, const BIGNUM *a,
                       const BIGNUM *p, const BIGNUM *m, BN_CTX *ctx,
                       BN_MONT_CTX *m_ctx);
    int (*init) (DH *dh);
    int (*finish) (DH *dh);
    int flags;
    char *app_data;
    /* If this is non-NULL, it will be used to generate parameters */
    int (*generate_params) (DH *dh, int prime_len, int generator,
                            BN_GENCB *cb);
};

void dh_cache_nid(DH *dh);
/* Obtain known q value based on nid or p */
int dh_get_known_q(const DH *dh, BIGNUM **q);
/* FIPS mode only check which requires nid set and looks up q based on it. */
int dh_check_pub_key_full(const DH *dh, const BIGNUM *pub_key, int *ret);
int dh_check_pairwise(const DH *dh);
int dh_generate_public_key(BN_CTX *ctx, const DH *dh,
                           const BIGNUM *priv_key, BIGNUM *pub_key);
