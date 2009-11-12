#!/usr/bin/env ruby
require 'rbconfig'
cmd = "#{File.dirname(__FILE__)}/pprof #{Config::CONFIG['bindir']}/#{Config::CONFIG['ruby_install_name']} #{ARGV.join(" ")}"
exec(cmd)
