# -*- encoding: binary -*-
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
ARGV << "-v"
require 'test/unit'
require 'tmpdir'
require 'tempfile'
require 'socket'
require 'fileutils'
$stderr.sync = $stdout.sync = Thread.abort_on_exception = true

TEST_HOST = ENV["TEST_HOST"] ||
            (RUBY_PLATFORM =~ /linux/ ?
             "127.#{rand(256)}.#{rand(256)}.#{rand(256)}" : "127.0.0.1")

# expand relative paths, --daemonize chdirs
path = ENV["PATH"].split(/:/)
ENV["PATH"] = path.map { |x| File.expand_path(x) }.join(":")

EPOCH = "Thu, 01 Jan 1970 00:00:00 GMT"

def replace_dates!(header)
  rv = header.gsub!(/: \w{3}, \d{2} \w{3} \d{4} \d{2}:\d{2}:\d{2} GMT\r\n/,
                    ": #{EPOCH}\r\n")
  assert(rv, header)
end

def t_yield
  100000.times { Thread.pass }
  sleep(ENV["VALGRIND"] ? 0.05 : 0.015)
  100000.times { Thread.pass }
end

def get_client(tries = 300, port = @port)
  begin
    c = TCPSocket.new(@host, port)
    @to_close << c
    c.setsockopt(Socket::IPPROTO_TCP, Socket::TCP_NODELAY, 1)
    return c
  rescue
    if (tries-=1) > 0
      t_yield
    else
      warn "get_client Failed on #@host #{port}"
      raise
    end
  end while true
end

def new_req(meth, path)
  r = Net::HTTP.const_get(meth).new(path)
  r.content_type = "application/octet-stream"
  r
end
