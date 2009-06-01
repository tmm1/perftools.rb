require 'mkmf'
require 'fileutils'
require 'net/http'

# http://google-perftools.googlecode.com/files/google-perftools-1.2.tar.gz
perftools = "google-perftools-1.2.tar.gz"
dir = File.basename(perftools, '.tar.gz')

Logging.message "(I'm about to download and compile google-perftools.. this will definitely take a while)"

FileUtils.mkdir("src") unless File.exists?("src")
Dir.chdir("src") do
  Net::HTTP.start("google-perftools.googlecode.com") { |http|
    resp = http.get("/files/#{perftools}")
    open(perftools, "wb") { |file|
      file.write(resp.body)
    }
  } unless File.exists?(perftools)

  unless File.exists?(dir)
    xsystem("tar zxvf #{perftools}")
    Dir.chdir(dir) do
      xsystem("patch -p1 < ../../../patches/perftools.patch")
      xsystem("patch -p1 < ../../../patches/perftools-static.patch")
      xsystem("patch -p1 < ../../../patches/perftools-osx.patch") if RUBY_PLATFORM =~ /darwin/
      xsystem("patch -p1 < ../../../patches/perftools-debug.patch") if ENV['DEBUG']
    end
  end

  unless File.exists?('../bin/pprof')
    Dir.chdir(dir) do
      FileUtils.cp 'src/pprof', '../../../bin/'
    end
  end

  unless File.exists?('../libprofiler.a')
    Dir.chdir(dir) do
      xsystem("./configure --disable-heap-profiler --disable-heap-checker --disable-minimal --disable-shared")
      xsystem("make")
      FileUtils.cp '.libs/libprofiler.a', '../../'
    end
  end
end

$libs = append_library($libs, 'profiler')
create_makefile 'perftools'