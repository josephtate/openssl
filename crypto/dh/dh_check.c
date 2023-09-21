/*
 * Copyright 1995-2019 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <stdio.h>
#include "internal/cryptlib.h"
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include "dh_local.h"
#include "openssl/fips.h"

# define DH_NUMBER_ITERATIONS_FOR_PRIME 64

/*-
 * Check that p and g are suitable enough
 *
 * p is odd
 * 1 < g < p - 1
 */
int DH_check_params_ex(const DH *dh)
{
    int errflags = 0;

    if (!DH_check_params(dh, &errflags))
        return 0;

    if ((errflags & DH_CHECK_P_NOT_PRIME) != 0)
        DHerr(DH_F_DH_CHECK_PARAMS_EX, DH_R_CHECK_P_NOT_PRIME);
    if ((errflags & DH_NOT_SUITABLE_GENERATOR) != 0)
        DHerr(DH_F_DH_CHECK_PARAMS_EX, DH_R_NOT_SUITABLE_GENERATOR);

    return errflags == 0;
}

int DH_check_params(const DH *dh, int *ret)
{
    int ok = 0;
    BIGNUM *tmp = NULL;
    BN_CTX *ctx = NULL;

    if (FIPS_mode()) {
        int nid = dh->nid == NID_undef ? DH_get_nid(dh) : dh->nid;

        return nid != NID_undef;
    }

    *ret = 0;
    ctx = BN_CTX_new();
    if (ctx == NULL)
        goto err;
    BN_CTX_start(ctx);
    tmp = BN_CTX_get(ctx);
    if (tmp == NULL)
        goto err;

    if (!BN_is_odd(dh->p))
        *ret |= DH_CHECK_P_NOT_PRIME;
    if (BN_is_negative(dh->g) || BN_is_zero(dh->g) || BN_is_one(dh->g))
        *ret |= DH_NOT_SUITABLE_GENERATOR;
    if (BN_copy(tmp, dh->p) == NULL || !BN_sub_word(tmp, 1))
        goto err;
    if (BN_cmp(dh->g, tmp) >= 0)
        *ret |= DH_NOT_SUITABLE_GENERATOR;

    ok = 1;
 err:
    BN_CTX_end(ctx);
    BN_CTX_free(ctx);
    return ok;
}

/*-
 * Check that p is a safe prime and
 * g is a suitable generator.
 */
int DH_check_ex(const DH *dh)
{
    int errflags = 0;

    if (!DH_check(dh, &errflags))
        return 0;

    if ((errflags & DH_NOT_SUITABLE_GENERATOR) != 0)
        DHerr(DH_F_DH_CHECK_EX, DH_R_NOT_SUITABLE_GENERATOR);
    if ((errflags & DH_CHECK_Q_NOT_PRIME) != 0)
        DHerr(DH_F_DH_CHECK_EX, DH_R_CHECK_Q_NOT_PRIME);
    if ((errflags & DH_CHECK_INVALID_Q_VALUE) != 0)
        DHerr(DH_F_DH_CHECK_EX, DH_R_CHECK_INVALID_Q_VALUE);
    if ((errflags & DH_CHECK_INVALID_J_VALUE) != 0)
        DHerr(DH_F_DH_CHECK_EX, DH_R_CHECK_INVALID_J_VALUE);
    if ((errflags & DH_UNABLE_TO_CHECK_GENERATOR) != 0)
        DHerr(DH_F_DH_CHECK_EX, DH_R_UNABLE_TO_CHECK_GENERATOR);
    if ((errflags & DH_CHECK_P_NOT_PRIME) != 0)
        DHerr(DH_F_DH_CHECK_EX, DH_R_CHECK_P_NOT_PRIME);
    if ((errflags & DH_CHECK_P_NOT_SAFE_PRIME) != 0)
        DHerr(DH_F_DH_CHECK_EX, DH_R_CHECK_P_NOT_SAFE_PRIME);

    return errflags == 0;
}

/* Note: according to documentation - this only checks the params */
int DH_check(const DH *dh, int *ret)
{
    int ok = 0, r;
    BN_CTX *ctx = NULL;
    BIGNUM *t1 = NULL, *t2 = NULL;

    if (!DH_check_params(dh, ret))
        return 0;

    if (FIPS_mode()) /* we allow only well-known params */
        return 1;

    ctx = BN_CTX_new();
    if (ctx == NULL)
        goto err;
    BN_CTX_start(ctx);
    t1 = BN_CTX_get(ctx);
    t2 = BN_CTX_get(ctx);
    if (t2 == NULL)
        goto err;

    if (dh->q) {
        if (BN_cmp(dh->g, BN_value_one()) <= 0)
            *ret |= DH_NOT_SUITABLE_GENERATOR;
        else if (BN_cmp(dh->g, dh->p) >= 0)
            *ret |= DH_NOT_SUITABLE_GENERATOR;
        else {
            /* Check g^q == 1 mod p */
            if (!BN_mod_exp(t1, dh->g, dh->q, dh->p, ctx))
                goto err;
            if (!BN_is_one(t1))
                *ret |= DH_NOT_SUITABLE_GENERATOR;
        }
        r = BN_is_prime_ex(dh->q, DH_NUMBER_ITERATIONS_FOR_PRIME, ctx, NULL);
        if (r < 0)
            goto err;
        if (!r)
            *ret |= DH_CHECK_Q_NOT_PRIME;
        /* Check p == 1 mod q  i.e. q divides p - 1 */
        if (!BN_div(t1, t2, dh->p, dh->q, ctx))
            goto err;
        if (!BN_is_one(t2))
            *ret |= DH_CHECK_INVALID_Q_VALUE;
        if (dh->j && BN_cmp(dh->j, t1))
            *ret |= DH_CHECK_INVALID_J_VALUE;
    }

    r = BN_is_prime_ex(dh->p, DH_NUMBER_ITERATIONS_FOR_PRIME, ctx, NULL);
    if (r < 0)
        goto err;
    if (!r)
        *ret |= DH_CHECK_P_NOT_PRIME;
    else if (!dh->q) {
        if (!BN_rshift1(t1, dh->p))
            goto err;
        r = BN_is_prime_ex(t1, DH_NUMBER_ITERATIONS_FOR_PRIME, ctx, NULL);
        if (r < 0)
            goto err;
        if (!r)
            *ret |= DH_CHECK_P_NOT_SAFE_PRIME;
    }
    ok = 1;
 err:
    BN_CTX_end(ctx);
    BN_CTX_free(ctx);
    return ok;
}

