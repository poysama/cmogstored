#!/usr/bin/env ruby
# -*- encoding: binary -*-
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
require 'test/test_helper'
require 'net/http'
require 'timeout'

class TestUpgrade < Test::Unit::TestCase
  def setup
    @start_pid = $$
    @tmpdir = Dir.mktmpdir('cmogstored-upgrade-test')
    @to_close = []
    @host = TEST_HOST
    http = TCPServer.new(@host, 0)
    @http_port = http.addr[1]
    mgmt = TCPServer.new(@host, 0)
    @mgmt_port = mgmt.addr[1]
    @err = Tempfile.new("stderr")
    pid = Tempfile.new(%w(upgrade .pid))
    @pid_path = pid.path
    @to_close << @err
    @old = "#@pid_path.oldbin"
  ensure
    mgmt.close if mgmt
    http.close if http
  end

  def teardown
    return if $$ != @start_pid
    if @pid_path && File.exist?(@pid_path)
      warn "#@pid_path exists"
      pid = File.read(@pid_path).to_i rescue 0
      if pid > 0 && Process.kill(0, pid)
        warn "Failed to kill #{pid}, Nuking"
        Process.kill(:KILL, pid)
        wait_for_death(pid)
      end
    end
    w = File.read(@err.path).strip
    warn(w) if w.size > 0
    @to_close.each { |io| io.close unless io.closed? }
    FileUtils.rm_rf(@tmpdir)
  end

  def upgrade_prepare_full(wp = nil)
    cmd = [ "cmogstored", "--docroot=#@tmpdir", "--pidfile=#@pid_path",
            "--daemonize", "--maxconns=500",
            "--mgmtlisten=#@host:#@mgmt_port",
            "--httplisten=#@host:#@http_port" ]
    cmd << "--worker-processes=#{wp}" if wp
    tmp_pid = fork do
      $stderr.reopen(@err.path)
      exec(*cmd)
    end
    _, status = Process.waitpid2(tmp_pid)
    assert status.success?, status.inspect

    assert_http_running
    old_pid = assert_pidfile_valid(@pid_path)

    # start the upgrade
    Process.kill(:USR2, old_pid)
    wait_for_pidfile(@old)
    wait_for_pidfile(@pid_path)

    # both old and new should be running
    first_pid = assert_pidfile_valid(@old)
    assert_equal old_pid, first_pid
    assert File.exist?(@pid_path)
    new_pid = assert_pidfile_valid(@pid_path)
    assert new_pid != old_pid
    [ old_pid, new_pid ]
  end

  def test_upgrade_kill(new_sig = :QUIT, wp = nil)
    old_pid, new_pid = upgrade_prepare_full(wp)
    Process.kill(new_sig, new_pid)
    wait_for_death(new_pid)
    Timeout.timeout(30) { sleep(0.01) while File.exist?(@old) }
    wait_for_pidfile(@pid_path)
    orig_pid = assert_pidfile_valid(@pid_path)
    assert_equal old_pid, orig_pid
    Process.kill(:QUIT, orig_pid)
    wait_for_death(orig_pid)
  end

  def test_upgrade_kill_KILL(wp = nil)
    test_upgrade_kill(:KILL, wp)
  end

  def test_upgrade_kill_ABRT(wp = nil)
    test_upgrade_kill(:ABRT, wp)
  end

  def test_upgrade_normal(wp = nil)
    old_pid, new_pid = upgrade_prepare_full(wp)
    Process.kill(:QUIT, old_pid)
    wait_for_death(old_pid)
    Process.kill(0, new_pid)
    assert_http_running
    mgmt = TCPSocket.new(TEST_HOST, @mgmt_port)
    mgmt.write "shutdown\n"
    Timeout.timeout(30) { assert_nil mgmt.gets }
    wait_for_death(new_pid)
  end

  def test_upgrade_kill_KILL_worker_process
    test_upgrade_kill_KILL(1)
  end

  def test_upgrade_kill_ABRT_worker_process
    test_upgrade_kill_ABRT(1)
  end

  def test_upgrade_kill_QUIT_worker_process
    test_upgrade_kill(:QUIT, 1)
  end

  def test_upgrade_normal_worker_process
    test_upgrade_normal(1)
  end

  def wait_for_death(pid, seconds = 30)
    Timeout.timeout(seconds) do
      begin
        Process.kill(0, pid)
        sleep(0.01)
      rescue Errno::ESRCH
        break
      end while true
    end
  end

  def wait_for_pidfile(pidf, seconds = 30)
    Timeout.timeout(seconds) do
      begin
        if File.exist?(pidf)
          nr = File.read(pidf)
          return if nr.to_i > 0
        end
        sleep 0.01
      rescue Errno::ENOENT
        sleep 0.01
      end while true
    end
  end

  def assert_http_running
    # make sure process is running and signals are ready
    Net::HTTP.start(@host, @http_port) do |http|
      req = Net::HTTP::Get.new("/")
      resp = http.request(req)
      assert_kind_of Net::HTTPOK, resp
    end
  end

  def assert_pidfile_valid(path)
    pid = File.read(path).to_i
    assert_operator pid, :>, 0
    pid
  end
end
