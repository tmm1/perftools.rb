# perftools.rb

    google-perftools for ruby code
    (c) 2010 Aman Gupta (tmm1)
    http://www.ruby-lang.org/en/LICENSE.txt

## Usage (in a webapp)

  Use [rack-perftools_profiler](https://github.com/bhb/rack-perftools_profiler):

    require 'rack/perftools_profiler'
    config.middleware.use ::Rack::PerftoolsProfiler, :default_printer => 'gif'

  Simply add `profile=true` to profile a request:

    curl -o 10_requests_to_homepage.gif "http://localhost:3000/homepage?profile=true&times=10"

## Usage (from Ruby)

  Run the profiler with a block:

    require 'perftools'
    PerfTools::CpuProfiler.start("/tmp/add_numbers_profile") do
      5_000_000.times{ 1+2+3+4+5 }
    end

  Start and stop the profiler manually:

    require 'perftools'
    PerfTools::CpuProfiler.start("/tmp/add_numbers_profile")
    5_000_000.times{ 1+2+3+4+5 }
    PerfTools::CpuProfiler.stop

## Usage (externally)

  Profile an existing ruby application without modifying it:

    $ CPUPROFILE=/tmp/my_app_profile \
      RUBYOPT="-r`gem which perftools | tail -1`" \
      ruby my_app.rb

## Profiler Modes

The profiler can be run in one of many modes, set via an environment
variable before the library is loaded:

  * `CPUPROFILE_REALTIME=1`

    Use walltime instead of cputime profiling. This will capture all time spent in a method, even if it does not involve the CPU.

    For example, `sleep()` is not expensive in terms of cputime, but very expensive in walltime. walltime will also show functions spending a lot of time in network i/o.

  * `CPUPROFILE_OBJECTS=1`

    Profile object allocations instead of cpu/wall time. Each sample represents one object created inside that function.

  * `CPUPROFILE_METHODS=1`

    Profile method calls. Each sample represents one method call made inside that function.

The sampling interval of the profiler can be adjusted to collect more
(for better profile detail) or fewer samples (for lower overhead):

  * `CPUPROFILE_FREQUENCY=500`

    Default sampling interval is 100 times a second. Valid range is 1-4000

## Reporting

    pprof.rb --text /tmp/add_numbers_profile

    pprof.rb --pdf /tmp/add_numbers_profile > /tmp/add_numbers_profile.pdf

    pprof.rb --gif /tmp/add_numbers_profile > /tmp/add_numbers_profile.gif

    pprof.rb --callgrind /tmp/add_numbers_profile > /tmp/add_numbers_profile.grind
    kcachegrind /tmp/add_numbers_profile.grind

    pprof.rb --gif --focus=Integer /tmp/add_numbers_profile > /tmp/add_numbers_custom.gif

    pprof.rb --text --ignore=Gem /tmp/my_app_profile


  For more options, see [pprof documentation](http://google-perftools.googlecode.com/svn/trunk/doc/cpuprofile.html#pprof)


### Examples

#### pprof.rb --text

    Total: 1735 samples
        1487  85.7%  85.7%     1735 100.0% Integer#times
         248  14.3% 100.0%      248  14.3% Fixnum#+

#### pprof.rb --gif

  * Simple [require 'rubygems'](http://perftools-rb.rubyforge.org/examples/rubygems.gif) profile

  * Comparing redis-rb [with](http://perftools-rb.rubyforge.org/examples/redis-rb.gif) and [without](http://perftools-rb.rubyforge.org/examples/redis-rb-notimeout.gif) SystemTimer based socket timeouts

  * [Sinatra](http://perftools-rb.rubyforge.org/examples/sinatra.gif) vs. [Merb](http://perftools-rb.rubyforge.org/examples/merb.gif) vs. [Rails](http://perftools-rb.rubyforge.org/examples/rails.gif)

  * C-level profile of EventMachine + epoll + Ruby threads [before](http://perftools-rb.rubyforge.org/examples/eventmachine-epoll+nothreads.gif) and [after](http://perftools-rb.rubyforge.org/examples/eventmachine-epoll+threads.gif) a [6 line EM bugfix](http://timetobleed.com/6-line-eventmachine-bugfix-2x-faster-gc-1300-requestssec/)

  * C-level profile of a [ruby/rails vm](http://perftools-rb.rubyforge.org/examples/ruby_interpreter.gif)
    * 12% time spent in re_match_exec because of excessive calls to rb_str_sub_bang by Date.parse


## Installation

  Just install the gem, which will download, patch and compile google-perftools for you:

    sudo gem install perftools.rb

  Or build your own gem:

    git clone git://github.com/tmm1/perftools.rb
    cd perftools.rb
    gem build perftools.rb.gemspec
    gem install perftools.rb


  You'll also need graphviz to generate call graphs using dot:

    sudo brew    install graphviz ghostscript # osx
    sudo apt-get install graphviz ghostscript # debian/ubuntu

  If graphviz fails to build on OSX Lion, you may need to recompile libgd, [see here](https://github.com/mxcl/homebrew/issues/6645#issuecomment-1806807)

## Advantages over ruby-prof

* Sampling profiler

  * perftools samples your process using setitimer() so it can be used in production with minimal overhead.


## Profiling the Ruby VM and C extensions

  To profile C code, download and build an unpatched perftools (libunwind or ./configure --enable-frame-pointers required on x86_64).

  Download:

    wget http://google-perftools.googlecode.com/files/google-perftools-1.8.3.tar.gz
    tar zxvf google-perftools-1.8.3.tar.gz
    cd google-perftools-1.8.3

  Compile:

    ./configure --prefix=/opt
    make
    sudo make install

  Profile:

    export LD_PRELOAD=/opt/lib/libprofiler.so                 # for linux
    export DYLD_INSERT_LIBRARIES=/opt/lib/libprofiler.dylib   # for osx
    CPUPROFILE=/tmp/ruby_interpreter.profile ruby -e' 5_000_000.times{ "hello world" } '

  Report:

    pprof `which ruby` --text /tmp/ruby_interpreter.profile


## TODO

  * Add support for heap profiling to find memory leaks (PerfTools::HeapProfiler)
  * Allow both C and Ruby profiling
  * Add setter for the sampling interval


## Resources

  * [GoRuCo 2009 Lightning Talk on perftools.rb](http://goruco2009.confreaks.com/30-may-2009-18-35-rejectconf-various-presenters.html) @ 21:52

  * [Ilya Grigorik's introduction to perftools.rb](http://www.igvita.com/2009/06/13/profiling-ruby-with-googles-perftools/)

  * [Google Perftools](http://code.google.com/p/google-perftools/)

  * [Analyzing profiles and interpreting different output formats](http://google-perftools.googlecode.com/svn/trunk/doc/cpuprofile.html#pprof)
