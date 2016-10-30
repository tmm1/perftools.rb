spec = Gem::Specification.new do |s|
  s.name = 'perftools.rb'
  s.version = '2.0.5'
  s.rubyforge_project = 'perftools-rb'
  s.summary = 'gperftools for ruby code'
  s.description = 'A sampling profiler for ruby code based on patches to gperftools'

  s.homepage = "http://github.com/tmm1/perftools.rb"

  s.authors = ["Aman Gupta"]
  s.email = "perftools@tmm1.net"

  s.has_rdoc = false
  s.extensions = 'ext/extconf.rb'
  s.bindir = 'bin'
  s.executables << 'pprof.rb'

  # ruby -rpp -e' pp `git ls-files | grep -v examples`.split("\n").sort '
  s.files = `git ls-files`.split("\n").reject{ |f| f =~ /^examples/ }
  
  s.add_dependency "debase-ruby_core_source"
end
