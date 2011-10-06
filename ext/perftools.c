#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ruby.h>

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#include <assert.h>

void ProfilerGcMark(void (*cb)(VALUE));
int  ProfilerStart(const char*);
void ProfilerStop();
void ProfilerFlush();
void ProfilerRecord(int, void*, void*);
int  ProfilingIsEnabledForAllThreads();

static VALUE Iallocate;
static VALUE I__send__;
static VALUE Isend;

#define SAVE_FRAME() \
  if (method && method != I__send__ && method != Isend) { \
    if (self && FL_TEST(klass, FL_SINGLETON) && (BUILTIN_TYPE(self) == T_CLASS || BUILTIN_TYPE(self) == T_MODULE)) \
      result[depth++] = (void*) self; \
    else \
      result[depth++] = 0; \
    \
    result[depth++] = (void*) klass; \
    result[depth++] = (void*) (method == ID_ALLOCATOR ? Iallocate : method); \
  }

#ifdef RUBY18
  #include <env.h>
  #include <node.h>
  #include <setjmp.h>
  #include <signal.h>

  static jmp_buf saved_location;
  static sig_t saved_handler = NULL;

  void
  segv_handler(int sig)
  {
    fprintf(stderr, "*** Caught a segfault during stack trace sampling!\n");
    assert(saved_handler);
    _longjmp(saved_location, 1);
  }

  int
  rb_stack_trace(void** result, int max_depth)
  {
    struct FRAME *frame = ruby_frame;
    NODE *n;

    VALUE klass, self;
    ID method;
    int depth = 0;

    if (max_depth == 0)
      return 0;

    #ifdef HAVE_RB_DURING_GC
    if (rb_during_gc()) {
      result[0] = rb_gc;
      return 1;
    }
    #endif

    // should not be possible to get here and already have a saved signal handler
    assert(!saved_handler);

    // ruby_frame is occasionally inconsistent, so temporarily catch segfaults
    saved_handler = signal(SIGSEGV, segv_handler);
    if (_setjmp(saved_location)) {
      signal(SIGSEGV, saved_handler);
      saved_handler = NULL;
      return 0;
    }

    /*
    // XXX does it make sense to track allocations or not?
    if (frame->last_func == ID_ALLOCATOR) {
      frame = frame->prev;
    }

    // XXX SIGPROF can come in while ruby_frame is in an inconsistent state (rb_call0), so we ignore the top-most frame
    if (frame->last_func && frame->last_class) {
      self = frame->self;
      klass = frame->last_class;
      method = frame->last_func;
      SAVE_FRAME();
    }
    */

    for (; frame && (n = frame->node); frame = frame->prev) {
      if (frame->prev && frame->prev->last_func) {
        if (frame->prev->node == n) {
          if (frame->prev->last_func == frame->last_func) continue;
        }

        if (depth+3 > max_depth)
          break;

        self = frame->prev->self;
        klass = frame->prev->last_class;
        method = frame->prev->last_func;
        SAVE_FRAME();
      }
    }

    signal(SIGSEGV, saved_handler);
    saved_handler = NULL;

    assert(depth <= max_depth);
    return depth;
  }
#endif

#ifdef RUBY19
  #include <vm_core.h>
  #include <iseq.h>

