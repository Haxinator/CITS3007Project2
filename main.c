#define _POSIX_C_SOURCE 200112L

#include "adjust_score.c"
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main(int argc, char** argv)
{
  uid_t myuid = getuid();
  gid_t mygid = getgid();
  printf("myuid: %d\n", myuid);
  printf("mygid: %d\n", mygid);


  //make sure euid is the same as uid
  if(seteuid(myuid) == -1)
  {
    fprintf(stderr, "error setting uid: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  //make sure egid is the same as gid
  if(setegid(mygid) == -1)
  {
    fprintf(stderr, "error setting uid: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  printf("myeuid: %d\n", geteuid());
  printf("myegid: %d\n", getegid());

  char **message;

  if(argc == 1)
  {
    adjust_score(1001,"J", 20, message);
  } else if(argc == 2){
    adjust_score(1001,argv[1], 20, message);
  } else {
    printf("Invalid Params\n");
  }
  return 0;
}
