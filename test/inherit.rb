#!/usr/bin/env ruby
# -*- encoding: binary -*-
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
require 'test/test_helper'
require 'net/http'

class TestInherit < Test::Unit::TestCase
  def setup
    @tmpdir = Dir.mktmpdir('cmogstored-inherit-test')
    @to_close = []
    @host = TEST_HOST
    @srv = TCPServer.new(@host, 0)
    @to_close << @srv
    @port = @srv.addr[1]
    @err = Tempfile.new("stderr")
    @pid = nil
    @exec_fds = {}
  end

  # Ruby 1.8 did not have the close_on_exec= method, but it
  # also defaulted to close_on_exec=false (same as POSIX)
  # 2.0.0 will be the first release with close_on_exec=true
  # by default, but 1.9 already added the close_on_exec= method
  def maybe_cloexec(io, val)
    if io.respond_to?(:close_on_exec=)
      io.close_on_exec = val
      @exec_fds[io.fileno] = io
    end
  end

  # FD inheritance is explicit in Ruby 2.0.0
  def exec(*cmd)
    cmd << @exec_fds if @exec_fds.size > 0
    Process.exec(*cmd)
  end

  def teardown
    if @pid
      Process.kill(:QUIT, @pid) rescue nil
      _, status = Process.waitpid2(@pid)
      @err.rewind
      $stderr.write(@err.read)
      assert status.success?, status.inspect
    end
    @to_close.each { |io| io.close unless io.closed? }
    FileUtils.rm_rf(@tmpdir)
  end

  def test_inherit_bad
    cmd = [ "cmogstored", "--docroot=#@tmpdir", "--httplisten=#@host:#@port",
            "--maxconns=100" ]
    pid = fork do
      r, w = IO.pipe
      maybe_cloexec(r, false)
      maybe_cloexec(w, false)
      $stderr.reopen(@err)
      ENV["CMOGSTORED_FD"] = "#{r.fileno}"
      exec(*cmd)
    end
    _, status = Process.waitpid2(pid)
    assert ! status.success?, status.inspect
    @err.rewind
    assert_match(/getsockname.*failed/, @err.read)
  end

  def test_inherit
    cmd = [ "cmogstored", "--docroot=#@tmpdir", "--httplisten=#@host:#@port",
            "--maxconns=100" ]
    vg = ENV["VALGRIND"] and cmd = vg.split(/\s+/).concat(cmd)
    drop = TCPServer.new(@host, 0)
    @to_close << drop

    @pid = fork do
      maybe_cloexec(drop, false)
      maybe_cloexec(@srv, false)
      ENV["CMOGSTORED_FD"] = "#{@srv.fileno},#{drop.fileno}"
      exec(*cmd)
    end

    # just ensure HTTP works after being inherited
    Net::HTTP.start(@host, @port) do |http|
      [ Net::HTTP::Get, Net::HTTP::Head ].each do |meth|
        resp = http.request(meth.new("/"))
        assert_kind_of Net::HTTPOK, resp
      end
    end

    # ensure inherited FDs get closed if they're unused
    drop_port = drop.addr[1]

    # still works since drop is open in _this_ process
    c = TCPSocket.new(@host, drop_port)
    assert_instance_of(TCPSocket, c)
    @to_close << c

    # drop is no longer valid after this:
    drop.close
    assert_raises(Errno::ECONNREFUSED) { TCPSocket.new(@host, drop_port) }
  end

  def test_inherit_fake
    cmd = [ "cmogstored", "--docroot=#@tmpdir", "--httplisten=#@host:#@port",
            "--maxconns=100" ]
    @srv.close
    pid = fork do
      ENV["CMOGSTORED_FD"] = "666"
      $stderr.reopen(@err)
      exec(*cmd)
    end
    _, status = Process.waitpid2(pid)
    assert ! status.success?, status.inspect
  end

  def test_inherit_bogus
    cmd = [ "cmogstored", "--docroot=#@tmpdir", "--httplisten=#@host:#@port",
            "--maxconns=100" ]
    pid = fork do
      maybe_cloexec(@srv, false)
      ENV["CMOGSTORED_FD"] = "#{@srv.fileno};"
      $stderr.reopen(@err)
      exec(*cmd)
    end
    _, status = Process.waitpid2(pid)
    @err.rewind
    assert_match(/invalid byte: 59$/, @err.read)
    assert ! status.success?, status.inspect
  end

  def test_inherit_high
    cmd = [ "cmogstored", "--docroot=#@tmpdir", "--httplisten=#@host:#@port",
            "--maxconns=100" ]
    pid = fork do
      maybe_cloexec(@srv, false)
      fd = 0xffffffffffffffffffffffffff.to_s
      ENV["CMOGSTORED_FD"] = "#{@srv.fileno},#{fd}"
      $stderr.reopen(@err)
      exec(*cmd)
    end
    _, status = Process.waitpid2(pid)
    assert ! status.success?, status.inspect
    @err.rewind
    assert_match(/failed to parse/, @err.read)
  end
end
