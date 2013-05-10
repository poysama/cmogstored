#!/usr/bin/env ruby
# -*- encoding: binary -*-
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
require 'test/test_helper'
require 'io/wait'
require 'io/nonblock'

class TestHTTPIdleExpire < Test::Unit::TestCase
  def setup
    @to_close = []
    @host = TEST_HOST

    # skip tests under libkqueue, different versions may use more or less
    # descriptors in the background.  The functionality tested in this
    # file only works in Linux, and anybody running this under Linux
    # will want to use epoll directly instead of libkqueue.
    cmd_path = %x{which cmogstored}.chomp
    @kq = %x{ldd #{cmd_path}}.chomp.split(/\n/).grep(/libkqueue/)[0]
    return if @kq

    @tmpdir = Dir.mktmpdir('cmogstored-http-test')

    srv = TCPServer.new(@host, 0)
    @http_port = srv.addr[1]
    srv.close

    srv = TCPServer.new(@host, 0)
    @mgmt_port = srv.addr[1]
    srv.close

    cmd = [ "cmogstored", "--docroot=#@tmpdir",
            "--httplisten=#@host:#@http_port",
            "--mgmtlisten=#@host:#@mgmt_port",
            "--maxconns=500" ]
    @err = Tempfile.new("stderr")
    nofile = 20

    @pid = fork {
      Process.setrlimit(Process::RLIMIT_NOFILE, nofile)
      $stderr.reopen(@err)
      @err.close
      exec(*cmd)
    }
    @idle_timeout = 5
  end

  def test_idle_expire
    return if @kq

    # keep connecting until we don't get a response
    clients = []
    loop do
      c = get_client(300, @http_port)
      c.nonblock = true
      c.write "GET / HTTP/1.1\r\nHost: #@host:#@http_port\r\n\r\n"
      if c.wait(1)
        clients << c
      else # not accepted
        c.close
        break
      end
    end

    # drain the sockets
    clients.each do |c|
      while line = c.gets
        break if line == "\r\n"
      end
      c.write("G") # start the next request
    end

    # wait for timeouts to be valid
    sleep(@idle_timeout + 1)

    # start new clients to trigger keepalive expiry
    new_clients = clients.map do
      c = get_client(1, @http_port)
      c.nonblock = true
      assert_kind_of TCPSocket, c
      c
    end

    clients.each do |c|
      assert_equal [[c],[],[]], IO.select([c], nil, nil, 1)
      assert_nil c.gets, "expect nil (EOF)"
      c.close
    end
    new_clients.each do |c|
      c.write("GET / HTTP/1.0\r\n\r\n")
      assert c.wait(5)
      buf = c.read
      assert_match(%r{\AHTTP/1\.1 }, buf)
      assert_match(%r{\r\n\r\n\z}, buf)
      c.close
    end
  end

  def teardown
    return if @kq

    Process.kill(:QUIT, @pid) rescue nil
    _, status = Process.waitpid2(@pid)
    @to_close.each { |io| io.close unless io.closed? }
    FileUtils.rm_rf(@tmpdir)
    @err.rewind
    $stderr.write(@err.read)
    assert status.success?, status.inspect
  end
end if RUBY_PLATFORM =~ /linux/
