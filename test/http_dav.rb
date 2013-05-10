#!/usr/bin/env ruby
# -*- encoding: binary -*-
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
require 'test/test_helper'
require 'net/http'
$stderr.sync = $stdout.sync = Thread.abort_on_exception = true

class TestHTTPDav < Test::Unit::TestCase
  def setup
    @tmpdir = Dir.mktmpdir('cmogstored-httpdav-test')
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

  def test_mkcol_delete
    Net::HTTP.start(@host, @port) do |http|
      assert ! File.directory?("#@tmpdir/foo")
      assert_equal 400, http.request(new_req(:Mkcol, "/foo")).code.to_i
      assert ! File.directory?("#@tmpdir/foo")

      Dir.mkdir("#@tmpdir/foo")
      File.open("#@tmpdir/foo/a", "w").close

      resp = http.request(new_req(:Delete, "/foo/a"))
      assert_equal 204, resp.code.to_i
      assert ! File.exist?("#@tmpdir/foo/a")

      resp = http.request(new_req(:Delete, "/foo/a"))
      assert_equal 404, resp.code.to_i

      resp = http.request(new_req(:Delete, "/foo"))
      assert_equal 403, resp.code.to_i

      resp = http.request(new_req(:Delete, "../foo"))
      assert_equal 400, resp.code.to_i

      resp = http.request(new_req(:Mkcol, "../foo"))
      assert_equal 400, resp.code.to_i

      resp = http.request(new_req(:Delete, "/"))
      assert_equal 403, resp.code.to_i
      resp = http.request(new_req(:Delete, "//"))
      assert_equal 403, resp.code.to_i
    end
  end
end
