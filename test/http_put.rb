#!/usr/bin/env ruby
# -*- encoding: binary -*-
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
require 'test/test_helper'
require 'digest/md5'
require 'net/http'
require 'stringio'

class TestHTTPPut < Test::Unit::TestCase
  def setup
    @tmpdir = Dir.mktmpdir('cmogstored-httpput-test')
    Dir.mkdir("#@tmpdir/dev666")
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
    #$stderr.write(@err.read)
    assert status.success?, status.inspect
  end

  def bad_put(path)
    Net::HTTP.start(@host, @port) do |http|
      put = Net::HTTP::Put.new(path)
      put.content_type = "application/octet-stream"
      body = StringIO.new("BODY!")
      put.body_stream = body
      put.content_length = body.size
      return http.request(put)
    end
  end

  def test_put_does_not_create_shallow_dev_dirs
    assert ! File.exist?("#@tmpdir/dev777")
    assert_equal 404, bad_put("/dev777/foo").code.to_i
    assert_equal 400, bad_put("/dev777/foo/").code.to_i
    assert_equal 400, bad_put("/foo").code.to_i
    assert ! File.exist?("#@tmpdir/foo")
    assert ! File.exist?("#@tmpdir/foo/")
    assert ! File.exist?("#@tmpdir/dev777")
  end

  def test_put
    Net::HTTP.start(@host, @port) do |http|
      4.times do |i|
        put = Net::HTTP::Put.new("/dev666/#{i}/foo#{i}")
        body = StringIO.new("BODY!")
        put.content_type = "application/octet-stream"
        put.body_stream = body
        put.content_length = body.size
        resp = http.request(put)
        assert_equal 201, resp.code.to_i
        assert_equal "BODY!", IO.read("#@tmpdir/dev666/#{i}/foo#{i}")
      end
    end
  end

  def test_single_write
    req = "PUT /dev666/zz HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Content-Length: 5\r\n" \
          "\r\n" \
          "abcde"
    @client.write(req)
    expect = "HTTP/1.1 201 Created\r\n" \
      "Status: 201 Created\r\n" \
      "Date: #{EPOCH}\r\n" \
      "Content-Length: 0\r\n" \
      "Content-Type: text/plain\r\n" \
      "Connection: keep-alive\r\n\r\n"
    resp = @client.readpartial(666)
    replace_dates!(resp)
    assert_equal expect, resp
    assert_equal "abcde", IO.read("#@tmpdir/dev666/zz")
  end

  def test_pipelined
    req = 2.times.map do |i|
      "PUT /dev666/#{i} HTTP/1.1\r\n" \
      "Host: #@host:#@port\r\n" \
      "Content-Length: 5\r\n" \
      "\r\n" \
      "abcd#{i}"
    end.join
    @client.write(req)
    expect = "HTTP/1.1 201 Created\r\n" \
      "Status: 201 Created\r\n" \
      "Date: #{EPOCH}\r\n" \
      "Content-Length: 0\r\n" \
      "Content-Type: text/plain\r\n" \
      "Connection: keep-alive\r\n\r\n"
    resp = @client.read(expect.size * 2)
    replace_dates!(resp)
    assert_equal(expect * 2, resp)
    assert_equal "abcd0", IO.read("#@tmpdir/dev666/0")
    assert_equal "abcd1", IO.read("#@tmpdir/dev666/1")
    assert_nil IO.select([@client], nil, nil, 0.1)
  end

  def test_range_put
    req = "PUT /dev666/foo HTTP/1.0\r\n" \
          "Content-Length: 1\r\n" \
          "Content-Range: bytes 5-5/*\r\n" \
          "\r\na"
    @client.write(req)
    resp = @client.readpartial(666)
    assert_match(%r{\AHTTP/1\.1 201 Created\r\n}, resp)
    File.open("#@tmpdir/dev666/foo") do |fp|
      assert_equal("\0\0\0\0\0a", fp.read(fp.stat.size))
    end
  end

  def test_range_put_bad_content_length
    req = "PUT /dev666/foo HTTP/1.0\r\n" \
          "Content-Length: 2\r\n" \
          "Content-Range: bytes 5-5/*\r\n" \
          "\r\na"
    @client.write(req)
    resp = @client.read
    assert_match(%r{\AHTTP/1\.1 400 Bad Request\r\n}, resp)
    assert( ! File.exist?("#@tmpdir/dev666/foo") )
  end

  def test_range_put_bogus_range
    req = "PUT /dev666/foo HTTP/1.0\r\n" \
          "Content-Length: 2\r\n" \
          "Content-Range: bytes 5-1/*\r\n" \
          "\r\na"
    @client.write(req)
    resp = @client.read
    assert_match(%r{\AHTTP/1\.1 400 Bad Request\r\n}, resp)
    assert( ! File.exist?("#@tmpdir/dev666/foo") )
  end

  def test_put_content_len_overflow
    max = 0xffffffff << 64
    req = "PUT /dev666/foo HTTP/1.0\r\n" \
          "Content-Length: #{max}\r\n" \
          "\r\n"
    @client.write(req)
    resp = @client.read
    assert_match(%r{\AHTTP/1\.1 507 Insufficient Storage\r\n}, resp)
    assert( ! File.exist?("#@tmpdir/dev666/foo") )
  end

  def test_put_range_beg_overflow
    max = 0xffffffff << 64
    req = "PUT /dev666/foo HTTP/1.1\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "Content-Range: bytes #{max}-#{max + 1}/*\r\n" \
          "\r\n"
    @client.write(req)
    resp = @client.read
    assert_match(%r{\AHTTP/1\.1 507 Insufficient Storage\r\n}, resp)
    assert( ! File.exist?("#@tmpdir/dev666/foo") )
  end

  def test_put_range_end_overflow
    max = 0xffffffff << 64
    req = "PUT /dev666/foo HTTP/1.1\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "Content-Range: bytes 0-#{max}/*\r\n" \
          "\r\n"
    @client.write(req)
    resp = @client.read
    assert_match(%r{\AHTTP/1\.1 507 Insufficient Storage\r\n}, resp)
    assert( ! File.exist?("#@tmpdir/dev666/foo") )
  end

  def test_put_range_real
    expect = ""
    Net::HTTP.start(@host, @port) do |http|
      beg = 0
      4.times do |i|
        put = Net::HTTP::Put.new("/dev666/foo")
        body = StringIO.new("BODY#{i}\n")
        put.content_type = "application/octet-stream"
        put.body_stream = body
        put["Content-Range"] = "bytes #{beg}-#{beg+body.size-1}/*"
        put.content_length = body.size
        resp = http.request(put)
        assert_equal 201, resp.code.to_i
        expect << body.string
        beg += body.size
      end
    end

    assert_equal expect, IO.read("#@tmpdir/dev666/foo")
  end

  def test_put_bad_content_length
    req = "PUT /dev666/foo HTTP/1.1\r\n" \
          "Content-Length: 123a\r\n" \
          "\r\n"
    @client.write(req)
    resp = @client.read
    assert_match(%r{\AHTTP/1\.1 400 Bad Request\r\n}, resp)
    assert( ! File.exist?("#@tmpdir/dev666/foo") )
  end

  def test_content_md5_good
    req = "PUT /dev666/foo HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Content-Length: 5\r\n" \
          "Content-MD5: q1a02StAcTrMWviZhdS3hg==\r\n" \
          "\r\n" \
          "abcde"
    @client.write(req)
    line = @client.gets
    assert_match(%r{\AHTTP/1\.1 201 Created}, line)
  end

  def test_content_md5_bad
    req = "PUT /dev666/foo HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Content-Length: 5\r\n" \
          "Content-MD5: q1a02StAcTrMWviZhdS3hg==\r\n" \
          "\r\n" \
          "abcd!"
    @client.write(req)
    line = @client.gets
    assert_match(%r{\AHTTP/1\.1 400 Bad Request}, line)
    lines = []
    lines << @client.gets until lines[-1] == "\r\n"
    assert_equal 1, lines.grep(/\AConnection:\s*keep-alive/).size
  end

  def test_put_get_absolute_url
    url = "http://#@host:#@port/dev666/foo"
    req = "PUT #{url} HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Content-Length: 5\r\n" \
          "\r\n" \
          "abcde"
    @client.write(req)
    line = @client.gets
    assert_match(%r{\AHTTP/1\.1 201}, line)

    assert_equal 'abcde', File.read("#@tmpdir/dev666/foo")

    begin
      @client.read_nonblock(1000)
    rescue Errno::EAGAIN
      break
    end while true

    req = "GET #{url} HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Content-Length: 5\r\n" \
          "\r\n" \
          "abcde"
    @client.write(req)
    line = @client.gets
    assert_match(%r{\AHTTP/1\.1 200}, line)
  end

  def test_put_premature_eof
    path = "/dev666/foo"
    url = "http://#@host:#@port#{path}"
    req = "PUT #{url} HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Content-Length: 666\r\n" \
          "\r\n" \
          "abcde"
    @client.write(req)
    @client.shutdown(Socket::SHUT_WR)
    addr = "#{@client.addr[3]}:#{@client.addr[1]}"
    assert_nil @client.read(1)
    assert ! File.exist?("#@tmpdir#{path}")
    buf = File.read(@err.path)
    assert_match(%r{PUT #{path} failed from #{addr} after 5 bytes:}, buf)
    if RUBY_PLATFORM =~ /linux/
      assert_match(%r{last_data_recv=\d+ms from #{addr} for PUT #{path}}, buf)
    end
  end

  def test_put_premature_eof_chunked
    path = "/dev666/foo"
    url = "http://#@host:#@port#{path}"
    req = "PUT #{url} HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "\r\n" \
          "666\r\nf"
    @client.write(req)
    @client.shutdown(Socket::SHUT_WR)
    addr = "#{@client.addr[3]}:#{@client.addr[1]}"
    assert_nil @client.read(1)
    assert ! File.exist?("#@tmpdir#{path}")
    buf = File.read(@err.path)
    assert_match(%r{PUT #{path} failed from #{addr} after 1 bytes:}, buf)
    if RUBY_PLATFORM =~ /linux/
      assert_match(%r{last_data_recv=\d+ms from #{addr} for PUT #{path}}, buf)
    end
  end
end
