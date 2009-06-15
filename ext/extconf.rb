require 'mkmf'
require 'fileutils'
require 'net/http'

url = 'http://google-perftools.googlecode.com/files/google-perftools-1.2.tar.gz'
perftools = File.basename(url)
dir = File.basename(perftools, '.tar.gz')

Logging.message "(I'm about to download and compile google-perftools.. this will definitely take a while)"

FileUtils.mkdir_p('src')

if proxy = URI(ENV['http_proxy'] || ENV['HTTP_PROXY']) rescue nil
  proxy_host = proxy.host
  proxy_port = proxy.port
  proxy_user, proxy_pass = proxy.userinfo.split(/:/) if proxy.userinfo
end

Dir.chdir('src') do
  unless File.exists?(perftools)
    Net::HTTP::Proxy(proxy_host, proxy_port, proxy_user, proxy_pass).get_response(URI(url)) do |res|
      File.open(perftools, 'wb') do |out|
        res.read_body do |chunk|
          out.write(chunk)
        end
      end
    end
  end

  unless File.exists?(dir)
    xsystem("tar zxvf #{perftools}")
    Dir.chdir(dir) do
      xsystem("patch -p1 < ../../../patches/perftools.patch")
      xsystem("patch -p1 < ../../../patches/perftools-static.patch")
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

$libs = append_library($libs, 'rubyprofiler')
have_func('rb_during_gc', 'ruby.h')
create_makefile 'perftools'