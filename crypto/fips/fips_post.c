/* ====================================================================
 * Copyright (c) 2011 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#define OPENSSL_FIPSAPI

#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/fips_rand.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/hmac.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/evp.h>
#include <string.h>
#include <limits.h>

#ifdef OPENSSL_FIPS

/* Power on self test (POST) support functions */

# include <openssl/fips.h>
# include "crypto/fips.h"
# include "crypto/rand.h"
# include "fips_locl.h"

char* FIPS_show_version(void)
{
    return "Rocky Linux 8 OpenSSL Cryptographic Module Version Rocky8.20230620";
}

/* POST notification callback */

int (*fips_post_cb)(int op, int id, int subid, void *ex);

void FIPS_post_set_callback(
	int (*post_cb)(int op, int id, int subid, void *ex))
	{
	fips_post_cb = post_cb;
	}

/* POST status: i.e. status of all tests */
#define FIPS_POST_STATUS_NOT_STARTED	0
#define FIPS_POST_STATUS_OK		1
#define FIPS_POST_STATUS_RUNNING	2
#define FIPS_POST_STATUS_FAILED		-1
static int post_status = 0;
/* Set to 1 if any test failed */
static int post_failure = 0;

/* All tests started */

int fips_post_begin(void)
	{
	post_failure = 0;
	post_status = FIPS_POST_STATUS_NOT_STARTED;
	if (fips_post_cb)
		if (!fips_post_cb(FIPS_POST_BEGIN, 0, 0, NULL))
			return 0;
	post_status = FIPS_POST_STATUS_RUNNING;
	return 1;
	}

void fips_post_end(void)
	{
	if (post_failure)
		{
		post_status = FIPS_POST_STATUS_FAILED;
		if(fips_post_cb)
			fips_post_cb(FIPS_POST_END, 0, 0, NULL);
		}
	else
		{
		post_status = FIPS_POST_STATUS_OK;
		if (fips_post_cb)
			fips_post_cb(FIPS_POST_END, 1, 0, NULL);
		}
	}

/* A self test started */
int fips_post_started(int id, int subid, void *ex)
	{
	if (fips_post_cb)
		return fips_post_cb(FIPS_POST_STARTED, id, subid, ex);
	return 1;
	}
/* A self test passed successfully */
int fips_post_success(int id, int subid, void *ex)
	{
	if (fips_post_cb)
		return fips_post_cb(FIPS_POST_SUCCESS, id, subid, ex);
	return 1;
	}
/* A self test failed */
int fips_post_failed(int id, int subid, void *ex)
	{
	post_failure = 1;
	if (fips_post_cb)
		return fips_post_cb(FIPS_POST_FAIL, id, subid, ex);
	return 1;
	}
/* Indicate if a self test failure should be induced */
int fips_post_corrupt(int id, int subid, void *ex)
	{
	if (fips_post_cb)
		return fips_post_cb(FIPS_POST_CORRUPT, id, subid, ex);
	return 1;
	}
/* Note: if selftests running return status OK so their operation is
 * not interrupted. This will only happen while selftests are actually
 * running so will not interfere with normal operation.
 */
int fips_post_status(void)
	{
	return post_status > 0 ? 1 : 0;
	}

/* Run all selftests */
int FIPS_selftest(void)
{
    int rv = 1;
    fips_post_begin();
    if (!rand_drbg_selftest()) {
        FIPSerr(FIPS_F_FIPS_SELFTEST, FIPS_R_TEST_FAILURE);
        ERR_add_error_data(2, "Type=", "rand_drbg_selftest");
        rv = 0;
    }
    if (!FIPS_selftest_sha1())
        rv = 0;
    if (!FIPS_selftest_sha2())
        rv = 0;
    if (!FIPS_selftest_sha3())
        rv = 0;
    if (!FIPS_selftest_hmac())
        rv = 0;
    if (!FIPS_selftest_cmac())
        rv = 0;
    if (!FIPS_selftest_aes())
        rv = 0;
    if (!FIPS_selftest_aes_ccm())
        rv = 0;
    if (!FIPS_selftest_aes_gcm())
        rv = 0;
    if (!FIPS_selftest_aes_xts())
        rv = 0;
    if (!FIPS_selftest_rsa())
        rv = 0;
    if (!FIPS_selftest_ecdsa())
        rv = 0;
    if (!FIPS_selftest_dsa())
        rv = 0;
    if (!FIPS_selftest_dh())
        rv = 0;
    if (!FIPS_selftest_ecdh())
        rv = 0;
    if (!FIPS_selftest_kdf())
        rv = 0;
    
    fips_post_end();
    return rv;
}

/* Generalized public key test routine. Signs and verifies the data
 * supplied in tbs using mesage digest md and setting option digest
 * flags md_flags. If the 'kat' parameter is not NULL it will
 * additionally check the signature matches it: a known answer test
 * The string "fail_str" is used for identification purposes in case
 * of failure. If "pkey" is NULL just perform a message digest check.
 */

