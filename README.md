# TAG-based data exchange service

## Short Description

This project implements a Kernel module for the tag-based message exchange service. Conceptually it is a
publish/subscribe system with a critical difference, there is no a central entity wich takes the message log and wich
provides decoupling among readers and writers.

- Readers follow a tag-level and wait untill a message arrives.
- A writer sends a messsage and wakes up all awaiting threads,then wait delivery ends up. If nobody is waiting for a
  message, just drop it.
- Awaker: conceptually acts as a writer but sends an awake notification instead of a message. This is repeated for every
  level of a tag.
- Remover: Removes a tag when there are no more readers.

For more informations and implemntation details, please see the documentation provided in the **"documentation"**
folder.

## API

```c
/**
 * @description Create a new instance associated with the key or opens an existing one by using the key.
 * This function acts differently basing on the command and key combination.
 * @param key associated to a tag or IPC_PRIVATE
 * @param command Use IPC_CREAT to create a new tag instance associated to the corresponding key or to open an existing one.
 * If  IPC_CREAT | IPC_EXCL is specified and the tag instance associated to the key already exists an error is generated.
 * @param permissions 0 to grant all user access, > 0 if the access is restricted to the creator
 * @return a tag descriptor on success or an appropriate error code.
 * @errors
 * EINVAL: Invalid Arguments.\n
 * ENOMEM: Out of memory.\n
 * EEXIST: Tag already exists and IPC_EXCL is specified with IPC_CREAT.\n
 * EEAGAIN: Operation failed, but if you retry may success.\n
 */
int tag_get(int key, int command, int permissions);

/**
 * @description Send a message to the corresponding tag-level instance, awake all waiting threads then wait delivery ends up.
 * This function could be blocking and could be interrupted by a signal.
 * This service doesn't keep any message log; if nobody waits for the incoming message this is discarded.
 * @param tag tag descriptor
 * @param level message source level
 * @param buffer userspace buffer address
 * @param size buffer lenght, empty messages are anyhow allowed.
 * @return 0 on success, appropriate error code otherwise
 * @errors
 * EINVAL: Invalid Arguments.\n
 * ENOMEM: Out of memory.\n
 * ENOENT: Tag doesn't exist.\n
 * EINTR: Stopped, interrupt occured.\n
 * EPERM: Operation not permited.\n
 * EFAULT: Message delivery fault.\n
 */
int tag_send(int tag, int level, char *buffer, size_t size);

/**
 * @description This operation blocks the caller untill an incoming message arrives from the corresponding tag-level instance.
 * The caller could be unlocked even if a signal arrives or another thread calls tag_clt with the AWAKE_ALL command.
 * @param tag tag descriptor
 * @param level message source level
 * @param buffer userspace buffer address
 * @param size buffer lenght
 * @return bytes copied on success, appropriate error code otherwise.
 * @errors
 * EINVAL: Invalid Arguments.\n
 * ENOBUFS: Not enough buffer space available.\n
 * ENOENT: Tag doesn't exist.\n
 * EINTR: Stopped, interrupt occured.\n
 * EPERM: Operation not permitted.\n
 * EFAULT: Message recovery fault.\n
 * ECANCELED: Operation canceled because of AWAKE notification.\n
 */
int tag_receive(int tag, int level, char *buffer, size_t size);

/**
 * @description This operation control a tag instance by awakening operation or the by removing operation.
 * This function acts differently basing on the command and key combination.
 * @param tag tag descriptor
 * @param command use IPC_RMID command to remove a tag instance, this will fail if there are readers waiting for a message on the corresponding tag.
 * IPC_RMID command can be combinating with IPC_NOWAIT command to have a nonblocking behavior.
 * Use the AWAKE_ALL command to wake up all thread waiting for a message on the corresponding tag indipendently of the level.
 * @return non-negative value on success, negative on failure and errno is set to the correct error code.
 * @errors
 * EINVAL: Invalid Arguments.\n
 * EBUSY: Resource busy.\n
 * ENOENT: Tag doesn't exist.\n
 * EINTR: Stopped, interrupt occured.\n
 * EPERM: Operation not permited.\n
 */
int tag_ctl(int tag, int command);


```

## Installation

1. Use install.sh to compile and insert the module.
2. Use `~cat /dev/mydev` to open and read the device driver.
3. Use uninstall.sh to completely uninstall the service.

>  install.sh and uninstall.sh require root privileges.

## Usage

In the **"user"** folder some examples are provided. Basically the **tag_lib.h** header exposes the system calls, be
carefull system call numers must be adapted to the current installation by using the information printed by the
**install.sh** script.

>  Required Kernel verison  >= 4.20; Tested on 5.11.0-27-generic

## Development Environment

![](clion.png)


**CLion** was used to develop the entire project.
To have a better experience use this Tool if you want to read the code provided.