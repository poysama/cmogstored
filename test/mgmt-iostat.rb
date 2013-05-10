#!/usr/bin/env ruby
# -*- encoding: binary -*-
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
require 'test/test_helper'

class TestMgmtIostat < Test::Unit::TestCase
  TEST_PATH = File.dirname(__FILE__) + ":#{ENV['PATH']}"
  RUBY = ENV["RUBY"] || "ruby"

  def iostat_mock
    "#{RUBY} #{File.dirname(__FILE__)}/iostat-mock.rb"
  end

  def setup
    @iostat_pid = Tempfile.new('testt-iostat-pid')
    @tmpdir = Dir.mktmpdir('cmogstored-mgmt-iostat-test')
    @host = TEST_HOST
    srv = TCPServer.new(@host, 0)
    @port = srv.addr[1]
    srv.close
    cmd = [ "cmogstored", "--docroot=#@tmpdir", "--mgmtlisten=#@host:#@port",
            "--maxconns=500" ]
    vg = ENV["VALGRIND"] and cmd = vg.split(/\s+/).concat(cmd)
    @cmd = cmd
    @pid = nil
    @to_close = []
  end

  def teardown
    @to_close.each { |io| io.close unless io.closed? }
    if @pid
      Process.kill(:QUIT, @pid) rescue nil
      _, status = Process.waitpid2(@pid)
      assert status.success?, status.inspect
    end
    FileUtils.rm_rf(@tmpdir)
  end

  def test_iostat_fast
    Dir.mkdir "#@tmpdir/dev666"
    @pid = fork do
      ENV["PATH"] = TEST_PATH
      ENV["MOG_IOSTAT_CMD"] = "#{iostat_mock} #{@iostat_pid.path} fast"
      exec(*@cmd)
    end

    og = get_client
    og.write "watch\n"
    threads = []
    nr = RUBY_PLATFORM =~ /linux/ ? 400 : 10
    nr.times do
      threads << Thread.new do
        c = get_client
        assert_equal 6, c.write("watch\n")
        100.times { assert_kind_of(String, c.gets) }
        c
      end
    end

    sleep 1
    threads.each { |th| th.value.close }
    assert og.readpartial(16384)
    sleep 1
    assert og.read_nonblock(512)
    sleep 1
    assert og.read_nonblock(512)
    iostat_pid = @iostat_pid.read.to_i
    if iostat_pid > 0
      Process.kill(:TERM, iostat_pid)
    end
  end

  def test_iostat_bursty1
    iostat_edge_case("bursty1")
  end

  def test_iostat_bursty2
    iostat_edge_case("bursty2")
  end

  def test_iostat_slow
    iostat_edge_case("slow")
  end

  def iostat_edge_case(type)
    Dir.mkdir "#@tmpdir/dev666"
    @pid = fork do
      ENV["PATH"] = TEST_PATH
      ENV["MOG_IOSTAT_CMD"] = "#{iostat_mock} #{@iostat_pid.path} #{type}"
      exec(*@cmd)
    end

    og = get_client
    og.write "watch\n"
    5.times do
      assert_match(/\n$/, x = og.gets)
      p(x) if $DEBUG
    end
    og.close
  end
end
