#ifdef HAVE_ENDIAN_H
#include <endian.h>
#endif
#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#endif


#ifndef BYTE_ORDER
# if defined(__BYTE_ORDER__)
#   define BYTE_ORDER __BYTE_ORDER__
# elif defined(__BYTE_ORDER)
#   define BYTE_ORDER __BYTE_ORDER
# elif defined(_BYTE_ORDER)
#   define BYTE_ORDER _BYTE_ORDER
# endif
#endif

#ifndef __ORDER_LITTLE_ENDIAN__
#  define __ORDER_LITTLE_ENDIAN__ 1234
#endif
#ifndef __ORDER_BIG_ENDIAN__
#  define __ORDER_BIG_ENDIAN__ 4321
#endif

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#  define cpu_to_be32(x) (x)
#  define cpu_to_be16(x) (x)
#  define be32_to_cpu(x) (x)
#  define be16_to_cpu(x) (x)
#else
#  define cpu_to_be32(x) ntohl(x)
#  define cpu_to_be16(x) ntohs(x)
#  define be32_to_cpu(x) htonl(x)
#  define be16_to_cpu(x) htons(x)
#endif
