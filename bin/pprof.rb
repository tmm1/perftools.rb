#!/usr/bin/env ruby
require 'rbconfig'
exec(File.join(File.dirname(__FILE__), 'pprof'), File.join(RbConfig::CONFIG['bindir'], RbConfig::CONFIG['ruby_install_name']), *ARGV)
