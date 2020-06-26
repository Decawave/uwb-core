#include <linux/types.h>

#ifndef _INITYPES_H_
#define _INTTYPES_H_

#define __PRI_8_LENGTH_MODIFIER__ "hh"
#define __PRI_64_LENGTH_MODIFIER__ "ll"
#define __SCN_64_LENGTH_MODIFIER__ "ll"
#define __PRI_MAX_LENGTH_MODIFIER__ "j"
#define __SCN_MAX_LENGTH_MODIFIER__ "j"

#define PRId8         __PRI_8_LENGTH_MODIFIER__ "d"
#define PRIi8         __PRI_8_LENGTH_MODIFIER__ "i"
#define PRIo8         __PRI_8_LENGTH_MODIFIER__ "o"
#define PRIu8         __PRI_8_LENGTH_MODIFIER__ "u"
#define PRIx8         __PRI_8_LENGTH_MODIFIER__ "x"
#define PRIX8         __PRI_8_LENGTH_MODIFIER__ "X"

#define PRId16        "hd"
#define PRIi16        "hi"
#define PRIo16        "ho"
#define PRIu16        "hu"
#define PRIx16        "hx"
#define PRIX16        "hX"

#define PRId32        "d"
#define PRIi32        "i"
#define PRIo32        "o"
#define PRIu32        "u"
#define PRIx32        "x"
#define PRIX32        "X"

#define PRId64        __PRI_64_LENGTH_MODIFIER__ "d"
#define PRIi64        __PRI_64_LENGTH_MODIFIER__ "i"
#define PRIo64        __PRI_64_LENGTH_MODIFIER__ "o"
#define PRIu64        __PRI_64_LENGTH_MODIFIER__ "u"
#define PRIx64        __PRI_64_LENGTH_MODIFIER__ "x"
#define PRIX64        __PRI_64_LENGTH_MODIFIER__ "X"

#endif
