#define _GNU_SOURCE
#define _XOPEN_SOURCE 600

#include <assert.h>
#include <ucontext.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>


void
func() {
  printf("hi\n");
}


static inline void *
page_align(void *addr) {
  assert(addr != NULL);
  return (void *)((size_t)addr & ~(0xFFFF));
}

static void
copy_instructions(void *dest, void *src, size_t count) {
  assert(dest != NULL);
  assert(src != NULL);

  void *aligned_addr = page_align(dest);
  if (mprotect(aligned_addr, (dest - aligned_addr) + count, PROT_READ|PROT_WRITE|PROT_EXEC) != 0)
    perror("mprotect");
  memcpy(dest, src, count);
}

#define NUM_ORIG_BYTES 2
struct {
  void *location;
  unsigned char value;
} orig_bytes[NUM_ORIG_BYTES];

static inline void**
uc_get_ip(ucontext_t *uc) {
  #if defined(__FreeBSD__)
    return (void**)&uc->uc_mcontext.mc_rip;
  #elif defined(__dietlibc__)
    return (void**)&uc->uc_mcontext.rip;
  #elif defined(__APPLE__)
    return (void**)&uc->uc_mcontext->__ss.__rip;
  #else
    return (void**)&uc->uc_mcontext.gregs[REG_RIP];
  #endif
}

static void
trap_handler(int signal, siginfo_t *info, void *data) {
  int i;
  ucontext_t *uc = (ucontext_t *)data;
  void **ip = uc_get_ip(uc);

  // printf("signal: %d, addr: %p, ip: %p\n", signal, info->si_addr, *ip);

  for (i=0; i<NUM_ORIG_BYTES; i++) {
    if (orig_bytes[i].location == *ip-1) {
      // restore original byte
      copy_instructions(orig_bytes[i].location, &orig_bytes[i].value, 1);

      // setup next breakpoint
      copy_instructions(orig_bytes[(i+1)%NUM_ORIG_BYTES].location, "\xCC", 1);

      // first breakpoint is the notification
      if (i == 0)
        printf(" ---> YOU'RE CALLING FUNC()\n");

      // reset instruction pointer
      *ip -= 1;

      break;
    }
  }
}

int
main() {
  int i;
  struct sigaction sig = { .sa_sigaction = trap_handler, .sa_flags = SA_SIGINFO };
  sigemptyset(&sig.sa_mask);
  sigaction(SIGTRAP, &sig, NULL);

  for (i=0; i<NUM_ORIG_BYTES; i++) {
    orig_bytes[i].location = func + i;
    orig_bytes[i].value    = ((unsigned char*)func)[i];
    copy_instructions(func + i, "\xCC", 1);
  }

  printf("func: %p\n", func);

  for (i=0; i<10; i++)
    func();

  return 0;
}
