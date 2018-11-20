#include <fcntl.h>

#ifndef O_BINARY
# ifdef _O_BINARY
#   define O_BINARY _O_BINARY
# endif
#endif

#ifndef O_BINARY
# define O_BINARY 0
#endif
