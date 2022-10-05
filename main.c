#define _POSIX_C_SOURCE 200112L

#include "adjust_score.c"
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

int main(void)
{
  uid_t myuid = getuid();
  gid_t mygid = getgid();
  printf("myuid: %d\n", myuid);
  printf("mygid: %d\n", mygid);


  //make sure euid is the same as uid
  if(seteuid(myuid) == -1)
  {
    fprintf(stderr, "error setting uid: %s\n", strerror(errno));
  }

  //make sure egid is the same as gid
  if(setegid(mygid) == -1)
  {
    fprintf(stderr, "error setting uid: %s\n", strerror(errno));
  }

  printf("myeuid: %d\n", geteuid());
  printf("myegid: %d\n", getegid());

  char **message;

  adjust_score(1001,"BobFoster", 20, message);

  return 0;
}