// Fix compile error in ruby 1.9.3
#ifdef RTYPEDDATA_DATA
#define ruby_current_thread ((rb_thread_t *)RTYPEDDATA_DATA(rb_thread_current()))
#endif

  int
  rb_stack_trace(void** result, int max_depth)
  {
    rb_thread_t *th = GET_THREAD();
    rb_control_frame_t *cfp = th->cfp;
    rb_control_frame_t *end_cfp = RUBY_VM_END_CONTROL_FRAME(th);

    VALUE klass, self;
    ID method;
    int depth = 0;

    if (max_depth == 0)
      return 0;

    if (rb_during_gc()) {
      result[0] = rb_gc;
      return 1;
    }

    while (RUBY_VM_VALID_CONTROL_FRAME_P(cfp, end_cfp) && depth+3 <= max_depth) {
      rb_iseq_t *iseq = cfp->iseq;

      if (iseq && iseq->type == ISEQ_TYPE_METHOD) {
        self = 0; // maybe use cfp->self here, but iseq->self is a ISeq ruby obj
        klass = iseq->klass;
        method = iseq->defined_method_id;
        SAVE_FRAME();
      }

      if (depth+3 > max_depth)
        break;

      switch (VM_FRAME_TYPE(cfp)) {
        case VM_FRAME_MAGIC_METHOD:
        case VM_FRAME_MAGIC_CFUNC:
          self = cfp->self;
#ifdef HAVE_METHOD_H
          if (!cfp->me) break;

          klass = cfp->me->klass;
          method = cfp->me->called_id;
#else
          klass = cfp->method_class;
          method = cfp->method_id;
#endif
          SAVE_FRAME();
          break;
      }

      cfp = RUBY_VM_PREVIOUS_CONTROL_FRAME(cfp);
    }

    assert(depth <= max_depth);
    return depth;
  }

  #if 0
  void
  rb_dump_stack()
  {
    rb_thread_t *th = GET_THREAD();
    rb_control_frame_t *cfp = th->cfp;
    rb_control_frame_t *end_cfp = RUBY_VM_END_CONTROL_FRAME(th);
    ID func;

    printf("\n\n*********************\n");
    while (RUBY_VM_VALID_CONTROL_FRAME_P(cfp, end_cfp)) {
      printf("cfp (%p):\n", cfp);
      printf("  type: 0x%x\n", VM_FRAME_TYPE(cfp));
      printf("  pc: %p\n", cfp->pc);
      printf("  iseq: %p\n", cfp->iseq);
      if (cfp->iseq) {
        printf("     type: %d\n", FIX2INT(cfp->iseq->type));
        printf("     self: %p\n", cfp->iseq->self);
        printf("     klass: %p (%s)\n", cfp->iseq->klass, cfp->iseq->klass ? rb_class2name(cfp->iseq->klass) : "");
        printf("     method: %p (%s)\n", cfp->iseq->defined_method_id, cfp->iseq->defined_method_id ? rb_id2name(cfp->iseq->defined_method_id) : "");
      }
      printf("  self: %p\n", cfp->self);
      printf("  klass: %p (%s)\n", cfp->method_class, cfp->method_class ? rb_class2name(cfp->method_class) : "");
      printf("  method: %p (%s)\n", cfp->method_id, cfp->method_id ? rb_id2name(cfp->method_id) : "");

      cfp = RUBY_VM_PREVIOUS_CONTROL_FRAME(cfp);
      printf("\n");
    }
    printf("*********************\n\n");
  }
  #endif
#endif

static VALUE objprofiler_setup();
static VALUE objprofiler_teardown();
static VALUE methprofiler_setup();
static VALUE methprofiler_teardown();

/* CpuProfiler */

static VALUE cPerfTools;
static VALUE cCpuProfiler;
static VALUE eError;
static VALUE bProfilerRunning;
static VALUE gc_hook;

static VALUE
cpuprofiler_running_p(VALUE self)
{
  return bProfilerRunning;
}

static VALUE
cpuprofiler_stop(VALUE self)
{
  if (!bProfilerRunning)
    return Qfalse;

  bProfilerRunning = Qfalse;
  objprofiler_teardown();
  methprofiler_teardown();
  ProfilerStop();
  ProfilerFlush();

  return Qtrue;
}

static VALUE
cpuprofiler_start(VALUE self, VALUE filename)
{
  StringValue(filename);

  if (bProfilerRunning)
    rb_raise(eError, "profiler is already running");

  if (getenv("CPUPROFILE_OBJECTS"))
    objprofiler_setup();
  else if (getenv("CPUPROFILE_METHODS"))
    methprofiler_setup();

  if (ProfilerStart(RSTRING_PTR(filename))) {
    bProfilerRunning = Qtrue;
  } else {
    rb_raise(eError, "profiler could not be started");
  }

  if (rb_block_given_p()) {
    rb_yield(Qnil);
    cpuprofiler_stop(self);
  }

  return Qtrue;
}

static void
cpuprofiler_gc_mark()
{
  ProfilerGcMark(rb_gc_mark);
}

/* ObjProfiler */

#include <assert.h>
#include <ucontext.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

static VALUE bObjProfilerRunning;
static VALUE bMethProfilerRunning;
#define NUM_ORIG_BYTES 2

struct {
  char *location;
  unsigned char value;
} orig_bytes[NUM_ORIG_BYTES];

static inline void *
page_align(void *addr) {
  assert(addr != NULL);
  return (void *)((size_t)addr & ~(0xFFFF));
}

static void
unprotect_page(char *addr) {
  assert(addr != NULL);
  char *aligned_addr = page_align(addr);
  if (mprotect(aligned_addr, (addr - aligned_addr), PROT_READ|PROT_WRITE|PROT_EXEC) != 0)
    perror("mprotect");
}

static inline char**
uc_get_ip(ucontext_t *uc) {
# if defined(__FreeBSD__)
#   ifdef __i386__
#     define program_counter uc_mcontext.mc_eip
#   else
#     define program_counter uc_mcontext.mc_rip
#   endif
# elif defined(__dietlibc__)
#  define program_counter uc_mcontext.rip
# elif defined(__APPLE__)
#   ifdef __LP64__
#     define program_counter uc_mcontext->__ss.__rip
#   else
#     define program_counter uc_mcontext->__ss.__eip
#   endif
# else
#    ifdef REG_RIP
#      define program_counter uc_mcontext.gregs[REG_RIP]
#    else
#      define program_counter uc_mcontext.gregs[REG_EIP]
#    endif
# endif
  return (char**)&uc->program_counter;
}

