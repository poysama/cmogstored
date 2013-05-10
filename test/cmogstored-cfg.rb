#!/usr/bin/env ruby
# -*- encoding: binary -*-
require 'test/test_helper'
require 'stringio'
require 'net/http'
require 'timeout'

class TestCmogstoredConfig < Test::Unit::TestCase
  def setup
    @tmpdir = Dir.mktmpdir('cmogstored-cfg-test')
    @host = TEST_HOST
    srv = TCPServer.new(@host, 0)
    @port = srv.addr[1]
    srv.close
    cmd = [ "cmogstored" ]
    vg = ENV["VALGRIND"] and cmd = vg.split(/\s+/).concat(cmd)
    @cmd = cmd
    @pid = nil
    @to_close = []
  end

  def teardown
    @to_close.each { |io| io.close unless io.closed? }
    FileUtils.rm_rf(@tmpdir)
  end

  def pre_kill
    # need to ensure connections are accepted before we can safely kill
    c = get_client
    c.write "HI\n"
    assert_kind_of String, c.gets
    # just because a client gets a response from one thread does not
    # mean they all started.
    t_yield
  end

  def children(pid = @pid)
    pids = `ps --ppid #{pid} --no-headers -o pid 2>/dev/null`.strip
    if RUBY_PLATFORM =~ /linux/
      assert $?.success?, $?.inspect
    end
    pids.split(/\s+/).grep(/\A\d+\z/).map { |x| x.to_i }.sort
  end

  def expand_suppressions!(cmd)
    cmd.map! do |x|
      case x
      when /\A--suppressions=(\S+)\z/
        "--suppressions=#{File.expand_path($1)}"
      else
        x
      end
    end
  end

  def test_worker_processes
    nproc = 2
    @cmd << "--worker-processes=#{nproc}"
    @cmd << "--docroot=#@tmpdir"
    @cmd << "--mgmtlisten=#@host:#@port"
    tmp = Tempfile.new("err")
    @pid = fork do
      $stderr.reopen(tmp)
      exec(*@cmd)
    end
    pre_kill # ensure workers are running
    pids = children

    # kill gently
    pids.each do |pid|
      begin
        Process.kill(:QUIT, pid)
        t_yield
      rescue Errno::ESRCH
        break
      end while true
    end

    pre_kill # ensure workers are respawned

    if pids[0]
      p pids if $VERBOSE
      new_pids = children
      assert pids != new_pids, "new=#{new_pids.inspect} old=#{pids.inspect}"
      pids.each do |pid|
        assert_raises(Errno::ESRCH) { Process.kill(0, pid) }
      end
    end

    # kill brutally
    pids = children
    pids.each do |pid|
      begin
        Process.kill(:KILL, pid)
        t_yield
      rescue Errno::ESRCH
        break
      end while true
    end

    pre_kill # ensure workers are respawned

    if pids[0]
      p pids if $VERBOSE
      new_pids = children
      assert pids != new_pids, "new=#{new_pids.inspect} old=#{pids.inspect}"
      pids.each do |pid|
        assert_raises(Errno::ESRCH) { Process.kill(0, pid) }
      end
    end

    # ensure USR2 (upgrade for master) is no-op for children
    running = children
    10.times do
      running.each do |pid|
        Process.kill(:USR2, pid)
      end
      pre_kill # ensure workers are still running
    end

    Process.kill(:QUIT, @pid)
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
    if new_pids && new_pids[0]
      new_pids.each do |pid|
        assert_raises(Errno::ESRCH) { Process.kill(0, pid) }
      end
    end
  end

  def test_config_file
    tmp = Tempfile.new("cmogstored-cfg-test")
    tmp.write("mgmtlisten = #@host:#@port\n")
    tmp.write("maxconns = 50\n")
    tmp.write("docroot = #@tmpdir")
    tmp.flush
    @cmd << "--config=#{tmp.path}"
    @pid = fork { exec(*@cmd) }
    pre_kill
    Process.kill(:QUIT, @pid)
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
  end

  def test_multi_config_file_fail
    Dir.mkdir("#@tmpdir/a")
    Dir.mkdir("#@tmpdir/b")
    tmp = Tempfile.new("cmogstored-cfg-test")
    tmp.write("mgmtlisten = #@host:#@port\n")
    tmp.write("maxconns = 50\n")
    tmp.write("docroot = #@tmpdir/a")
    tmp.flush
    @cmd << "--config=#{tmp.path}"

    tmp = Tempfile.new("cmogstored-cfg-test")
    tmp.write("mgmtlisten = #@host:#@port\n")
    tmp.write("maxconns = 50\n")
    tmp.write("docroot = #@tmpdir/b")
    tmp.flush
    @cmd << "--config=#{tmp.path}"
    cmd = @cmd.join(' ')
    msg = `#{cmd} 2>&1`
    assert ! $?.success?, $?.inspect
    assert_match(/--multi/, msg)
  end

  def test_stale_pidfile
    pidfile = Tempfile.new("cmogstored.test_stale_pidfile")
    tmppid = fork {}
    _, status = Process.waitpid2(tmppid)
    assert status.success?, status.inspect
    pidfile.puts tmppid
    pidfile.flush
    @cmd << "--docroot=#{Dir.pwd}"
    @cmd << "--maxconns=50"
    @cmd << "--httplisten=#@host:#@port"
    @cmd << "--pidfile=#{pidfile.path}"
    @pid = fork { exec(*@cmd) }
    pre_kill
    Process.kill(:QUIT, @pid)
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
  end

  def test_active_pidfile
    pidfile = Tempfile.new("cmogstored.test_active_pidfile")
    tmperr = Tempfile.new("cmogstored.tmperr")
    tmppid = fork { sleep }
    pidfile.puts tmppid
    pidfile.flush
    @cmd << "--docroot=#{Dir.pwd}"
    @cmd << "--maxconns=50"
    @cmd << "--httplisten=#@host:#@port"
    @cmd << "--pidfile=#{pidfile.path}"
    @pid = fork {
      $stderr.reopen(tmperr)
      exec(*@cmd)
    }
    _, status = Process.waitpid2(@pid)
    assert ! status.success?, status.inspect
    Process.kill(:KILL, tmppid)
    pid, status = Process.waitpid2(tmppid)
    assert_equal tmppid, pid, status.inspect

    tmperr.rewind
    assert_match(/already running on PID: #{tmppid}\n/,
                 tmperr.readlines.grep(/already running/)[0])
  end

  def rand_port
    srv = TCPServer.new(@host, 0)
    port = srv.addr[1]
    srv.close
    port
  end

  def test_multi_config
    http_a = @port
    http_b = rand_port
    mgmt_a = rand_port
    mgmt_b = rand_port
    site_a = Tempfile.new("site_a")
    site_b = Tempfile.new("site_b")
    FileUtils.mkpath(@tmpdir + "/a/dev123")
    FileUtils.mkpath(@tmpdir + "/b/dev456")
    site_a.puts "docroot = #@tmpdir/a"
    site_a.puts "httplisten = #@host:#{http_a}"
    site_a.puts "mgmtlisten = #@host:#{mgmt_a}"
    site_a.flush
    site_b.puts "docroot = #@tmpdir/b"
    site_b.puts "httplisten = #@host:#{http_b}"
    site_b.puts "mgmtlisten = #@host:#{mgmt_b}"
    site_b.flush
    @cmd << "--multi"
    @cmd << "--maxconns=100"
    @cmd << "--config=#{site_a.path}"
    @cmd << "--config=#{site_b.path}"
    @pid = fork do
      exec(*@cmd)
    end

    ma = get_client(300, mgmt_a)
    mb = get_client(300, mgmt_b)

    ma.write "MD5 /dev123/usage\r\n"
    assert_match(%r{\A/dev123/usage MD5=[a-f0-9]{32}\r\n}, ma.gets)
    mb.write "MD5 /dev123/usage\r\n"
    assert_equal "/dev123/usage MD5=-1\r\n", mb.gets

    ma.write "MD5 /dev456/usage\r\n"
    assert_equal "/dev456/usage MD5=-1\r\n", ma.gets
    mb.write "MD5 /dev456/usage\r\n"
    assert_match(%r{\A/dev456/usage MD5=[a-f0-9]{32}\r\n}, mb.gets)

    if RUBY_PLATFORM =~ /linux/
      ma.write "watch\r\n"
      mb.write "watch\r\n"
      2.times do
        assert_kind_of String, ma.gets
        assert_kind_of String, mb.gets
      end
      a_line = ma.gets
      b_line = mb.gets
      assert_match(/^123\t/, a_line)
      assert_match(/^456\t/, b_line)
    end

    Net::HTTP.start(@host, http_a) do |http|
      resp = http.request(Net::HTTP::Get.new("/dev123/usage"))
      assert_kind_of Net::HTTPOK, resp
      resp = http.request(Net::HTTP::Get.new("/dev456/usage"))
      assert_kind_of Net::HTTPNotFound, resp
    end
    Net::HTTP.start(@host, http_b) do |http|
      resp = http.request(Net::HTTP::Get.new("/dev123/usage"))
      assert_kind_of Net::HTTPNotFound, resp
      resp = http.request(Net::HTTP::Get.new("/dev456/usage"))
      assert_kind_of Net::HTTPOK, resp
    end

    Process.kill(:QUIT, @pid)
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
  end

  def test_server_none
    http = rand_port
    @cmd << "--docroot=#@tmpdir"
    @cmd << "--server=none"
    @cmd << "--mgmtlisten=#@host:#@port"
    @cmd << "--httplisten=#@host:#{http}"
    tmp = Tempfile.new('err')
    @pid = fork do
      $stderr.reopen(tmp)
      exec(*@cmd)
    end
    pre_kill
    assert_raises(Errno::ECONNREFUSED) { TCPSocket.new(@host, http) }
    Process.kill(:QUIT, @pid)
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
  end

  def test_server_perlbal
    http = rand_port
    FileUtils.mkpath("#@tmpdir/dev666")
    @cmd << "--docroot=#@tmpdir"
    @cmd << "--server=perlbal"
    @cmd << "--mgmtlisten=#@host:#@port"
    @cmd << "--httplisten=#@host:#{http}"
    tmp = Tempfile.new('err')
    @pid = fork do
      $stderr.reopen(tmp)
      exec(*@cmd)
    end
    pre_kill
    Net::HTTP.start(@host, http) do |c|
      put = Net::HTTP::Put.new("/dev666/foo")
      body = StringIO.new("BODY!")
      put.content_type = "application/octet-stream"
      put.body_stream = body
      put.content_length = body.size
      resp = c.request(put)
      assert_equal 201, resp.code.to_i
      assert_equal "BODY!", IO.read("#@tmpdir/dev666/foo")
    end
    Process.kill(:QUIT, @pid)
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
    tmp.rewind
    assert_match(/W: using internal HTTP for 'server = perlbal'/, tmp.read)
  end

  def test_unsupported_servers
    FileUtils.mkpath("#@tmpdir/dev666")
    @cmd << "--docroot=#@tmpdir"
    @cmd << "--httplisten=#@host:#@port"
    tmp = Tempfile.new("err")

    %w(apache lighttpd nginx).each do |srv|
      cmd = @cmd.dup
      cmd << "--server=#{srv}"
      tmp.rewind
      tmp.truncate(0)
      pid = fork do
        $stderr.reopen(tmp)
        exec(*cmd)
      end
      _, status = Process.waitpid2(pid)
      assert ! status.success?, status.inspect
      tmp.rewind
      assert_match(%r{E: 'server = #{srv}' not understood by cmogstored},
                   tmp.read)
    end
  end

  def test_http_only_no_usage
    @cmd << "--docroot=#@tmpdir"
    @cmd << "--httplisten=#@host:#@port"
    tmp = Tempfile.new('err')
    Dir.mkdir("#@tmpdir/dev666")
    File.open("#@tmpdir/dev666/cmogstored.test", "a") { |fp| fp.write "HI" }
    @pid = fork do
      $stderr.reopen(tmp)
      exec(*@cmd)
    end
    get_client
    Net::HTTP.start(@host, @port) do |http|
      resp = http.request(Net::HTTP::Get.new("/dev666/usage"))
      assert_kind_of Net::HTTPNotFound, resp
      resp = http.request(Net::HTTP::Get.new("/dev666/cmogstored.test"))
      assert_kind_of Net::HTTPOK, resp
      assert_equal "HI", resp.body
    end
    Process.kill(:QUIT, @pid)
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
  end

  def test_worker_processes_mgmt_shutdown
    nproc = 2
    @cmd << "--worker-processes=#{nproc}"
    @cmd << "--docroot=#@tmpdir"
    @cmd << "--mgmtlisten=#@host:#@port"
    tmp = Tempfile.new("err")
    @pid = fork do
      $stderr.reopen(tmp)
      exec(*@cmd)
    end
    mgmt = get_client
    pre_kill
    pids = children
    mgmt.write "shutdown\n"
    assert_nil mgmt.gets
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
    pids.each do |pid|
      100.times do
        begin
          Process.kill(0, pid)
          sleep 0.1
        rescue Errno::ESRCH
          break
        end
      end
      assert_raises(Errno::ESRCH) { Process.kill(0, pid) }
    end
  end

  def PATH_env_has_relpath(badpath)
    @cmd << "--docroot=#@tmpdir"
    @cmd << "--daemonize"
    @cmd << "--mgmtlisten=#@host:#@port"
    tmp = Tempfile.new("err")
    @pid = fork do
      ENV["PATH"] = badpath
      $stderr.reopen(tmp)
      exec(*@cmd)
    end
    _, status = Process.waitpid2(@pid)
    assert ! status.success?, "#{status.inspect} badpath=#{badpath.inspect}"
    tmp.rewind
    lines = tmp.read
    assert_match(/PATH environment contains relative path/, lines)
    assert_match(/relative paths are incompatible with --daemonize/, lines)
  end

  def test_PATH_env_has_relpath
    [
      "#{ENV["PATH"]}::#{ENV["PATH"]}",
      "#{ENV["PATH"]}:",
      "#{ENV["PATH"]}:.",
      ".:#{ENV["PATH"]}",
      ":#{ENV["PATH"]}"
    ].each do |badpath|
      PATH_env_has_relpath(badpath)
    end
  end

  def test_docroot_has_relpath
    @cmd << "--daemonize"
    @cmd << "--mgmtlisten=#@host:#@port"
    tmp = Tempfile.new("err")
    dirname, basename = File.split(@tmpdir)
    @cmd << "--docroot=#{basename}"
    @pid = fork do
      expand_suppressions!(@cmd)
      Dir.chdir(dirname)
      $stderr.reopen(tmp)
      exec(*@cmd)
    end
    _, status = Process.waitpid2(@pid)
    assert ! status.success?, status.inspect
    tmp.rewind
    lines = tmp.read
    assert_match(/docroot=#{basename} must use an absolute/, lines)
    assert_match(/relative paths are incompatible with --daemonize/, lines)
  end

  def test_config_is_relpath
    cfg = Tempfile.new("cfg")
    cfg.puts "mgmtlisten = #@host:#@port"
    cfg.puts "docroot = #@tmpdir"
    cfg.puts "daemonize"
    cfg.flush
    dirname, basename = File.split(cfg.path)
    @cmd << "--config=#{basename}"
    tmp = Tempfile.new("err")
    @pid = fork do
      expand_suppressions!(@cmd)
      Dir.chdir(dirname)
      $stderr.reopen(tmp)
      exec(*@cmd)
    end
    _, status = Process.waitpid2(@pid)
    assert ! status.success?, status.inspect
    tmp.rewind
    lines = tmp.read
    assert_match(/config=#{basename} must use an absolute/, lines)
    assert_match(/relative paths are incompatible with --daemonize/, lines)
  end

  def test_config_has_pidfile_relpath
    pid = Tempfile.new("pid")
    dirname, basename = File.split(pid.path)
    cfg = Tempfile.new("cfg")
    cfg.puts "mgmtlisten = #@host:#@port"
    cfg.puts "docroot = #@tmpdir"
    cfg.puts "pidfile = #{basename}"
    cfg.puts "daemonize"
    cfg.flush
    @cmd << "--config=#{cfg.path}"
    tmp = Tempfile.new("err")
    @pid = fork do
      expand_suppressions!(@cmd)
      Dir.chdir(dirname)
      $stderr.reopen(tmp)
      exec(*@cmd)
    end
    _, status = Process.waitpid2(@pid)
    assert ! status.success?, status.inspect
    tmp.rewind
    lines = tmp.read
    assert_match(/pidfile=#{basename} must use an absolute/, lines)
    assert_match(/relative paths are incompatible with --daemonize/, lines)
  end
end
