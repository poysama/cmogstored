#!/usr/bin/env ruby
# -*- encoding: binary -*-
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
require 'test/test_helper'
require 'digest/md5'
require 'net/http'
require 'stringio'
require 'io/wait'

class TestHTTPPutSlow < Test::Unit::TestCase
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
    $stderr.write(@err.read)
    assert status.success?, status.inspect
  end

  def test_put_one_way
    req = "PUT /dev666/foo HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Content-Length: 0\r\n" \
          "\r\n"
    ok = 0
    begin
      nr = @client.write_nonblock(req)
      if nr == req.size
        ok += 1
      end
    rescue Errno::EAGAIN
      break
    end while true

    created = 0
    while @client.wait(5)
      line = @client.gets
      if line =~ %r{\AHTTP/1\.1 201 Created}
        created += 1
      end
    end
    assert_equal ok, created
    @client.close
  end

  def test_zero_byte
    req = "PUT /dev666/zero HTTP/1.1\r\n" \
          "Host: #@host:#@port\r\n" \
          "Content-Length: 0\r\n" \
          "\r\n"
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
    assert_equal "", IO.read("#@tmpdir/dev666/zero")
    assert_nil IO.select([@client], nil, nil, 1)
  end
end
