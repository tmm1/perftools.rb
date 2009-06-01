spec = Gem::Specification.new do |s|
  s.name = 'perftools.rb'
  s.version = '0.1.3'
  s.date = '2009-06-01'
  s.summary = 'google-perftools for ruby code'
  s.description = 'A sampling profiler for ruby code based on patches to google-perftools'

  s.homepage = "http://github.com/tmm1/perftools.rb"

  s.authors = ["Aman Gupta"]
  s.email = "aman@tmm1.net"

  s.has_rdoc = false
  s.extensions = 'ext/extconf.rb'
  s.bindir = 'bin'
  s.executables << 'pprof.rb'

  # ruby -rpp -e' pp `git ls-files | grep -v examples`.split("\n") '
  s.files = [
    "README",
    "bin/pprof.rb",
    "ext/extconf.rb",
    "ext/perftools.c",
    "patches/perftools-debug.patch",
    "patches/perftools-osx.patch",
    "patches/perftools-static.patch",
    "patches/perftools.patch",
    "perftools.rb.gemspec"
  ]
end
