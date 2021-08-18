#include <stdio.h>
#include <sys/ipc.h>

int main() {
    printf("%o\n", (IPC_CREAT | IPC_EXCL) ^ IPC_CREAT);
    printf("%o\n", (IPC_EXCL ));
    return 0;
}
