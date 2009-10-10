CWD = File.expand_path(File.dirname(__FILE__))

def sys(cmd)
  puts "  -- #{cmd}"
  unless ret = xsystem(cmd)
    raise "#{cmd} failed, please report to perftools@tmm1.net with pastie.org link to #{CWD}/mkmf.log and #{CWD}/src/google-perftools-1.4/config.log"
  end
  ret
end

require 'mkmf'
$LIBPATH << CWD

if have_func('rb_thread_blocking_region')
  raise 'Ruby 1.9 is not supported yet'
end

require 'fileutils'

perftools = File.basename('google-perftools-1.4.tar.gz')
dir = File.basename(perftools, '.tar.gz')

puts "(I'm about to compile google-perftools.. this will definitely take a while)"

Dir.chdir('src') do
  unless File.exists?(dir)
    sys("tar zxvf #{perftools}")
    Dir.chdir(dir) do
      sys("patch -p1 < ../../../patches/perftools.patch")
      sys("patch -p1 < ../../../patches/perftools-gc.patch")
      sys("patch -p1 < ../../../patches/perftools-osx.patch") if RUBY_PLATFORM =~ /darwin/
      sys("patch -p1 < ../../../patches/perftools-osx-106.patch") if RUBY_PLATFORM =~ /darwin10/
      sys("patch -p1 < ../../../patches/perftools-debug.patch")
    end
  end

  unless File.exists?('../bin/pprof')
    Dir.chdir(dir) do
      FileUtils.cp 'src/pprof', '../../../bin/'
    end
  end

  unless File.exists?('../librubyprofiler.a')
    Dir.chdir(dir) do
      if RUBY_PLATFORM =~ /darwin10/
        ENV['CFLAGS'] = ENV['CXXFLAGS'] = '-D_XOPEN_SOURCE'
      end
      sys("./configure --disable-heap-profiler --disable-heap-checker --disable-shared")
      sys("make")
      FileUtils.cp '.libs/libprofiler.a', '../../librubyprofiler.a'
    end
  end
end

case RUBY_PLATFORM
when /darwin/, /linux/
  CONFIG['LDSHARED'] = "$(CXX) " + CONFIG['LDSHARED'].split[1..-1].join(' ')
end

$libs = append_library($libs, 'rubyprofiler')
have_func('rb_during_gc', 'ruby.h')
create_makefile 'perftools'