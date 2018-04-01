#include <sys/stat.h>
#include <time.h>

int file_size(char *fn) {
    struct stat s; 
    if (stat(fn, &s) == 0)
        return s.st_size;
    return 0; 
}

time_t file_date_fd(int fd){
  struct stat s; 
  fstat(fd, &s);
  return s.st_mtime;
}

int max(int a,int b){
  if(a<b) return b;
  return a;
}


