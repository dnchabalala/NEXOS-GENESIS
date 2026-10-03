/* Minimal Mbed TLS configuration for the freestanding NexOS kernel. */
#ifndef NEXOS_MBEDTLS_CONFIG_H
#define NEXOS_MBEDTLS_CONFIG_H

/* Keep the upstream default feature set, then remove host-only services. */
#include <mbedtls/mbedtls_config.h>

#undef MBEDTLS_FS_IO
#undef MBEDTLS_NET_C
#undef MBEDTLS_TIMING_C
#undef MBEDTLS_THREADING_C
#undef MBEDTLS_DEBUG_C
#undef MBEDTLS_SELF_TEST
#undef MBEDTLS_PSA_ITS_FILE_C
#undef MBEDTLS_PSA_CRYPTO_STORAGE_C
#undef MBEDTLS_AESNI_C
#undef MBEDTLS_AESCE_C
#undef MBEDTLS_PADLOCK_C

/* The adapter supplies the only strong source (NexOS RDRAND/getrandom). */
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_TEST_PLATFORM_IS_NOT_UNIXLIKE

#define MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
#define MBEDTLS_PLATFORM_STD_MEM_HDR "nexos_tls_platform.h"
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_CALLOC_MACRO nexos_tls_calloc
#define MBEDTLS_PLATFORM_FREE_MACRO nexos_tls_free_mem
#define MBEDTLS_PLATFORM_STD_SNPRINTF nexos_tls_snprintf
#define MBEDTLS_PLATFORM_SNPRINTF_ALT
#define MBEDTLS_PLATFORM_VSNPRINTF_MACRO nexos_tls_vsnprintf
#define MBEDTLS_PLATFORM_PRINTF_MACRO nexos_tls_printf

/* Certificate validity checks use the RTC-backed NexOS clock. */
#define MBEDTLS_PLATFORM_TIME_TYPE_MACRO int64_t
#define MBEDTLS_PLATFORM_TIME_MACRO nexos_tls_time
#define MBEDTLS_PLATFORM_MS_TIME_ALT
#define MBEDTLS_PLATFORM_GMTIME_R_ALT

#endif
