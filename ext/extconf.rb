CWD = File.expand_path(File.dirname(__FILE__))

def sys(cmd)
  puts "  -- #{cmd}"
  unless ret = xsystem(cmd)
    raise "#{cmd} failed, please report to perftools@tmm1.net with pastie.org link to #{CWD}/mkmf.log and #{CWD}/src/google-perftools-1.4/config.log"
  end
  ret
end

require 'mkmf'
require 'fileutils'

if RUBY_VERSION >= "1.9"
  begin
    require "ruby_core_source"
  rescue LoadError
    STDERR.puts "\n\n"
    STDERR.puts "***************************************************************************************"
    STDERR.puts "******************** PLEASE RUN gem install ruby_core_source FIRST ********************"
    STDERR.puts "***************************************************************************************"
    exit(1)
  end
end

perftools = File.basename('google-perftools-1.4.tar.gz')
dir = File.basename(perftools, '.tar.gz')

puts "(I'm about to compile google-perftools.. this will definitely take a while)"

Dir.chdir('src') do
  FileUtils.rm_rf(dir) if File.exists?(dir)

  sys("tar zxvf #{perftools}")
  Dir.chdir(dir) do
    sys("patch -p1 < ../../../patches/perftools.patch")
    sys("patch -p1 < ../../../patches/perftools-pprof.patch")
    sys("patch -p1 < ../../../patches/perftools-gc.patch")
    sys("patch -p1 < ../../../patches/perftools-osx.patch") if RUBY_PLATFORM =~ /darwin/
    sys("patch -p1 < ../../../patches/perftools-osx-106.patch") if RUBY_PLATFORM =~ /darwin10/
    sys("patch -p1 < ../../../patches/perftools-debug.patch")
  end

  Dir.chdir(dir) do
    FileUtils.cp 'src/pprof', '../../../bin/'
    FileUtils.chmod 0755, '../../../bin/pprof'
  end

  Dir.chdir(dir) do
    if RUBY_PLATFORM =~ /darwin10/
      ENV['CFLAGS'] = ENV['CXXFLAGS'] = '-D_XOPEN_SOURCE'
    end
    sys("./configure --disable-heap-profiler --disable-heap-checker --disable-shared")
    sys("make")
    FileUtils.cp '.libs/libprofiler.a', '../../librubyprofiler.a'
  end
end

$LIBPATH << CWD
$libs = append_library($libs, 'rubyprofiler')
def add_define(name)
  $defs.push("-D#{name}")
end

case RUBY_PLATFORM
when /darwin/, /linux/
  CONFIG['LDSHARED'] = "$(CXX) " + CONFIG['LDSHARED'].split[1..-1].join(' ')
end

if RUBY_VERSION >= "1.9"
  add_define 'RUBY19'

  hdrs = proc {
    have_header("vm_core.h") and
    have_header("iseq.h") and
    have_header("insns.inc") and
    have_header("insns_info.inc")
  }
  have_func('rb_during_gc', 'ruby.h')

  unless Ruby_core_source::create_makefile_with_core(hdrs, "perftools")
    STDERR.puts "\n\n"
    STDERR.puts "***************************************************************************************"
    STDERR.puts "********************** Ruby_core_source::create_makefile FAILED ***********************"
    STDERR.puts "***************************************************************************************"
    exit(1)
  end
else
  add_define 'RUBY18'

  have_func('rb_during_gc', 'ruby.h')
  create_makefile 'perftools'
end
