#!/usr/bin/env ruby
# -*- encoding: binary -*-
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
require 'test/test_helper'
require 'net/http'
require 'time'
def Socket.gethostbyname(*args); raise "disabled for test"; end
require 'webrick'
require 'stringio'

class TestHTTPRange < Test::Unit::TestCase
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
    @wblogger = StringIO.new
    wbopts = {
      :Port => 0,
      :AccessLog => [],
      :DocumentRoot => @tmpdir,
      :ServerType => Thread,
      :Logger => WEBrick::Log.new(@wblogger),
      :DoNotReverseLookup => true,
    }
    @webrick = WEBrick::HTTPServer.new(wbopts)
    @webrick.start
    @wport = @webrick.listeners[0].addr[1]
    @client = get_client
  end

  def test_hello
    full = File.open("/dev/urandom") { |fp| fp.read(666) }
    File.open("#@tmpdir/hello", "wb") do |hello|
      assert_equal 666, hello.write(full)
    end
    Net::HTTP.start(@host, @wport) do |wb|
      Net::HTTP.start(@host, @port) do |cm|
        [ Net::HTTP::Get, Net::HTTP::Head ].each do |meth|
          r = meth.new("/hello")
          cm_r = cm.request(r)
          wb_r = wb.request(r)
          assert_equal "666", cm_r["Content-Length"]
          assert_equal "666", wb_r["Content-Length"]
          assert_equal nil, cm_r["Content-Range"]
          assert_equal nil, wb_r["Content-Range"]
          assert_equal wb_r.body, cm_r.body
          assert_equal nil, wb_r.body if Net::HTTP::Head === meth

          r = meth.new("/hello")
          r["Range"] = "bytes=66-70"
          cm_r = cm.request(r)
          wb_r = wb.request(r)
          assert_equal wb_r["Content-Length"], cm_r["Content-Length"]
          assert_equal "5", cm_r["Content-Length"]
          assert_equal wb_r["Content-Range"], cm_r["Content-Range"]
          assert_equal "bytes 66-70/666", cm_r["Content-Range"]
          assert_equal wb_r.body, cm_r.body
          assert_equal nil, wb_r.body if Net::HTTP::Head === meth

          r["Range"] = "bytes=66-"
          cm_r = cm.request(r)
          wb_r = wb.request(r)
          assert_equal wb_r["Content-Length"], cm_r["Content-Length"]
          assert_equal "600", cm_r["Content-Length"]
          assert_equal wb_r["Content-Range"], cm_r["Content-Range"]
          assert_equal "bytes 66-665/666", cm_r["Content-Range"]
          assert_equal wb_r.body, cm_r.body
          assert_equal nil, wb_r.body if Net::HTTP::Head === meth

          r["Range"] = "bytes=-66"
          cm_r = cm.request(r)
          wb_r = wb.request(r)
          assert_equal wb_r["Content-Length"], cm_r["Content-Length"]
          assert_equal "66", cm_r["Content-Length"]
          assert_equal wb_r["Content-Range"], cm_r["Content-Range"]
          assert_equal "bytes 600-665/666", cm_r["Content-Range"]
          assert_equal wb_r.body, cm_r.body
          assert_equal nil, wb_r.body if Net::HTTP::Head === meth

          %w(0 665 123).each do |i|
            r["Range"] = "bytes=#{i}-#{i}"
            cm_r = cm.request(r)
            wb_r = wb.request(r)
            assert_equal wb_r["Content-Length"], cm_r["Content-Length"]
            assert_equal "1", cm_r["Content-Length"]
            assert_equal wb_r["Content-Range"], cm_r["Content-Range"]
            assert_equal "bytes #{i}-#{i}/666", cm_r["Content-Range"]
            assert_equal wb_r.body, cm_r.body
            assert_equal nil, wb_r.body if Net::HTTP::Head === meth
          end

          # webrick doesn't seem to support this, follow nginx
          r["Range"] = "bytes=666-666"
          cm_r = cm.request(r)
          assert_equal "0", cm_r["Content-Length"]
          assert_equal "bytes */666", cm_r["Content-Range"]
          assert_equal nil, cm_r.body if Net::HTTP::Head === meth
          assert_equal 416, cm_r.code.to_i

          # send the entire response as a 200 if the range is too far
          # webrick doesn't seem to support this, follow nginx
          %w(667 6666).each do |i|
            r["Range"] = "bytes=-#{i}"
            cm_r = cm.request(r)
            assert_equal "666", cm_r["Content-Length"]
            assert_nil cm_r["Content-Range"]
            assert_equal nil, cm_r.body if Net::HTTP::Head === meth
            assert_equal 200, cm_r.code.to_i
          end

          # send the entire request if the range is everything
          r["Range"] = "bytes=-666"
          cm_r = cm.request(r)
          wb_r = wb.request(r)
          assert_equal wb_r["Content-Range"], cm_r["Content-Range"]
          assert_equal wb_r["Content-Length"], cm_r["Content-Length"]
          assert_equal "666", cm_r["Content-Length"]
          assert_equal wb_r.body, cm_r.body
          assert_equal nil, wb_r.body if Net::HTTP::Head === meth
          assert_equal wb_r.code.to_i, cm_r.code.to_i

          # out-of-range requests
          %w(666 6666).each do |i|
            r["Range"] = "bytes=#{i}-"
            cm_r = cm.request(r)
            wb_r = wb.request(r)
            assert_equal wb_r.code.to_i, cm_r.code.to_i
            assert_equal 416, cm_r.code.to_i
            assert_equal "bytes */666", cm_r["Content-Range"]
            assert_equal nil, cm_r.body if Net::HTTP::Head === meth
          end

          %w(666 6666).each do |i|
            r["Range"] = "bytes=#{i}-#{i}"
            cm_r = cm.request(r)
            wb_r = wb.request(r)
            assert_equal wb_r.code.to_i, cm_r.code.to_i
            assert_equal 416, cm_r.code.to_i
            assert_equal "bytes */666", cm_r["Content-Range"]
            assert_equal nil, cm_r.body if Net::HTTP::Head === meth
          end

          # bogus ranges
          %w(662-226).each do |bogus|
            r["Range"] = "bytes=#{bogus}"
            cm_r = cm.request(r)
            assert_equal 416, cm_r.code.to_i, "bogus=#{bogus}"
            assert_equal nil, cm_r.body if Net::HTTP::Head === meth
          end

          %w(-5-5 4qasdlkj 323).each do |bogus|
            r["Range"] = "bytes=#{bogus}"
            cm_r = cm.request(r)
            assert_equal 400, cm_r.code.to_i, "bogus=#{bogus}"
          end

          r["Range"] = "bytes=5-6666"
          cm_r = cm.request(r)
          wb_r = wb.request(r)
          assert_equal wb_r.code.to_i, cm_r.code.to_i
          assert_equal 206, cm_r.code.to_i
          assert_equal wb_r["Content-Length"], cm_r["Content-Length"]
          assert_equal "bytes 5-665/666", cm_r["Content-Range"]
          assert_equal nil, cm_r.body if Net::HTTP::Head === meth
          assert_equal wb_r.body, cm_r.body
        end
      end
    end
  end

  def teardown
    Process.kill(:QUIT, @pid) rescue nil
    _, status = Process.waitpid2(@pid)
    @to_close.each { |io| io.close unless io.closed? }
    FileUtils.rm_rf(@tmpdir)
    @err.rewind
    $stderr.write(@err.read)
    assert status.success?, status.inspect
    @webrick.shutdown
  end
end
