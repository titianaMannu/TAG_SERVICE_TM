
#ifndef SOA_PROJECT_TM_TAG_DEV_H

#include "../tag_flags.h"
#include "../tag.h"

#define SOA_PROJECT_TM_TAG_DEV_H

#define LINE_LEN 64 // max allowed size for a line of information
#define DEVICE_NAME "tag-device-driver"

#endif //SOA_PROJECT_TM_TAG_DEV_H

typedef struct object_state {
    char *content; // information of the current snapshot of the tag-service
    int content_size;
} dev_object_t;


typedef struct tag_stastus {
    int key;
    kuid_t uid_owner;
    unsigned long standing_readers[LEVELS];
    bool present;
} tag_status_t;
typedef tag_status_t *tag_status_ptr_t;

/**
 * @description This function opens a new session for the device file and build the information needed.
 * Be carefull, this device driver cannot be accessed in time sharing (single istance)
 * if someone else try to open a new session without closing the current one this will cause an error condition.
 *
 * @param inode dev file inode
 * @param file file struct
 * @return 0 or errno is set to a correct value
 */
int open_tag_status(struct inode *inode, struct file *file);

/**
 * @description Allows readings informations about the tag-service collected during the open operation.
 * @param filp file struct
 * @param buff destination user space buffer
 * @param len buffer length
 * @param off reading starting point
 * @return number of bytes read
 */
ssize_t read_tag_status(struct file *filp, char *buff, size_t len, loff_t *off);

/**
 * @description release resources
 * @param inode dev file inode
 * @param file file struct
 * @return 0 on success
 */
int release_tag_status(struct inode *inode, struct file *file);

/**
 * @description Not implemented yet
 */
ssize_t write_tag_status(struct file *filp, const char *buff, size_t len, loff_t *off);

/**
 * @description Builds a text with the information collected from the tag_list of the tag_service.
 * @return integer corresponding to the number of bytes that have been written
 */
int build_content(void);