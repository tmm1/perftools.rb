CWD = File.expand_path(File.dirname(__FILE__))

def sys(cmd)
  puts "  -- #{cmd}"
  unless ret = xsystem(cmd)
    raise "#{cmd} failed, please report to https://github.com/tmm1/perfools.rb/issues/new with #{CWD}/src/mkmf.log and #{CWD}/src/gperftools-2.0/config.log"
  end
  ret
end

require 'mkmf'
require 'fileutils'
if RUBY_VERSION > "1.9.3"
  begin
    require 'debase/ruby_core_source'
  rescue LoadError
    require 'rubygems/dependency_installer'
    installer = Gem::DependencyInstaller.new
    installer.install 'debase-ruby_core_source'

    Gem.refresh
    Gem::Specification.find_by_name('debase-ruby_core_source').activate
    require 'debase/ruby_core_source'
  end
else
  begin
    require "debugger/ruby_core_source"
  rescue LoadError
    require 'rubygems/user_interaction' # for 1.9.1
    require 'rubygems/dependency_installer'
    installer = Gem::DependencyInstaller.new
    installer.install 'debugger-ruby_core_source'

    Gem.refresh
    if Gem.method_defined? :activate
      Gem.activate('debugger-ruby_core_source') # for 1.9.1
    else
      Gem::Specification.find_by_name('debugger-ruby_core_source').activate
    end

    require "debugger/ruby_core_source"
  end
end

perftools = File.basename('gperftools-2.0.tar.gz')
dir = File.basename(perftools, '.tar.gz')

puts "(I'm about to compile google-perftools.. this will definitely take a while)"
ENV["PATCH_GET"] = '0'

Dir.chdir('src') do
  FileUtils.rm_rf(dir) if File.exists?(dir)

  sys("tar zpxvf #{perftools}")
  Dir.chdir(dir) do
    if ENV['DEV']
      sys("git init")
      sys("git add .")
      sys("git commit -m 'initial source'")
    end

    [ ['perftools', true],
      ['perftools-notests', true],
      ['perftools-pprof', true],
      ['perftools-gc', true],
      ['perftools-osx', false], # fixed in 2.0
      ['perftools-debug', true],
      ['perftools-objects', true],
      ['perftools-frames', true],
      ['perftools-realtime', true],
      ['perftools-pause', true]
    ].each do |patch, apply|
      if apply
        sys("patch -p1 < ../../../patches/#{patch}.patch")
        sys("git commit -am '#{patch}'") if ENV['DEV']
      end
    end

    sys("sed -i -e 's,SpinLock,ISpinLock,g' src/*.cc src/*.h src/base/*.cc src/base/*.h")
    sys("git commit -am 'rename spinlock'") if ENV['DEV']
  end

  Dir.chdir(dir) do
    FileUtils.cp 'src/pprof', '../../../bin/'
    FileUtils.chmod 0755, '../../../bin/pprof'
  end

  Dir.chdir(dir) do
    if RUBY_PLATFORM =~ /darwin10/
      ENV['CFLAGS'] = ENV['CXXFLAGS'] = '-D_XOPEN_SOURCE'
    end
    sys("./configure --disable-heap-profiler --disable-heap-checker --disable-debugalloc --disable-shared")
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
when /darwin/, /linux/, /freebsd/
  CONFIG['LDSHARED'] = "$(CXX) " + CONFIG['LDSHARED'].split[1..-1].join(' ')
end

if RUBY_VERSION >= "1.9"
  add_define 'RUBY19'

  if RUBY_VERSION >= "2.0"
    add_define 'HAVE_RB_NEWOBJ_OF'
  end

  hdrs = proc {
    have_header("method.h") # exists on 1.9.2
    have_header("vm_core.h") and
    have_header("iseq.h") and
    have_header("insns.inc") and
    have_header("insns_info.inc")
  }

  core_source = Debugger rescue Debase

  unless core_source::RubyCoreSource::create_makefile_with_core(hdrs, "perftools")
    STDERR.puts "\n\n"
    STDERR.puts "***************************************************************************************"
    STDERR.puts "****************** Debugger::RubyCoreSource::create_makefile FAILED *******************"
    STDERR.puts "***************************************************************************************"
    exit(1)
  end
else
  add_define 'RUBY18'

  have_func('rb_during_gc', 'ruby.h')
  create_makefile 'perftools'
end