#if defined(RB_EVENT_HOOKS_HAVE_CALLBACK_DATA) || defined(RUBY_EVENT_VM)
static void
event_handler(rb_event_flag_t event, VALUE data, VALUE self, ID id, VALUE klass) {
  ProfilerRecord(0, NULL, NULL);
}
#else
static void
event_handler(rb_event_t event, NODE *node, VALUE self, ID id, VALUE klass) {
  ProfilerRecord(0, NULL, NULL);
}
#endif

static VALUE
methprofiler_setup()
{
  if (bMethProfilerRunning)
    return Qtrue;

#if defined(RB_EVENT_HOOKS_HAVE_CALLBACK_DATA) || defined(RUBY_EVENT_VM)
  rb_add_event_hook(event_handler, RUBY_EVENT_CALL|RUBY_EVENT_C_CALL, 0);
#else
  rb_add_event_hook(event_handler, RUBY_EVENT_CALL|RUBY_EVENT_C_CALL);
#endif

  bMethProfilerRunning = Qtrue;
  return Qtrue;
}

static VALUE
methprofiler_teardown()
{
  if (!bMethProfilerRunning)
    return Qfalse;

  rb_remove_event_hook(event_handler);

  bMethProfilerRunning = Qfalse;
  return Qtrue;
}

static void
trap_handler(int sig, siginfo_t *info, void *data) {
  int i;
  ucontext_t *uc = (ucontext_t *)data;
  char **ip = uc_get_ip(uc);

  // printf("signal: %d, addr: %p, ip: %p\n", signal, info->si_addr, *ip);

  for (i=0; i<NUM_ORIG_BYTES; i++) {
    if (orig_bytes[i].location == *ip-1) {
      // restore original byte
      orig_bytes[i].location[0] = orig_bytes[i].value;

      // setup next breakpoint
      orig_bytes[(i+1)%NUM_ORIG_BYTES].location[0] = '\xCC';

      // first breakpoint is the notification
      if (i == 0)
        ProfilerRecord(sig, info, data);

      // reset instruction pointer
      *ip -= 1;

      break;
    }
  }
}

static VALUE
objprofiler_setup()
{
  if (bObjProfilerRunning)
    return Qtrue;

  int i;
  struct sigaction sig = { .sa_sigaction = trap_handler, .sa_flags = SA_SIGINFO };
  sigemptyset(&sig.sa_mask);
  sigaction(SIGTRAP, &sig, NULL);

  unprotect_page((char*)rb_newobj);

  for (i=0; i<NUM_ORIG_BYTES; i++) {
    orig_bytes[i].location = (char *)rb_newobj + i;
    orig_bytes[i].value    = ((unsigned char*)rb_newobj)[i];
    orig_bytes[i].location[0] = '\xCC';
  }

  // setenv("CPUPROFILE_OBJECTS", "1", 1);
  bObjProfilerRunning = Qtrue;
  return Qtrue;
}

static VALUE
objprofiler_teardown()
{
  if (!bObjProfilerRunning)
    return Qfalse;

  int i;
  struct sigaction sig = { .sa_handler = SIG_IGN };
  sigemptyset(&sig.sa_mask);
  sigaction(SIGTRAP, &sig, NULL);

  for (i=0; i<NUM_ORIG_BYTES; i++) {
    orig_bytes[i].location[0] = orig_bytes[i].value;
  }

  // unsetenv("CPUPROFILE_OBJECTS");
  bObjProfilerRunning = Qfalse;
  return Qtrue;
}

/* Init */

static void
profiler_at_exit(VALUE self)
{
  cpuprofiler_stop(self);
}

void
Init_perftools()
{
  cPerfTools = rb_define_class("PerfTools", rb_cObject);
  eError = rb_define_class_under(cPerfTools, "Error", rb_eStandardError);
  cCpuProfiler = rb_define_class_under(cPerfTools, "CpuProfiler", rb_cObject);

  Iallocate = rb_intern("allocate");
  I__send__ = rb_intern("__send__");
  Isend = rb_intern("send");

  bMethProfilerRunning = bObjProfilerRunning = bProfilerRunning = Qfalse;

  rb_define_singleton_method(cCpuProfiler, "running?", cpuprofiler_running_p, 0);
  rb_define_singleton_method(cCpuProfiler, "start", cpuprofiler_start, 1);
  rb_define_singleton_method(cCpuProfiler, "stop", cpuprofiler_stop, 0);

  gc_hook = Data_Wrap_Struct(cCpuProfiler, cpuprofiler_gc_mark, NULL, NULL);
  rb_global_variable(&gc_hook);

  if (ProfilingIsEnabledForAllThreads()) { // profiler is already running?
    bProfilerRunning = Qtrue;

    if (getenv("CPUPROFILE_OBJECTS")) {    // want to profile objects
      objprofiler_setup();
    } else if (getenv("CPUPROFILE_METHODS")) {
      methprofiler_setup();
    }

    rb_set_end_proc(profiler_at_exit, 0);  // make sure to cleanup before the VM shuts down
  }
}
