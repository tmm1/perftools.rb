spec = Gem::Specification.new do |s|
  s.name = 'perftools.rb'
  s.version = '0.3.5'
  s.date = '2009-10-13'
  s.rubyforge_project = 'perftools-rb'
  s.summary = 'google-perftools for ruby code'
  s.description = 'A sampling profiler for ruby code based on patches to google-perftools'

  s.homepage = "http://github.com/tmm1/perftools.rb"

  s.authors = ["Aman Gupta"]
  s.email = "perftools@tmm1.net"

  s.has_rdoc = false
  s.extensions = 'ext/extconf.rb'
  s.bindir = 'bin'
  s.executables << 'pprof.rb'

  # ruby -rpp -e' pp `git ls-files | grep -v examples`.split("\n").sort '
  s.files = [
    "README",
    "bin/pprof.rb",
    "ext/extconf.rb",
    "ext/perftools.c",
    "ext/src/google-perftools-1.4.tar.gz",
    "patches/perftools-debug.patch",
    "patches/perftools-gc.patch",
    "patches/perftools-osx-106.patch",
    "patches/perftools-osx.patch",
    "patches/perftools-pprof.patch",
    "patches/perftools-notests.patch",
    "patches/perftools.patch",
    "perftools.rb.gemspec"
  ]
end
