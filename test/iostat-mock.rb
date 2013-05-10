#!/usr/bin/env ruby
# -*- encoding: binary -*-
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
usage = "Usage: #$0 PIDFILE (fast|slow)"
$stdout.binmode
$stdout.sync = $stderr.sync = true
$-w = true
require 'stringio'

output = <<EOF
Linux 6.6.6 (faster) 	01/01/1970 	_x86_64_	(666 CPU)

Device:         rrqm/s   wrqm/s     r/s     w/s   rsec/s   wsec/s avgrq-sz avgqu-sz   await  svctm  %util
sda               0.02    71.67    1.05    5.94    31.95   630.78    94.85     0.82  116.28   0.65   0.45
sdb               0.18    11.17    0.30    0.34    76.56    92.09   263.80     0.03   40.68   3.14   0.20
sdc               0.19     0.28    0.32    0.27   125.83    34.07   275.40     0.06  108.49  35.36   2.05
sdd               0.26     0.01    0.42    0.10     5.44    24.15    56.47     0.72 1373.24   1.88   0.10
sde               0.61    13.63    1.14    0.38   213.65   112.08   214.80     0.06   42.54   3.28   0.50

EOF

pidfile = ARGV.shift or abort usage
File.open(pidfile, "wb") { |fp| fp.write("#$$\n") }
n = 666666
begin
  case ARGV.shift
  when "fast"
    n.times { $stdout.write(output * 2) }
  when "slow"
    n.times do
      io = StringIO.new(output)
      while buf = io.read(rand(25) + 1)
        $stdout.write(buf)
        sleep 0.01
      end
      sleep 0.1
    end
  when "bursty1"
    n.times do
      io = StringIO.new(output.dup)
      add =  io.gets
      add << io.gets
      $stdout.write(output + add)
      sleep 2
      while buf = io.read(rand(666) + 1)
        $stdout.write(buf)
      end
    end
  when "bursty2"
    n.times do
      io = StringIO.new(output.dup)
      add = io.read(rand(66) + 6)
      $stdout.write(output + add)
      sleep 2
      while buf = io.read(rand(666) + 1)
        $stdout.write(buf)
        sleep 0.01
      end
      sleep 1
    end
  else
    abort usage
  end
rescue Errno::EPIPE
  exit 0
rescue => e
  abort("#{e.message} (#{e.class})")
end
