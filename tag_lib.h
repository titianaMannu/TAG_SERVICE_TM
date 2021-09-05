//
// Created by tiziana on 24/08/21.
//

#ifndef SOA_PROJECT_TM_TAG_LIB_H
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/ipc.h>
#define SOA_PROJECT_TM_TAG_LIB_H

#endif //SOA_PROJECT_TM_TAG_LIB_H

/*change those values by check dmsg once the module is inserted */
#define GET_NR 134
#define SND_NR 156
#define RCV_NR 174
#define CTL_NR 177

static inline int tag_get(int key, int command, int permission) {
    errno  = 0;
    return syscall(GET_NR, key, command, permission);
}

static inline int tag_send(int tag, int level, char *buffer, size_t size) {
    errno  = 0;
    return syscall(SND_NR, tag, level, buffer, size);
}

static inline int tag_receive(int tag, int level, char *buffer, size_t size) {
    errno  = 0;
    return syscall(RCV_NR, tag, level, buffer, size);
}

static inline int tag_ctl(int tag, int command) {
    errno  = 0;
    return syscall(CTL_NR, tag, command);
}
