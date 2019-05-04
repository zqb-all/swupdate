/*
 * (C) Copyright 2019
 * Stefano Babic, DENX Software Engineering, sbabic@denx.de.
 *
 * SPDX-License-Identifier:     GPL-2.0-or-later
 */

#ifndef _SWUPDATE_VERIFY_H
#define _SWUPDATE_VERIFY_H

#include <openssl/evp.h>

struct swupdate_digest;
int dgst_init(struct swupdate_digest *dgst, const EVP_MD *md);

#if defined(CONFIG_SIGALG_RAWRSA)
EVP_PKEY *load_pubkey(const char *file);
#endif

#ifdef CONFIG_SIGALG_CMS
#include <openssl/cms.h>
int check_code_sign(const X509_PURPOSE *xp, const X509 *crt, int ca);
X509_STORE *load_cert_chain(const char *file);
#endif

#endif
