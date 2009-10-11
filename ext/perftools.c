#include <ruby.h>
static VALUE Iallocate;
static VALUE I__send__;

#define SAVE_FRAME() \
  if (method != I__send__) { \
    if (FL_TEST(klass, FL_SINGLETON) && (BUILTIN_TYPE(self) == T_CLASS || BUILTIN_TYPE(self) == T_MODULE)) \
      result[depth++] = (void*) self; \
    else \
      result[depth++] = 0; \
    \
    result[depth++] = (void*) klass; \
    result[depth++] = (void*) (method == ID_ALLOCATOR ? Iallocate : method); \
  }

#ifdef RUBY18
  #include <node.h>
  #include <env.h>

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

    /*
    // XXX does it make sense to track allocations or not?
    if (frame->last_func == ID_ALLOCATOR) {
      frame = frame->prev;
    }

    // XXX SIGPROF can come in while ruby_frame is in an inconsistent state (rb_call0), so we ignore the top-most frame
    if (frame->last_func) {
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

    return depth;
  }
#endif

#ifdef RUBY19
  #include <vm_core.h>
  #include <iseq.h>

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

    #ifdef HAVE_RB_DURING_GC
    if (rb_during_gc()) {
      result[0] = rb_gc;
      return 1;
    }
    #endif

    while (RUBY_VM_VALID_CONTROL_FRAME_P(cfp, end_cfp) && depth+3 < max_depth) {
      rb_iseq_t *iseq = cfp->iseq;

      if (iseq && iseq->type == ISEQ_TYPE_METHOD) {
        self = iseq->self;
        klass = iseq->klass;
        method = iseq->defined_method_id;
        SAVE_FRAME();
      }

      switch(VM_FRAME_TYPE(cfp)) {
        case VM_FRAME_MAGIC_METHOD:
        case VM_FRAME_MAGIC_CFUNC:
          self = cfp->self;
          klass = cfp->method_class;
          method = cfp->method_id;
          SAVE_FRAME();
          break;
      }

      cfp = RUBY_VM_PREVIOUS_CONTROL_FRAME(cfp);
    }

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

      func = frame_func_id(cfp);
      printf("  func(): %p (%s)\n", func, func ? rb_id2name(func) : "");

      cfp = RUBY_VM_PREVIOUS_CONTROL_FRAME(cfp);
      printf("\n");
    }
    printf("*********************\n\n");
  }
  #endif
#endif

static VALUE cPerfTools;
static VALUE cCpuProfiler;
static VALUE bProfilerRunning;
static VALUE gc_hook;

VALUE
cpuprofiler_running_p(VALUE self)
{
  return bProfilerRunning;
}

VALUE
cpuprofiler_stop(VALUE self)
{
  if (!bProfilerRunning)
    return Qfalse;

  bProfilerRunning = Qfalse;
  ProfilerStop();
  ProfilerFlush();
  return Qtrue;
}

VALUE
cpuprofiler_start(VALUE self, VALUE filename)
{
  StringValue(filename);

  if (bProfilerRunning)
    return Qfalse;

  ProfilerStart(RSTRING_PTR(filename));
  bProfilerRunning = Qtrue;

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

void
Init_perftools()
{
  cPerfTools = rb_define_class("PerfTools", rb_cObject);
  cCpuProfiler = rb_define_class_under(cPerfTools, "CpuProfiler", rb_cObject);
  bProfilerRunning = Qfalse;
  Iallocate = rb_intern("allocate");
  I__send__ = rb_intern("__send__");

  rb_define_singleton_method(cCpuProfiler, "running?", cpuprofiler_running_p, 0);
  rb_define_singleton_method(cCpuProfiler, "start", cpuprofiler_start, 1);
  rb_define_singleton_method(cCpuProfiler, "stop", cpuprofiler_stop, 0);

  gc_hook = Data_Wrap_Struct(cCpuProfiler, cpuprofiler_gc_mark, NULL, NULL);
  rb_global_variable(&gc_hook);
}