int DH_check_pub_key_ex(const DH *dh, const BIGNUM *pub_key)
{
    int errflags = 0;

    if (!DH_check_pub_key(dh, pub_key, &errflags))
        return 0;

    if ((errflags & DH_CHECK_PUBKEY_TOO_SMALL) != 0)
        DHerr(DH_F_DH_CHECK_PUB_KEY_EX, DH_R_CHECK_PUBKEY_TOO_SMALL);
    if ((errflags & DH_CHECK_PUBKEY_TOO_LARGE) != 0)
        DHerr(DH_F_DH_CHECK_PUB_KEY_EX, DH_R_CHECK_PUBKEY_TOO_LARGE);
    if ((errflags & DH_CHECK_PUBKEY_INVALID) != 0)
        DHerr(DH_F_DH_CHECK_PUB_KEY_EX, DH_R_CHECK_PUBKEY_INVALID);

    return errflags == 0;
}

static int dh_check_pub_key_int(const DH *dh, const BIGNUM *q, const BIGNUM *pub_key, int *ret)
{
    int ok = 0;
    BIGNUM *tmp = NULL;
    BN_CTX *ctx = NULL;

    *ret = 0;
    ctx = BN_CTX_new();
    if (ctx == NULL)
        goto err;
    BN_CTX_start(ctx);
    tmp = BN_CTX_get(ctx);
    if (tmp == NULL || !BN_set_word(tmp, 1))
        goto err;
    if (BN_cmp(pub_key, tmp) <= 0)
        *ret |= DH_CHECK_PUBKEY_TOO_SMALL;
    if (BN_copy(tmp, dh->p) == NULL || !BN_sub_word(tmp, 1))
        goto err;
    if (BN_cmp(pub_key, tmp) >= 0)
        *ret |= DH_CHECK_PUBKEY_TOO_LARGE;

    if (q != NULL) {
        /* Check pub_key^q == 1 mod p */
        if (!BN_mod_exp(tmp, pub_key, q, dh->p, ctx))
            goto err;
        if (!BN_is_one(tmp))
            *ret |= DH_CHECK_PUBKEY_INVALID;
    }

    ok = 1;
 err:
    BN_CTX_end(ctx);
    BN_CTX_free(ctx);
    return ok;
}

int DH_check_pub_key(const DH *dh, const BIGNUM *pub_key, int *ret)
{
    return dh_check_pub_key_int(dh, dh->q, pub_key, ret);
}

int dh_check_pub_key_full(const DH *dh, const BIGNUM *pub_key, int *ret)
{
    BIGNUM *q = dh->q;    

    if (q == NULL) {
        if (dh_get_known_q(dh, &q) == 0) {
            *ret |= DH_CHECK_INVALID_Q_VALUE;
            return 0;
        }
    }

    return dh_check_pub_key_int(dh, q, pub_key, ret);
}

/*
 * FFC pairwise check from SP800-56A R3.
 *    Section 5.6.2.1.4 Owner Assurance of Pair-wise Consistency
 */
int dh_check_pairwise(const DH *dh)
{
    int ret = 0;
    BN_CTX *ctx = NULL;
    BIGNUM *pub_key = NULL;

    if (!fips_post_started(FIPS_TEST_PAIRWISE, EVP_PKEY_DH, dh))
        return 1;

    if (dh->p == NULL
        || dh->g == NULL
        || dh->priv_key == NULL
        || dh->pub_key == NULL)
        return 0;

    ctx = BN_CTX_new();
    if (ctx == NULL)
        goto err;
    pub_key = BN_new();
    if (pub_key == NULL)
        goto err;

    /* recalculate the public key = (g ^ priv) mod p */
    if (!dh_generate_public_key(ctx, dh, dh->priv_key, pub_key))
        goto err;
    
    if (!fips_post_corrupt(FIPS_TEST_PAIRWISE, EVP_PKEY_DH, dh)) {
		BN_clear(pub_key);
	}

    /* check it matches the existing pubic_key */
    ret = BN_cmp(pub_key, dh->pub_key) == 0;
err:    
    BN_free(pub_key);
    BN_CTX_free(ctx);
    if (ret <= 0) {
        fips_post_failed(FIPS_TEST_PAIRWISE, EVP_PKEY_DH, dh);
        return 0;
    }
    return fips_post_success(FIPS_TEST_PAIRWISE, EVP_PKEY_DH, dh);
}

