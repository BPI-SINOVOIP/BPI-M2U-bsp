#ifndef HEADER_SHA_H
#define HEADER_SHA_H

#include "e_os2.h"


#ifdef  __cplusplus
extern "C" {
#endif

#if defined(OPENSSL_NO_SHA) || (defined(OPENSSL_NO_SHA0) && defined(OPENSSL_NO_SHA1))
#error SHA is disabled.
#endif

#if defined(OPENSSL_FIPS)
#define FIPS_SHA_SIZE_T size_t
#endif

/*
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * ! SHA_LONG has to be at least 32 bits wide. If it's wider, then !
 * ! SHA_LONG_LOG2 has to be defined along.                        !
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

#if defined(OPENSSL_SYS_WIN16) || defined(__LP32__)
#define SHA_LONG unsigned long
#elif defined(OPENSSL_SYS_CRAY) || defined(__ILP64__)
#define SHA_LONG unsigned long
#define SHA_LONG_LOG2 3
#else
#define SHA_LONG unsigned int
#endif

#define SHA_LBLOCK	16
#define SHA_CBLOCK	(SHA_LBLOCK*4)	/* SHA treats input data as a
					 * contiguous array of 32 bit
					 * wide big-endian values. */
#define SHA_LAST_BLOCK  (SHA_CBLOCK-8)
#define SHA_DIGEST_LENGTH 20

typedef struct SHAstate_st
	{
	SHA_LONG h0,h1,h2,h3,h4;
	SHA_LONG Nl,Nh;
	SHA_LONG data[SHA_LBLOCK];
	unsigned int num;
	} SHA_CTX;

#ifndef OPENSSL_NO_SHA0
#ifdef OPENSSL_FIPS
int private_SHA_Init(SHA_CTX *c);
#endif
int SHA_Init(SHA_CTX *c);
int SHA_Update(SHA_CTX *c, const void *data, size_t len);
int SHA_Final(unsigned char *md, SHA_CTX *c);
unsigned char *SHA(const unsigned char *d, size_t n, unsigned char *md);
void SHA_Transform(SHA_CTX *c, const unsigned char *data);
#endif
#ifndef OPENSSL_NO_SHA1
int SHA1_Init(SHA_CTX *c);
int SHA1_Update(SHA_CTX *c, const void *data, size_t len);
int SHA1_Final(unsigned char *md, SHA_CTX *c);
unsigned char *SHA1(const unsigned char *d, size_t n, unsigned char *md);
void SHA1_Transform(SHA_CTX *c, const unsigned char *data);
#endif

#define SHA256_CBLOCK	(SHA_LBLOCK*4)	/* SHA-256 treats input data as a
					 * contiguous array of 32 bit
					 * wide big-endian values. */
#define SHA224_DIGEST_LENGTH	28
#define SHA256_DIGEST_LENGTH	32

typedef struct SHA256state_st
	{
	SHA_LONG h[8];
	SHA_LONG Nl,Nh;
	SHA_LONG data[SHA_LBLOCK];
	unsigned int num,md_len;
	} SHA256_CTX;

#ifndef OPENSSL_NO_SHA256
int SHA224_Init(SHA256_CTX *c);
int SHA224_Update(SHA256_CTX *c, const void *data, size_t len);
int SHA224_Final(unsigned char *md, SHA256_CTX *c);
unsigned char *SHA224(const unsigned char *d, size_t n,unsigned char *md);
int SHA256_Init(SHA256_CTX *c);
int SHA256_Update(SHA256_CTX *c, const void *data, size_t len);
int SHA256_Final(unsigned char *md, SHA256_CTX *c);
unsigned char *SHA256(const unsigned char *d, size_t n,unsigned char *md);
void SHA256_Transform(SHA256_CTX *c, const unsigned char *data);
#endif

#define SHA384_DIGEST_LENGTH	48
#define SHA512_DIGEST_LENGTH	64

#ifndef OPENSSL_NO_SHA512
/*
 * Unlike 32-bit digest algorithms, SHA-512 *relies* on SHA_LONG64
 * being exactly 64-bit wide. See Implementation Notes in sha512.c
 * for further details.
 */
#define SHA512_CBLOCK	(SHA_LBLOCK*8)	/* SHA-512 treats input data as a
					 * contiguous array of 64 bit
					 * wide big-endian values. */
#if (defined(_WIN32) || defined(_WIN64)) && !defined(__MINGW32__)
#define SHA_LONG64 unsigned __int64
#define U64(C)     C##UI64
#elif defined(__arch64__)
#define SHA_LONG64 unsigned long
#define U64(C)     C##UL
#else
#define SHA_LONG64 unsigned long long
#define U64(C)     C##ULL
#endif

typedef struct SHA512state_st
	{
	SHA_LONG64 h[8];
	SHA_LONG64 Nl,Nh;
	union {
		SHA_LONG64	d[SHA_LBLOCK];
		unsigned char	p[SHA512_CBLOCK];
	} u;
	unsigned int num,md_len;
	} SHA512_CTX;
#endif

#ifndef OPENSSL_NO_SHA512
int SHA384_Init(SHA512_CTX *c);
int SHA384_Update(SHA512_CTX *c, const void *data, size_t len);
int SHA384_Final(unsigned char *md, SHA512_CTX *c);
unsigned char *SHA384(const unsigned char *d, size_t n,unsigned char *md);
int SHA512_Init(SHA512_CTX *c);
int SHA512_Update(SHA512_CTX *c, const void *data, size_t len);
int SHA512_Final(unsigned char *md, SHA512_CTX *c);
unsigned char *SHA512(const unsigned char *d, size_t n,unsigned char *md);
void SHA512_Transform(SHA512_CTX *c, const unsigned char *data);
#endif

#ifdef  __cplusplus
}
#endif

#endif
