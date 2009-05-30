spec = Gem::Specification.new do |s|
  s.name = 'perftools.rb'
  s.version = '0.1.0'
  s.date = '2009-05-30'
  s.summary = 'google-perftools for ruby code'
  s.description = s.summary

  s.homepage = "http://github.com/tmm1/perftools.rb"

  s.authors = ["Aman Gupta"]
  s.email = "aman@tmm1.net"

  s.has_rdoc = false
  s.extensions = 'ext/extconf.rb'

  # ruby -rpp -e' pp `git ls-files`.split("\n") '
  s.files = [
    "README",
    "perftools.rb.gemspec",
    "ext/extconf.rb",
    "ext/perftools.c",
    "patches/ruby.patch",
    "patches/perftools.patch",
    "patches/perftools-osx.patch",
    "patches/perftools-debug.patch"
  ]
end
