#!/usr/bin/env ruby
# -*- encoding: binary -*-
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
require 'test/test_helper'
require 'net/http'

class TestGracefulQuit < Test::Unit::TestCase
  def setup
    @tmpdir = Dir.mktmpdir('cmogstored-graceful-quit-test')
    Dir.mkdir("#@tmpdir/dev666")
    @to_close = []
    @host = TEST_HOST

    srv = TCPServer.new(@host, 0)
    @http = srv.addr[1]
    srv.close

    srv = TCPServer.new(@host, 0)
    @mgmt= srv.addr[1]
    srv.close

    @err = Tempfile.new("stderr")
    cmd = [ "cmogstored", "--docroot=#@tmpdir",
            "--httplisten=#@host:#@http", "--mgmtlisten=#@host:#@mgmt",
            "--maxconns=500" ]
    vg = ENV["VALGRIND"] and cmd = vg.split(/\s+/).concat(cmd)
    @pid = fork {
      #$stderr.reopen(@err)
      @err.close
      exec(*cmd)
    }
  end

  def wait_for_eof(client)
    assert_raises(EOFError) do
      begin
        client.read_nonblock(666)
      rescue Errno::EAGAIN
      end while true
    end
  end

  def teardown
    @to_close.each { |io| io.close unless io.closed? }
    FileUtils.rm_rf(@tmpdir)
  end

  def test_iostat_watcher_shutdown
    client = get_client(666, @mgmt)
    client.write "watch\n"
    2.times { assert_kind_of String, client.gets }
    Process.kill(:QUIT, @pid)

    line = ""
    10.times { line = client.gets or break }
    assert_nil line
    wait_for_eof(client)
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
  end if `which iostat 2>/dev/null`.chomp.size != 0 &&
         RUBY_PLATFORM !~ /kfreebsd-gnu/

  def test_http_get_huge_file
    big = 100 * 1024 * 1024
    File.open("#@tmpdir/dev666/sparse-file.fid", "w") do |fp|
      fp.seek(big - 1)
      fp.write('.')
    end
    client = get_client(666, @http)
    client.write("GET /dev666/sparse-file.fid HTTP/1.1\r\n" \
                 "Host: example.com\r\n\r\n")
    buf = client.readpartial(666)
    _, body = buf.split(/\r\n\r\n/, 2)
    Process.kill(:QUIT, @pid)

    unless ENV["VALGRIND"] || RUBY_PLATFORM =~ /kfreebsd-gnu/
      # changing process title doesn't work when we're using valgrind :<
      expect = /shutting down/
      tries = 10
      begin
        t_yield
        ps_output = `ps #@pid 2>/dev/null`
      end while (tries -= 1) >= 0
      assert_match(expect, ps_output)
    end

    errs = [ Errno::ECONNREFUSED ]
    errs << Errno::EINVAL if RUBY_PLATFORM =~ /freebsd/
    t_yield
    assert_raises(*(errs + [ "http acceptor should shut down" ])) do
      100.times do
        TCPSocket.new(@host, @http).close
        t_yield
      end
    end
    assert_raises(*(errs + [ "mgmt acceptor should shut down" ])) do
      100.times do
        TCPSocket.new(@host, @mgmt).close
        t_yield
      end
    end

    bytes = 0
    buf = ""
    begin
      client.readpartial(666666, buf)
      bytes += buf.size
    rescue EOFError
      break
    end while true
    assert_equal bytes + body.bytesize, big
    assert_raises(EOFError) { client.read_nonblock(666) }
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
  end

  def test_http_trickle_get
    client = get_client(666, @http)
    client.write("GET /")
    t_yield
    Process.kill(:QUIT, @pid)
    t_yield
    client.write("dev666/sparse-file.fid HTTP/1.1\r\n")
    t_yield
    client.write("Host: example.com\r\n\r\n")
    buf = client.readpartial(666)
    head, body = buf.split(/\r\n\r\n/, 2)
    assert_equal "", body
    assert_equal "HTTP/1.1 404 Not Found", head.split(/\r\n/)[0]
    wait_for_eof(client)
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
  end

  def test_http_trickle_get_pipelined
    client = get_client(666, @http)
    client.write("GET /")
    t_yield
    Process.kill(:QUIT, @pid)
    t_yield
    client.write("dev666/trickle-get-pipelined.fid HTTP/1.1\r\n")
    t_yield
    client.write("Host: example.com\r\n\r\nGET /")
    t_yield

    buf = client.readpartial(666)
    head, body = buf.split(/\r\n\r\n/, 2)
    assert_equal "", body
    assert_equal "HTTP/1.1 404 Not Found", head.split(/\r\n/)[0]

    client.write("dev666/trickle-get-pipelined.fid HTTP/1.1\r\n")
    client.write("Host: example.com\r\n\r\n")

    buf = client.readpartial(666)
    head, body = buf.split(/\r\n\r\n/, 2)
    assert_equal "", body
    assert_equal "HTTP/1.1 404 Not Found", head.split(/\r\n/)[0]

    wait_for_eof(client)
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
  end

  def test_http_trickle_put
    client = get_client(666, @http)
    client.write("PUT /dev666/blarg.fid HTTP/1.1\r\n" \
                 "Host: example.com\r\n" \
                 "Content-Length: 5\r\n\r\n")
    t_yield
    Process.kill(:QUIT, @pid)
    "hihi".each_byte do |x|
      t_yield
      client.write(x.chr)
    end

    t_yield
    client.write("\nGET")

    buf = client.readpartial(666)
    head, body = buf.split(/\r\n\r\n/, 2)
    assert_equal "", body
    assert_equal "HTTP/1.1 201 Created", head.split(/\r\n/)[0]
    assert_equal "hihi\n", File.read("#@tmpdir/dev666/blarg.fid")

    client.write(" /dev666/blarg.fid HTTP/1.0\r\n\r\n")
    buf = client.read
    head, body = buf.split(/\r\n\r\n/, 2)
    assert_equal "hihi\n", body
    assert_equal "HTTP/1.1 200 OK", head.split(/\r\n/)[0]

    wait_for_eof(client)
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
  end

  def test_mgmt_md5_huge_file
    big = 100 * 1024 * 1024
    File.open("#@tmpdir/dev666/sparse-file.fid", "w") do |fp|
      fp.seek(big - 1)
      fp.write('.')
    end
    client = get_client(666, @mgmt)
    client.write("size /dev666/sparse-file.fid\r\n")
    assert_equal "/dev666/sparse-file.fid 104857600\r\n", client.gets

    "MD5 /dev666/sparse-file.fid\r\n".each_byte do |x|
      t_yield
      client.write(x.chr)
    end
    Process.kill(:QUIT, @pid)
    t_yield

    buf = client.gets
    assert_nil client.gets
    expect = "/dev666/sparse-file.fid MD5=56bb745c25f52e8378a9ca49b7cfd27f\r\n"
    assert_equal expect, buf
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
  end

  def test_mgmt_md5_huge_file_fsck_queued
    big = 100 * 1024 * 1024
    File.open("#@tmpdir/dev666/sparse-file.fid", "w") do |fp|
      fp.seek(big - 1)
      fp.write('.')
    end

    client = get_client(666, @mgmt)
    n = 5
    client.write("MD5 /dev666/sparse-file.fid fsck")
    t_yield
    client.write("\r\n" << ("MD5 /dev666/sparse-file.fid fsck\r\n" * (n - 1)))
    thr = Thread.new { n.times.map { client.gets } }
    t_yield
    Process.kill(:QUIT, @pid)
    resp = thr.value
    assert_equal n, resp.size
    expect = "/dev666/sparse-file.fid MD5=56bb745c25f52e8378a9ca49b7cfd27f\r\n"
    resp.each do |line|
      assert_equal expect, line
    end
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
  end

  def test_mgmt_size_trickle
    client = get_client(666, @mgmt)
    client.write("size ")
    t_yield
    Process.kill(:QUIT, @pid)
    t_yield
    client.write("/dev666/missing.fid\r\n")

    buf = client.gets
    assert_nil client.gets
    assert_equal "/dev666/missing.fid -1\r\n", buf

    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
  end

  def test_mgmt_shutdown
    client = get_client(666, @mgmt)
    client.write("shutdown\r\n")
    assert_nil client.gets
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
  end

  # for compatibility with mogstored, cmogstored shutdown is always graceful
  def test_mgmt_shutdown_graceful
    client = get_client(666, @mgmt)
    client.write("shutdown graceful\r\n")
    assert_nil client.gets
    _, status = Process.waitpid2(@pid)
    assert status.success?, status.inspect
  end
end
