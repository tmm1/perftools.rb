#include <ruby.h>
#include <node.h>
#include <env.h>

static VALUE Iallocate;
static VALUE I__send__;

static inline void
save_frame(struct FRAME *frame, void** result, int *depth)
{
  VALUE klass = frame->last_class;
  // XXX what is an ICLASS anyway?
  // if (BUILTIN_TYPE(klass) == T_ICLASS)
  //   klass = RBASIC(klass)->klass;

  if (frame->last_func == I__send__)
    return;

  if (FL_TEST(klass, FL_SINGLETON) &&
      (BUILTIN_TYPE(frame->self) == T_CLASS || BUILTIN_TYPE(frame->self) == T_MODULE))
    result[(*depth)++] = (void*) frame->self;
  else
    result[(*depth)++] = 0;

  result[(*depth)++] = (void*) klass;
  result[(*depth)++] = (void*) (frame->last_func == ID_ALLOCATOR ? Iallocate : frame->last_func);
}

int
rb_stack_trace(void** result, int max_depth)
{
  int depth = 0;
  struct FRAME *frame = ruby_frame;
  NODE *n;

  if (max_depth == 0)
    return 0;

  // XXX: figure out what these mean. is there a way to access them from an extension?
  // if (rb_prohibit_interrupt || !rb_trap_immediate) return 0;

#ifdef HAVE_RB_DURING_GC
  if (rb_during_gc()) {
    result[0] = rb_gc;
    return 1;
  }
#endif

  // XXX does it make sense to track allocations or not?
  if (frame->last_func == ID_ALLOCATOR) {
    frame = frame->prev;
  }

  if (frame->last_func) {
    save_frame(frame, result, &depth);
  }

  for (; frame && (n = frame->node); frame = frame->prev) {
    if (frame->prev && frame->prev->last_func) {
      if (frame->prev->node == n) {
        if (frame->prev->last_func == frame->last_func) continue;
      }

      if (depth+3 > max_depth)
        break;

      save_frame(frame->prev, result, &depth);
    }
  }

  return depth;
}

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