int fips_pkey_signature_test(int id, EVP_PKEY *pkey,
                             const unsigned char *tbs, int tbslen,
                             const unsigned char *kat, unsigned int katlen,
                             const EVP_MD *digest, unsigned int flags,
                             const char *fail_str)
{
    int subid;
    int ret = 0;
    unsigned char sigtmp[256], *sig = sigtmp;
    size_t siglen = sizeof(sigtmp);
    EVP_MD_CTX *mctx;
    EVP_PKEY_CTX *pctx;

    if (digest == NULL)
        digest = EVP_sha256();

    subid = EVP_MD_type(digest);

    if (!fips_post_started(id, subid, pkey))
		return 1;

    mctx = EVP_MD_CTX_new();

    if ((EVP_PKEY_id(pkey) == EVP_PKEY_RSA)
        && (RSA_size(EVP_PKEY_get0_RSA(pkey)) > sizeof(sigtmp))) {
        sig = OPENSSL_malloc(RSA_size(EVP_PKEY_get0_RSA(pkey)));
        siglen = RSA_size(EVP_PKEY_get0_RSA(pkey));
    }
    if (!sig || ! mctx) {
        EVP_MD_CTX_free(mctx);
        FIPSerr(FIPS_F_FIPS_PKEY_SIGNATURE_TEST, ERR_R_MALLOC_FAILURE);
        return 0;
    }

    if (tbslen == -1)
        tbslen = strlen((char *)tbs);

    if (EVP_DigestSignInit(mctx, &pctx, digest, NULL, pkey) <= 0)
        goto error;

    if (flags == EVP_MD_CTX_FLAG_PAD_PSS) {
        EVP_PKEY_CTX_set_rsa_padding(pctx, RSA_PKCS1_PSS_PADDING);
        EVP_PKEY_CTX_set_rsa_pss_saltlen(pctx, 0);
    }

    if (EVP_DigestSignUpdate(mctx, tbs, tbslen) <= 0)
        goto error;

    if (!fips_post_corrupt(id, subid, pkey)) {
		if (!EVP_DigestSignUpdate(mctx, tbs, 1))
			goto error;
	}

    if (EVP_DigestSignFinal(mctx, sig, &siglen) <= 0)
        goto error;

    if (kat && ((siglen != katlen) || memcmp(kat, sig, katlen)))
        goto error;

    if (EVP_DigestVerifyInit(mctx, &pctx, digest, NULL, pkey) <= 0)
        goto error;

    if (flags == EVP_MD_CTX_FLAG_PAD_PSS) {
        EVP_PKEY_CTX_set_rsa_padding(pctx, RSA_PKCS1_PSS_PADDING);
        EVP_PKEY_CTX_set_rsa_pss_saltlen(pctx, 0);
    }

    if (EVP_DigestVerifyUpdate(mctx, tbs, tbslen) <= 0)
        goto error;

    ret = EVP_DigestVerifyFinal(mctx, sig, siglen);

 error:
    if (sig != sigtmp)
        OPENSSL_free(sig);
    EVP_MD_CTX_free(mctx);
    if (ret <= 0) {
        FIPSerr(FIPS_F_FIPS_PKEY_SIGNATURE_TEST, FIPS_R_TEST_FAILURE);
        if (fail_str)
            ERR_add_error_data(2, "Type=", fail_str);
        fips_post_failed(id, subid, pkey);
        return 0;
    }
    return fips_post_success(id, subid, pkey);
}

/* Generalized symmetric cipher test routine. Encrypt data, verify result
 * against known answer, decrypt and compare with original plaintext.
 */

int fips_cipher_test(int id, EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher,
                     const unsigned char *key,
                     const unsigned char *iv,
                     const unsigned char *plaintext,
                     const unsigned char *ciphertext, int len)
{
    unsigned char pltmp[FIPS_MAX_CIPHER_TEST_SIZE];
    unsigned char citmp[FIPS_MAX_CIPHER_TEST_SIZE];
    int subid = EVP_CIPHER_nid(cipher);
    int rv = 0;

    OPENSSL_assert(len <= FIPS_MAX_CIPHER_TEST_SIZE);
    memset(pltmp, 0, FIPS_MAX_CIPHER_TEST_SIZE);
    memset(citmp, 0, FIPS_MAX_CIPHER_TEST_SIZE);

    if (!fips_post_started(id, subid, NULL))
		return 1;
    if (EVP_CipherInit_ex(ctx, cipher, NULL, key, iv, 1) <= 0)
        goto error; //return 0;
    if (EVP_Cipher(ctx, citmp, plaintext, len) <= 0)
        goto error; //return 0;
    if (memcmp(citmp, ciphertext, len))
        goto error; //return 0;
    if (!fips_post_corrupt(id, subid, NULL)) {
			citmp[0] ^= 0x1;
    }
    if (EVP_CipherInit_ex(ctx, cipher, NULL, key, iv, 0) <= 0)
        goto error; //return 0;
    if (EVP_Cipher(ctx, pltmp, citmp, len) <= 0)
        goto error; //return 0;
    if (memcmp(pltmp, plaintext, len))
        goto error; //return 0;
    rv = 1;
	error:
	if (rv == 0) {
		fips_post_failed(id, subid, NULL);
		return 0;
	}
	return fips_post_success(id, subid, NULL);
}
#endif
