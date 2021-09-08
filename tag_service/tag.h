//
// Created by tiziana on 24/08/21.
//

#ifndef SOA_PROJECT_TM_TAG_H
#define SOA_PROJECT_TM_TAG_H


#ifdef  __KERNEL__

#define MODNAME "TAG-SERVICE"

#define LEVELS 32
#define MAX_TAG 256
#define MAX_KEY 256
#define MSG_LEN 4096

#endif


#define AWAKE_ALL  00006000   /* awake all threads waiting for a message*/

#endif //SOA_PROJECT_TM_TAG_H
