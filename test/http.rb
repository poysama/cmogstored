#!/usr/bin/env ruby
# -*- encoding: binary -*-
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
require 'test/test_helper'
require 'digest/md5'
require 'net/http'
require 'time'
$stderr.sync = $stdout.sync = Thread.abort_on_exception = true

class TestHTTP < Test::Unit::TestCase
  def setup
    @tmpdir = Dir.mktmpdir('cmogstored-http-test')
    @to_close = []
    @host = TEST_HOST
    srv = TCPServer.new(@host, 0)
    @port = srv.addr[1]
    srv.close
    @err = Tempfile.new("stderr")
    cmd = [ "cmogstored", "--docroot=#@tmpdir", "--httplisten=#@host:#@port",
            "--maxconns=500" ]
    vg = ENV["VALGRIND"] and cmd = vg.split(/\s+/).concat(cmd)
    @pid = fork {
      $stderr.reopen(@err)
      @err.close
      exec(*cmd)
    }
    @client = get_client
  end

  def teardown
    Process.kill(:QUIT, @pid) rescue nil
    _, status = Process.waitpid2(@pid)
    @to_close.each { |io| io.close unless io.closed? }
    FileUtils.rm_rf(@tmpdir)
    @err.rewind
    $stderr.write(@err.read)
    assert status.success?, status.inspect
  end

  def test_slash_for_mogadm_check
    Net::HTTP.start(@host, @port) do |http|
      [ Net::HTTP::Get, Net::HTTP::Head ].each do |meth|
        resp = http.request(meth.new("/"))
        date = Time.httpdate(resp["Date"])
        assert_in_delta Time.now.to_f, date.to_f, 3.0
        assert_kind_of Net::HTTPOK, resp
        assert_equal Time.httpdate(resp["Last-Modified"]), Time.at(0)
      end
    end
  end

  # ensure HTTP HEAD responses get pushed right away
  def test_head_response_time
    File.open("#@tmpdir/somefile", "wb") { |fp| fp.puts "HI\n" }
    Net::HTTP.start(@host, @port) do |http|
      req = Net::HTTP::Head.new("/somefile")
      t0 = Time.now

      http.request(req)
      diff = Time.now - t0
      assert_operator diff, :<, 0.2

      http.request(req)
      diff = Time.now - t0
      assert_operator diff, :<, 0.4
    end
  end

  def test_response_headers
    File.open("#@tmpdir/somefile", "wb") { |fp| fp.puts "HI\n" }
    st = File.stat("#@tmpdir/somefile")
    Net::HTTP.start(@host, @port) do |http|
      [ Net::HTTP::Get, Net::HTTP::Head ].each do |meth|
        resp = http.request(meth.new("/somefile"))
        assert_kind_of Net::HTTPOK, resp
        date = Time.httpdate(resp["Date"])
        assert_in_delta Time.now.to_f, date.to_f, 3.0
        assert_equal Time.httpdate(resp["Last-Modified"]).to_i, st.mtime.to_i
        assert_equal "3", resp["Content-Length"]

        # redundant slashes, mogadm sends them
        resp = http.request(meth.new("//somefile"))
        assert_kind_of Net::HTTPOK, resp
        date = Time.httpdate(resp["Date"])
        assert_in_delta Time.now.to_f, date.to_f, 3.0
        assert_equal Time.httpdate(resp["Last-Modified"]).to_i, st.mtime.to_i
        assert_equal "3", resp["Content-Length"]
      end
    end
  end

  def test_get_huge
    Dir.mkdir("#@tmpdir/dev666")
    big = 100 * 1024 * 1024
    File.open("#@tmpdir/dev666/sparse-file.fid", "w") do |fp|
      fp.seek(big - 1)
      fp.write('.')
    end
    @client.write("GET /dev666/sparse-file.fid HTTP/1.0\r\n\r\n")
    buf = @client.readpartial(600)
    _, body = buf.split(/\r\n\r\n/, 2)

    bytes = IO.copy_stream(@client, "/dev/null")
    assert_equal bytes + body.bytesize, big
  end if IO.respond_to?(:copy_stream)

  def test_dir_forbidden
    Dir.mkdir("#@tmpdir/dev666")
    Net::HTTP.start(@host, @port) do |http|
      resp = http.request(Net::HTTP::Get.new("/dev666"))
      assert_kind_of Net::HTTPForbidden, resp
      resp = http.request(Net::HTTP::Head.new("/dev666"))
      assert_kind_of Net::HTTPForbidden, resp
    end
  end

  def test_trickle_burst
    req = "GET"
    more = " /dev666/test HTTP/1.1\r\nHost: #@host:#@port\r\n\r\n"
    @client.write(req)
    sleep 0.01
    @client.write(more)
    buf = @client.readpartial(600)
    assert_match(%r{\AHTTP/1\.1 404 Not Found\r\n}, buf)
    assert_match(%r{^Content-Length: 0\r\n}, buf)
    assert_match(%r{\r\n\r\n\z}, buf)
  end

  def test_trickle_header
    req = "GET /dev666/test HTTP/1.1\r\nHost: #@host:#@port\r\n\r\n"
    req.split(//).each do |x|
      @client.write(x)
      sleep 0.01
    end
    buf = @client.readpartial(600)
    assert_match(%r{\AHTTP/1\.1 404 Not Found\r\n}, buf)
    assert_match(%r{^Content-Length: 0\r\n}, buf)
    assert_match(%r{\r\n\r\n\z}, buf)

    Dir.mkdir("#@tmpdir/dev666")
    File.open("#@tmpdir/dev666/test", "w") { |fp| fp.write("HI\n") }
    req.split(//).each do |x|
      @client.write(x)
      sleep 0.01
    end

    buf = @client.readpartial(600)
    assert_match(%r{\AHTTP/1\.1 200 OK\r\n}, buf)
    assert_match(%r{^Content-Length: 3\r\n}, buf)
    assert_match(%r{\r\n\r\n}, buf)
  end

  def test_pipelined_small
    req = "GET /dev666/test HTTP/1.1\r\nHost: #@host:#@port\r\n\r\n"
    req *= 2
    @client.write(req)
    buf = ""
    until /\r\n\r\nHTTP.*\r\n\r\n\z/m =~ buf
      buf << @client.readpartial(6666)
    end
    resp = buf.split(/\r\n\r\n/)
    assert_equal 2, resp.size
    resp.each { |x| x.sub!(/^Date:[^\r\n]+\r\n/, "") }
    assert_equal resp[0], resp[1]
  end

  def test_pipelined_large
    req = "GET /dev666/test HTTP/1.1\r\nHost: #@host:#@port\r\n\r\n"
    nr = 10000
    thr = Thread.new do
      sleep 1
      all = ""
      buf = ""
      begin
        all << @client.readpartial(666, buf)
      rescue EOFError
        break
      end while true
      all
    end
    req2 = req * 2
    nr.times { @client.write(req2) }
    @client.write("GET /dev666/test HTTP/1.0\r\n\r\n")
    all = thr.value
    assert_equal((nr * 2) + 1, all.split(/\r\n/).grep(/Status:/).size);
  end

  def test_garbage
    @client.write("size /foo\r\n")
    buf = @client.readpartial(666)
    assert_match(%r{\AHTTP/1\.1 400 Bad Request\r\n}, buf)
    assert_nil(@client.read(666))
  end

  def test_monster_headers
    buf = "GET /hello-world HTTP/1.1\r\n"
    4094.times { buf << "X-Hello: World\r\n" }
    buf << "\r\n"
    assert_operator(buf.bytesize, :<, 0xffff)
    buf.each_line do |line|
      @client.write(line)
      sleep 0.000666
    end
    buf = @client.readpartial(666)
    assert_match %r{\AHTTP/1\.1 404 Not Found\r\n}, buf
  end

  def test_large_request_rejected
    buf = "GET /hello-world HTTP/1.1\r\n"
    4095.times { buf << "X-Hello: World\r\n" }
    buf << "\r\n"
    assert_operator(buf.bytesize, :>=, 0xffff)
    begin
      buf.each_line do |line|
        @client.write(line)
        sleep 0.000666
      end
    rescue Errno::ECONNRESET
    end
    buf = @client.readpartial(666)
    assert_match %r{\AHTTP/1\.1 400 Bad Request\r\n}, buf
  end
end
