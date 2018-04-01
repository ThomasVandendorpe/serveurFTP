#ifndef _UTILS_H_
#define _UTILS_H_

#include <time.h>

int file_size(char *fn);
time_t file_date_fd(int fd);
int max(int a,int b);

#endif
