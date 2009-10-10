require 'mkmf'

if have_func('rb_thread_blocking_region')
  raise 'Ruby 1.9 is not supported yet'
end

require 'fileutils'

perftools = File.basename('google-perftools-1.4.tar.gz')
dir = File.basename(perftools, '.tar.gz')

puts "(I'm about to compile google-perftools.. this will definitely take a while)"

Dir.chdir('src') do
  unless File.exists?(dir)
    xsystem("tar zxvf #{perftools}")
    Dir.chdir(dir) do
      xsystem("patch -p1 < ../../../patches/perftools.patch")
      xsystem("patch -p1 < ../../../patches/perftools-gc.patch")
      xsystem("patch -p1 < ../../../patches/perftools-osx.patch") if RUBY_PLATFORM =~ /darwin/
      xsystem("patch -p1 < ../../../patches/perftools-debug.patch")# if ENV['DEBUG']
    end
  end

  unless File.exists?('../bin/pprof')
    Dir.chdir(dir) do
      FileUtils.cp 'src/pprof', '../../../bin/'
    end
  end

  unless File.exists?('../librubyprofiler.a')
    Dir.chdir(dir) do
      xsystem("./configure --disable-heap-profiler --disable-heap-checker --disable-shared")
      xsystem("make")
      FileUtils.cp '.libs/libprofiler.a', '../../librubyprofiler.a'
    end
  end
end

case RUBY_PLATFORM
when /darwin/, /linux/
  CONFIG['LDSHARED'] = "$(CXX) " + CONFIG['LDSHARED'].split[1..-1].join(' ')
end

$LIBPATH << '.'
$libs = append_library($libs, 'rubyprofiler')
have_func('rb_during_gc', 'ruby.h')
create_makefile 'perftools'