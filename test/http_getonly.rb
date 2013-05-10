#!/usr/bin/env ruby
# -*- encoding: binary -*-
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
require 'test/test_helper'
require 'net/http'
require 'stringio'
require 'time'

class TestHTTPGetOnly < Test::Unit::TestCase
  def setup
    @tmpdir = Dir.mktmpdir('cmogstored-http-test')
    FileUtils.mkpath("#@tmpdir/dev666")
    @host = TEST_HOST
    srv = TCPServer.new(@host, 0)
    @port = srv.addr[1]
    srv.close
    @cfg = Tempfile.new("cmogstored-httpget")
    @cfg.puts "docroot = #@tmpdir"
    @cfg.puts "httpgetlisten = #@host:#@port"
    @cfg.puts "maxconns = 666"
    @cfg.flush
    @to_close = []
    @err = Tempfile.new("stderr")
    cmd = [ "cmogstored", "--config", @cfg.path ]
    vg = ENV["VALGRIND"] and cmd = vg.split(/\s+/).concat(cmd)
    @pid = fork {
      #$stderr.reopen(@err)
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

  def test_persistent_connections_with_failed_requests
    File.open("#@tmpdir/dev666/test-file", "wb") { |fp| fp.write "HIHI\n" }
    Net::HTTP.start(@host, @port) do |http|
      10.times do
        resp = http.request(Net::HTTP::Head.new("/dev666/test-file"))
        assert_equal "", resp.body.to_s
        assert_kind_of Net::HTTPOK, resp
        assert_equal "keep-alive", resp["Connection"]

        resp = http.request(Net::HTTP::Get.new("/dev666/test-file"))
        assert_kind_of Net::HTTPOK, resp
        assert_equal "HIHI\n", resp.body
        assert_equal "keep-alive", resp["Connection"]

        resp = http.request(new_req(:Delete, "/dev666/test-file"))
        assert_kind_of Net::HTTPMethodNotAllowed, resp
        assert_equal "keep-alive", resp["Connection"]

        resp = http.request(new_req(:Mkcol, "/dev666/test-dir"))
        assert_kind_of Net::HTTPMethodNotAllowed, resp
        assert_equal "keep-alive", resp["Connection"]
      end
    end
  end

  def test_put_fails
    Net::HTTP.start(@host, @port) do |http|
      10.times do |i|
        put = Net::HTTP::Put.new("/dev666/foo#{i}")
        body = StringIO.new("BODY!")
        put.content_type = "application/octet-stream"
        put.body_stream = body
        put.content_length = body.size
        resp = http.request(put)
        assert_equal "close", resp["Connection"]
        assert_kind_of Net::HTTPMethodNotAllowed, resp
        assert ! File.exist?("#@tmpdir/dev666/foo#{i}")

        resp = http.request(Net::HTTP::Head.new("/dev666/test-file"))
        assert_equal "", resp.body.to_s
        assert_kind_of Net::HTTPNotFound, resp
      end
    end
  end
end
