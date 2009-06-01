#!/usr/bin/env ruby
cmd = "#{File.dirname(__FILE__)}/pprof #{`which ruby`.strip} #{ARGV.join(" ")}"
exec(cmd)
