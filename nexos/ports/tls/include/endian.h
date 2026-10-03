#ifndef NEXOS_ENDIAN_H
#define NEXOS_ENDIAN_H

/* NexOS currently targets little-endian x86_64.  libnsfb uses these names
 * only to select its packed-pixel byte order. */
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#endif
#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN 4321
#endif
#ifndef __BYTE_ORDER
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif
#ifndef __ORDER_LITTLE_ENDIAN__
#define __ORDER_LITTLE_ENDIAN__ __LITTLE_ENDIAN
#endif
#ifndef __ORDER_BIG_ENDIAN__
#define __ORDER_BIG_ENDIAN__ __BIG_ENDIAN
#endif
#ifndef __BYTE_ORDER__
#define __BYTE_ORDER__ __LITTLE_ENDIAN
#endif

#endif
