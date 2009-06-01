#include <ruby.h>
#include <node.h>
#include <env.h>

int
rb_stack_trace(void** result, int max_depth)
{
  int depth = 0;
  struct FRAME *frame = ruby_frame;
  NODE *n;

  if (max_depth == 0) return 0;
  // if (rb_prohibit_interrupt || !rb_trap_immediate) return 0;

  if (rb_during_gc()) {
    result[0] = rb_gc;
    return 1;
  }

  if (frame->last_func == ID_ALLOCATOR) {
    frame = frame->prev;
  }

  if (frame->last_func) {
    VALUE klass = frame->last_class;
    // if (BUILTIN_TYPE(klass) == T_ICLASS)
    //   klass = RBASIC(klass)->klass;

    if (FL_TEST(klass, FL_SINGLETON) && (BUILTIN_TYPE(frame->self) == T_CLASS || BUILTIN_TYPE(frame->self) == T_MODULE))
      result[depth++] = (void*) frame->self;
    else
      result[depth++] = 0;

    result[depth++] = (void*) klass;
    result[depth++] = (void*) frame->last_func;
  }

  for (; frame && (n = frame->node); frame = frame->prev) {
    if (frame->prev && frame->prev->last_func) {
      if (frame->prev->node == n) {
        if (frame->prev->last_func == frame->last_func) continue;
      }

      if (depth+3 > max_depth) break;

      VALUE klass = frame->prev->last_class;
      // if (BUILTIN_TYPE(klass) == T_ICLASS)
      //   klass = RBASIC(klass)->klass;

      if (FL_TEST(klass, FL_SINGLETON) && (BUILTIN_TYPE(frame->prev->self) == T_CLASS || BUILTIN_TYPE(frame->prev->self) == T_MODULE))
        result[depth++] = (void*) frame->prev->self;
      else
        result[depth++] = 0;

      result[depth++] = (void*) klass;
      result[depth++] = (void*) frame->prev->last_func;
    }
  }

  return depth;
}

static VALUE cPerfTools;
static VALUE cCpuProfiler;
static VALUE bProfilerRunning;

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

void
Init_perftools()
{
  cPerfTools = rb_define_class("PerfTools", rb_cObject);
  cCpuProfiler = rb_define_class_under(cPerfTools, "CpuProfiler", rb_cObject);
  bProfilerRunning = Qfalse;

  rb_define_singleton_method(cCpuProfiler, "running?", cpuprofiler_running_p, 0);
  rb_define_singleton_method(cCpuProfiler, "start", cpuprofiler_start, 1);
  rb_define_singleton_method(cCpuProfiler, "stop", cpuprofiler_stop, 0);
}