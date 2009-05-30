#!/bin/sh
# XXX assuming that we're going to be run from inside the gem installation
`dirname $0`/pprof `which ruby` $*