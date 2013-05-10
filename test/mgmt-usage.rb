#!/usr/bin/env ruby
# -*- encoding: binary -*-
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
require 'test/test_helper'

class TestCmogstoredUsage < Test::Unit::TestCase
  def setup
    @tmpdir = Dir.mktmpdir('cmogstored-usage-test')
    @host = TEST_HOST
    srv = TCPServer.new(@host, 0)
    @port = srv.addr[1]
    srv.close
    @cmd = [ 'cmogstored', '--maxconns=50', '--daemonize',
             "--mgmtlisten=#@host:#@port", "--docroot=#@tmpdir" ]
  end

  def teardown
    FileUtils.rm_rf(@tmpdir)
  end

  def test_usage_daemon
    devdir = "#@tmpdir/dev666"
    pidf = "#@tmpdir/pid"
    Dir.mkdir(devdir)
    usage = "#{devdir}/usage"
    assert ! File.exist?(usage)
    @cmd << "--pidfile=#{pidf}"
    assert(system(*@cmd), proc { $?.to_s })
    sleep(0.05) until File.exist?(usage)
    before = check_usage_file(usage, devdir)
    sleep 1.2
    after = check_usage_file(usage, devdir)
    assert(before[3].split(/ /)[1].to_i <= after[3].split(/ /)[1].to_i)
    pid = File.read(pidf).to_i
    assert(pid > 0 && pid != $$, pid.inspect)
    Process.kill(:TERM, pid)
    begin
      Process.kill(0, pid)
      sleep 0.01
    rescue Errno::ESRCH
      break
    end while true
    assert ! File.exist?(pidf), "pidfile=#{pidf} exists"
  end

  def check_usage_file(usage, devdir)
    lines = File.readlines(usage)
    assert_match(/^available: \d+$/, lines[0])
    assert_match(%r{^device: /\S+$}, lines[1])
    assert_match(/^disk: #{Regexp.escape(devdir)}$/, lines[2])
    assert_match(/^time: \d+\n$/, lines[3])
    assert_match(/^total: \d+\n$/, lines[4])
    assert_match(/^use: \d+%$/, lines[5])
    assert_match(/^used: \d+$/, lines[6])
    lines
  end

  def test_usage_daemon_already_running
    old_err = $stderr.dup
    devdir = "#@tmpdir/dev666"
    pidf = "#@tmpdir/pid"
    Dir.mkdir(devdir)
    usage = "#{devdir}/usage"
    File.open(pidf, "w") { |fp| fp.write("#$$\n") }
    @cmd << "--pidfile=#{pidf}"
    $stderr.reopen("#@tmpdir/err", "ab")
    assert(! system(*@cmd))
    assert_equal "#$$\n", File.read(pidf)
    assert_match(/already running on PID: #$$\n/, File.read("#@tmpdir/err"))
    assert ! File.exist?(usage)
    ensure
      $stderr.reopen(old_err)
      old_err.close
  end
end
