#!/usr/bin/env ruby
# -*- encoding: binary -*-
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
require 'test/test_helper'
require 'digest/md5'
require 'net/http'
require 'stringio'

class TestHTTPChunkedPut < Test::Unit::TestCase
  def setup
    @tmpdir = Dir.mktmpdir('cmogstored-httpchunkedput-test')
    Dir.mkdir("#@tmpdir/dev666")
    Dir.mkdir("#@tmpdir/dev6")
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
      # $stderr.reopen(@err)
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

  def test_put
    Net::HTTP.start(@host, @port) do |http|
      4.times do |i|
        put = Net::HTTP::Put.new("/dev666/foo#{i}")
        body = StringIO.new("BODY!")
        put.content_type = "application/octet-stream"
        put["Transfer-Encoding"] = "chunked"
        put.body_stream = body
        resp = http.request(put)
        assert_equal 201, resp.code.to_i, "i=#{i}"
        assert_equal "BODY!", IO.read("#@tmpdir/dev666/foo#{i}")
      end
    end
  end

  def test_single_write
    req = "PUT /dev666/zz HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "\r\n" \
          "5\r\nabcde\r\n0\r\n\r\n"
    @client.write(req)
    check_abcde
  end

  def test_edge_finder_no_trailer
    edge_finder("\r\n")
  end

  def test_edge_finder_no_trailer_with_get
    edge_finder("\r\nGET")
  end

  def test_edge_finder_full_trailer
    edge_finder("Foo: Bar\r\n\r\n")
  end

  def test_edge_finder_full_trailer_with_get
    edge_finder("Foo: Bar\r\n\r\nGET")
  end

  def test_edge_finder_pipelined
    edge_finder(:another)
  end

  def edge_finder(suf)
    expect = "HTTP/1.1 200 OK\r\n" \
             "Status: 200 OK\r\n" \
             "Date: #{EPOCH}\r\n" \
             "Last-Modified: #{EPOCH}\r\n" \
             "Content-Length: 5\r\n" \
             "Content-Type: application/octet-stream\r\n" \
             "Accept-Ranges: bytes\r\n" \
             "Connection: close\r\n\r\nabcde"
    base = "PUT /dev666/zz HTTP/1.1\r\n" \
           "Host: #@host:#@port\r\n" \
           "Transfer-Encoding: chunked\r\n" \
           "\r\n" \
           "5\r\nabcde\r\n0\r\n"

    suf = another = "\r\n#{base.sub(%r{/zz}, "/yy")}\r\n" if suf == :another
    req = base + suf
    (1..(req.size-1)).each do |i|
      @client.write(req[0,i])
      t_yield
      @client.write(req[i, req.size])
      check_abcde("i=#{i} size=#{req.size}")
      case suf
      when another
        t_yield
        check_abcde("i=#{i} size=#{req.size} (again)")
        assert_equal "abcde", IO.read("#@tmpdir/dev666/yy"), "i=#{i}"
      when /GET\z/
        t_yield
        @client.write " /dev666/zz HTTP/1.0\r\n\r\n"
        got = @client.read(expect.size)
        replace_dates!(got)
        assert_equal expect, got, "i=#{i}"
      end
      @client.close
      @client = TCPSocket.new(@host, @port)
    end
  end

  def check_abcde(msg=nil)
    expect = "HTTP/1.1 201 Created\r\n" \
      "Status: 201 Created\r\n" \
      "Date: #{EPOCH}\r\n" \
      "Content-Length: 0\r\n" \
      "Content-Type: text/plain\r\n" \
      "Connection: keep-alive\r\n\r\n"
    resp = @client.readpartial(expect.size)
    assert_kind_of String, resp
    replace_dates!(resp)
    assert_equal expect, resp, msg
    assert_equal "abcde", IO.read("#@tmpdir/dev666/zz")
  end

  def test_boundary_chunk_size
    req = "PUT /dev666/zz HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "\r\n" \
          "5"
    more = "\r\nabcde\r\n0\r\n\r\n"
    @client.write(req)
    t_yield
    @client.write(more)
    check_abcde
  end

  def test_boundary_chunk_data
    req = "PUT /dev666/zz HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "\r\n" \
          "5\r\na"
    more = "bcde\r\n0\r\n\r\n"
    @client.write(req)
    t_yield
    @client.write(more)
    check_abcde
  end

  def test_boundary_chunk_size_last
    req = "PUT /dev666/zz HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "\r\n" \
          "5\r\nabcde\r\n0\r"
    more = "\n\r\n"
    @client.write(req)
    t_yield
    @client.write(more)
    check_abcde
  end

  def test_boundary_chunk_trailer
    req = "PUT /dev666/zz HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "\r\n" \
          "5\r\nabcde\r\n0\r\n"
    more = "\r\n"
    @client.write(req)
    t_yield
    @client.write(more)
    check_abcde
  end

  def test_boundary_chunk_trailer_md5
    req = "PUT /dev666/zz HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "\r\n" \
          "5\r\nabcde\r\n0\r\nContent-MD5:"
    more = " q1a02StAcTrMWviZhdS3hg==\r\n\r\n"
    @client.write(req)
    t_yield
    @client.write(more)
    check_abcde
  end

  def test_boundary_chunk_trailer_md5_pipelined
    req = "PUT /dev666/zz HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "\r\n" \
          "5\r\nabcde\r\n0\r\nContent-MD5:"
    more = " q1a02StAcTrMWviZhdS3hg==\r\n\r\nGET"
    moar = " /dev666/zz HTTP/1.1\r\n" \
           "Host: #@host:#@port\r\n" \
           "\r\n"
    @client.write(req)
    t_yield
    @client.write(more)
    check_abcde
    t_yield
    @client.write(moar)
    expect = "HTTP/1.1 200 OK\r\n" \
             "Status: 200 OK\r\n" \
             "Date: #{EPOCH}\r\n" \
             "Last-Modified: #{EPOCH}\r\n" \
             "Content-Length: 5\r\n" \
             "Content-Type: application/octet-stream\r\n" \
             "Accept-Ranges: bytes\r\n" \
             "Connection: keep-alive\r\n\r\nabcde"
    resp = @client.read(expect.size)
    replace_dates!(resp)
    assert_equal expect, resp
  end

  def test_boundary_pipelined
    req = "PUT /dev666/zz HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "\r\n" \
          "5\r\nabcde\r\n0\r\n\r\nGET"
    more = " /dev666/zz HTTP/1.1\r\n" \
           "Host: #@host:#@port\r\n" \
           "\r\n"
    @client.write(req)
    t_yield
    check_abcde
    @client.write(more)
    expect = "HTTP/1.1 200 OK\r\n" \
             "Status: 200 OK\r\n" \
             "Date: #{EPOCH}\r\n" \
             "Last-Modified: #{EPOCH}\r\n" \
             "Content-Length: 5\r\n" \
             "Content-Type: application/octet-stream\r\n" \
             "Accept-Ranges: bytes\r\n" \
             "Connection: keep-alive\r\n\r\nabcde"
    resp = @client.read(expect.size)
    replace_dates!(resp)
    assert_equal expect, resp
  end

  def test_boundary_pipelined_slow_1
    req = "PUT /dev666/zz HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "\r\n" \
          "5\r\nabcde\r\n0"
    more = "\r\n\r\nGET"
    moar = " /dev666/zz HTTP/1.1\r\n" \
           "Host: #@host:#@port\r\n" \
           "\r\n"
    @client.write(req)
    t_yield
    @client.write(more)
    check_abcde
    t_yield
    @client.write(moar)
    expect = "HTTP/1.1 200 OK\r\n" \
             "Status: 200 OK\r\n" \
             "Date: #{EPOCH}\r\n" \
             "Last-Modified: #{EPOCH}\r\n" \
             "Content-Length: 5\r\n" \
             "Content-Type: application/octet-stream\r\n" \
             "Accept-Ranges: bytes\r\n" \
             "Connection: keep-alive\r\n\r\nabcde"
    resp = @client.read(expect.size)
    replace_dates!(resp)
    assert_equal expect, resp
  end

  def test_zero_byte
    req = "PUT /dev666/zero HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Transfer-Encoding: chunked\r\n\r\n" \
          "0\r\n\r\n"
    @client.write(req)
    expect = "HTTP/1.1 201 Created\r\n" \
      "Status: 201 Created\r\n" \
      "Date: #{EPOCH}\r\n" \
      "Content-Length: 0\r\n" \
      "Content-Type: text/plain\r\n" \
      "Connection: keep-alive\r\n" \
      "\r\n"
    resp = @client.readpartial(666)
    replace_dates!(resp)
    assert_equal expect, resp
    assert_equal "", IO.read("#@tmpdir/dev666/zero")
    assert_nil IO.select([@client], nil, nil, 0.1)
  end

  def test_pipelined
    req = 2.times.map do |i|
      "PUT /dev666/#{i} HTTP/1.1\r\n" \
      "Host: #@host:#@port\r\n" \
      "Transfer-Encoding: chunked\r\n" \
      "\r\n" \
      "5\r\nabcd#{i}\r\n0\r\n\r\n"
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

  def test_pipelined_slow
    req = 2.times.map do |i|
      "PUT /dev666/#{i} HTTP/1.1\r\n" \
      "Host: #@host:#@port\r\n" \
      "Transfer-Encoding: chunked\r\n" \
      "\r\n" \
      "5\r\nabcd#{i}\r\n0\r\n\r\n"
    end.join

    req.each_byte do |x|
      @client.write(x.chr)
      t_yield
    end

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

  def test_put_chunk_len_overflow
    max = 0xffffffff << 64
    req = "PUT /dev666/foo HTTP/1.0\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "\r\n" \
          "#{sprintf("%x", max)}\r\n"
    @client.write(req)
    resp = @client.read
    assert_match(%r{\AHTTP/1\.1 507 Insufficient Storage\r\n}, resp)
    assert ! File.exist?("#@tmpdir/dev666/foo")
  end

  def test_content_md5_good
    req = "PUT /dev666/foo HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "Trailer: Content-MD5\r\n" \
          "\r\n" \
          "5\r\nabcde\r\n0\r\n" \
          "Content-MD5: q1a02StAcTrMWviZhdS3hg==\r\n" \
          "\r\n"
    @client.write(req)
    line = @client.gets
    assert_match(%r{\AHTTP/1\.1 201 Created}, line)
    assert_equal "abcde", File.read("#@tmpdir/dev666/foo")
  end

  def test_content_md5_bad
    req = "PUT /dev666/foo HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "Trailer: Content-MD5\r\n" \
          "\r\n" \
          "5\r\nabcd!\r\n0\r\n" \
          "Content-MD5: q1a02StAcTrMWviZhdS3hg==\r\n" \
          "\r\n"
    @client.write(req)
    line = @client.gets
    assert_match(%r{\AHTTP/1\.1 400 Bad Request}, line)
    assert ! File.exist?("#@tmpdir/dev666/foo")
  end

  def test_content_md5_funky_boundary
    # This bug was originally found when I attempted to upload a
    # 198689228 byte file to 127.0.0.1:7600 with Ruby
    # mogilefs-client 3.4.0 using chunked encoding and the
    # Content-MD5 trailer
    md5 = Digest::MD5.new
    extra = "#@host:#@port".size - "127.0.0.1:7600".size
    fid = "0000026374"
    fid = fid[0...(fid.size - extra)]
    req = "PUT /dev6/0/000/026/#{fid}.fid HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Trailer: Content-MD5\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "\r\n"

    bytes = 16384 * 500 + 460
    bytes = 198689228 if ENV["TEST_EXPENSIVE"]
    chunk_len = 16384
    chunk = '*' * chunk_len
    chunk_str = "4000\r\n#{chunk}\r\n"

    @client.write(req)
    while bytes > chunk_len
      bytes -= chunk_len
      @client.write(chunk_str)
      md5.update(chunk)
    end

    final = '!' * bytes
    @client.write("#{sprintf("%x\r\n", bytes)}#{final}\r\n")
    md5.update(final)

    content_md5 = [ md5.digest ].pack('m').rstrip!
    @client.write("0\r\nContent-MD5: #{content_md5}\r\n\r\n")
  ensure
    line = @client.gets
    assert_equal "HTTP/1.1 201 Created\r\n", line, line.inspect
  end

  def test_chunk_extension_small
    req = "PUT /dev666/foo HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "Trailer: Content-MD5\r\n" \
          "\r\n" \
          "5; foo=bar\r\nabcde\r\n0\r\n" \
          "Content-MD5: q1a02StAcTrMWviZhdS3hg==\r\n" \
          "\r\n"
    @client.write(req)
    line = @client.gets
    assert_match(%r{\AHTTP/1\.1 201 Created}, line)
    assert File.exist?("#@tmpdir/dev666/foo")
    assert_equal "abcde", File.read("#@tmpdir/dev666/foo")
  end

  def test_chunk_extension_gigantic
    req = "PUT /dev666/foo HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Transfer-Encoding: chunked\r\n" \
          "\r\n"
    @client.write(req)
    @client.write("5; foo=bar#{'IMABIRD'* 666666}")
    @client.write("\r\nabcde\r\n0\r\n\r\n")
    line = @client.gets
    assert_match(%r{\AHTTP/1\.1 201 Created}, line)
    assert_equal "abcde", File.read("#@tmpdir/dev666/foo")
  end
end
