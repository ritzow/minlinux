#ifndef __COMMAND_H__
#define __COMMAND_H__

#include "uring_ctl.h"

void process_command(char *, uring_queue *, char * envp[]);

#endif