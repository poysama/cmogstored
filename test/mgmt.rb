#!/usr/bin/env ruby
# -*- encoding: binary -*-
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
require 'test/test_helper'
require 'digest/md5'

class TestMgmt < Test::Unit::TestCase
  def setup
    @tmpdir = Dir.mktmpdir('cmogstored-mgmt-test')
    @to_close = []
    @host = TEST_HOST
    srv = TCPServer.new(@host, 0)
    @port = srv.addr[1]
    srv.close
    @err = Tempfile.new("stderr")
    cmd = [ "cmogstored", "--docroot=#@tmpdir", "--mgmtlisten=#@host:#@port",
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

  def t(expect_out, input)
    expect_out += "\r\n"
    input += "\r\n"

    @client.write(input)
    output = @client.gets
    assert_equal expect_out, output
  end

  def test_size
    File.open("#@tmpdir/foo", "wb") { |fp| fp.write(' ' * 3) }
    t("/foo 3", "size /foo")
    t("/missing -1", "size /missing")

    Dir.mkdir("#@tmpdir/dev666")
    File.open("#@tmpdir/dev666/sausage", "wb").close
    t("/dev666/sausage 0", "size /dev666/sausage")
  end

  def test_unknown_command
    t("ERROR: unknown command: hello", "  hello  ")
    t("ERROR: unknown command: hello", "hello")
    t("ERROR: unknown command: size", "size bad")
  end

  def test_invalid_uri
    t("ERROR: uri invalid (contains ..)", "size /..")
    t("ERROR: uri invalid (contains ..)", "size /..")
    t("ERROR: uri invalid (contains ..)", "size /a/..")
    t("ERROR: uri invalid (contains ..)", "size /a/../b")
    longest = "/dev16777215/0/000/000/0123456789.fid"
    t("#{longest} -1", "size #{longest}")

    # non-sensical error, but whatever...
    too_long = longest + "-"
    t("ERROR: uri invalid (contains ..)", "size #{too_long}")
  end

  def test_no_command
    t("", "");
    t("", " ");
  end

  def test_md5
    buf = ' ' * 3
    expect = "/foo MD5=#{Digest::MD5.hexdigest(buf)}"
    File.open("#@tmpdir/foo", "wb") { |fp| fp.write(buf) }
    t(expect, "MD5 /foo")
    t("/missing MD5=-1", "MD5 /missing")
    %w(fsck create_close replicate).each do |reason|
      t(expect, "MD5 /foo #{reason}")
      t("/missing MD5=-1", "MD5 /missing #{reason}")
    end
  end

  def test_sha1
    buf = ' ' * 3
    expect = "/foo SHA-1=#{Digest::SHA1.hexdigest(buf)}"
    File.open("#@tmpdir/foo", "wb") { |fp| fp.write(buf) }
    t(expect, "SHA-1 /foo")
    t("/missing SHA-1=-1", "SHA-1 /missing")
    %w(fsck create_close replicate).each do |reason|
      t(expect, "SHA-1 /foo #{reason}")
      t("/missing SHA-1=-1", "SHA-1 /missing #{reason}")
    end
  end

  def test_continuous_feed
    n = 0
    File.open("#@tmpdir/-1.fid", "wb").close

    200.times do |i|
      File.rename("#@tmpdir/#{i-1}.fid", "#@tmpdir/#{i}.fid")
      n += @client.write("size /#{i}.fid\r\n")
      x = @client.gets
      assert_equal "/#{i}.fid 0\r\n", x
    end

    @client.write "bad command\r\n"
    assert_equal "ERROR: unknown command: bad\r\n", @client.gets
    @client.write "  bad command\r\n"
    assert_equal "ERROR: unknown command: bad\r\n", @client.gets

    @client.write "size /foo bar\r\n"
    assert_equal "ERROR: unknown command: size\r\n", @client.gets

    @client.write "size /foo\r\n"
    assert_equal "/foo -1\r\n", @client.gets
  end

  def test_continuous_feed_mt
    n = 0
    nr = 200
    pfx = rand.to_s
    delay = 0.01
    thr = Thread.new do
      nr.times do |i|
        sleep delay
        x = @client.gets
        assert_equal "/#{pfx}-#{i} -1\r\n", x
      end
    end

    nr.times do |i|
      n += @client.write("size /#{pfx}-#{i}\r\n")
    end
    delay = 0
    thr.join
  end

  def test_monster_line
    assert_raises(Errno::ECONNRESET, Errno::EPIPE) {
      @client.write("size /#{'0' * (400 * 1024)}\r\n")
      @client.gets
    }

    @client = TCPSocket.new(@host, @port)
    @to_close << @client
    t("/missing -1", "size /missing")
  end

  def test_trickle
    "size /missing\r\n".split(//).each do |c|
      @client.write c
      sleep 0.01
    end
    assert_equal "/missing -1\r\n", @client.gets
  end

  def test_client_eof
    t("/missing -1", "size /missing")
    @client.close
    @client = TCPSocket.new(@host, @port)
    @to_close << @client
    t("/missing -1", "size /missing")
  end

  def test_md5_feed
    buf = ' ' * 3
    50.times do |i|
      pfx = (rand * i).to_s[0..rand(30)] + "a"
      expect = "/#{pfx} MD5=#{Digest::MD5.hexdigest(buf)}\r\n"
      File.open("#@tmpdir/#{pfx}", "wb") { |fp| fp.write(buf) }
      nr = 100
      th = Thread.new { @client.write("MD5 /#{pfx}\r\n" * nr) }
      nr.times do |j|
        assert_equal expect, @client.gets, "j=#{j}" # pfx=#{pfx} i=#{i}"
      end
      th.join
    end
  end

  def test_size_huge
    big = 2 * 1024 * 1024 * 1024 * 1020 # 2TB
    sparse_file_prepare(big)
    t("/dev666/sparse-file.fid #{big}", "size /dev666/sparse-file.fid")
  rescue Errno::ENOSPC
  end

  def test_concurrent_md5_fsck
    sparse_file_prepare
    threads = (0..5).map do
      Thread.new do
        c = get_client
        c.write("MD5 /dev666/sparse-file.fid fsck\r\n")
        c.gets
      end
    end
    answers = {}
    threads.each do |thr|
      val = thr.value
      answers[val] = true
      assert_match(%r{\A/dev666/sparse-file\.fid MD5=[a-f0-9]{32}\r\n}, val)
    end
    assert_equal 1, answers.size
  rescue Errno::ENOSPC
  end

  def test_concurrent_md5_fsck_pipelined
    sparse_file_prepare

    threads = (0..5).map do
      Thread.new do
        c = get_client
        c.write("MD5 /dev666/sparse-file.fid fsck\r\n" * 3)
        [ c.gets, c.gets, c.gets ]
      end
    end
    answers = {}
    threads.each do |thr|
      val = thr.value
      val.each do |line|
        answers[line] = true
        assert_match(%r{\A/dev666/sparse-file\.fid MD5=[a-f0-9]{32}\r\n}, line)
      end
    end
    assert_equal 1, answers.size
  rescue Errno::ENOSPC
  end

  # ensure aborted requests do not trigger failure in graceful shutdown
  def test_concurrent_md5_fsck_abort
    sparse_file_prepare
    File.open("#@tmpdir/dev666/sparse-file.fid") do |fp|
      if fp.respond_to?(:advise)
        # clear the cache
        fp.advise(:dontneed)
        req = "MD5 /dev666/sparse-file.fid fsck\r\n"
        starter = get_client
        clients = (1..5).map { get_client }

        starter.write(req)
        threads = clients.map do |c|
          Thread.new(c) do |client|
            client.write(req)
            client.shutdown
            client.close
            :ok
          end
        end
        threads.each { |thr| assert_equal :ok, thr.value }
        line = starter.gets
        assert_match(%r{\A/dev666/sparse-file\.fid MD5=[a-f0-9]{32}\r\n}, line)
        starter.close
      end
    end
  end

  def test_aio_threads
    tries = 1000
    @client.write "WTF\r\n"
    assert_match(%r{ERROR: unknown command}, @client.gets)
    t_yield # wait for threads to spawn
    taskdir = "/proc/#@pid/task"
    glob = "#{taskdir}/*"
    nr_threads = Dir[glob].size if File.directory?(taskdir)
    @client.write "server aio_threads = 1\r\n"
    assert_equal "\r\n", @client.gets
    if RUBY_PLATFORM =~ /linux/
      assert File.directory?(taskdir), "/proc not mounted on Linux?"
    end
    if File.directory?(taskdir)
      while nr_threads == Dir[glob].size && (tries -= 1) > 0
        sleep(0.1)
      end
      assert nr_threads != Dir[glob].size
    end
    @client.write "server aio_threads=6\r\n"
    assert_equal "\r\n", @client.gets
    @client.write "WTF\r\n"
    assert_match(%r{ERROR: unknown command}, @client.gets)
  end

  def test_iostat_watch
    Dir.mkdir("#@tmpdir/dev666")
    @client.write "watch\n"

    # wait for iostat to catch up
    2.times { assert_kind_of String, @client.gets }
    util = RUBY_PLATFORM =~ /linux/ ? %r{\d+\.\d\d} : %r{\d+(?:\.\d+)?}
    assert_match(/^666\t#{util}\n/, @client.gets)
    assert_equal ".\n", @client.gets
  end if `which iostat 2>/dev/null`.chomp.size != 0 &&
         RUBY_PLATFORM !~ /kfreebsd-gnu/

  def test_iostat_watch_multidir
    Dir.mkdir("#@tmpdir/dev666")
    Dir.mkdir("#@tmpdir/dev999")
    @client.write "watch\n"

    # wait for iostat to catch up
    3.times { assert_kind_of String, @client.gets }
    util = RUBY_PLATFORM =~ /linux/ ? %r{\d+\.\d\d} : %r{\d+(?:\.\d+)?}
    lines = []
    lines << @client.gets
    lines << @client.gets
    assert_match(/^(666|999)\t#{util}\n/, lines[0])
    assert_match(/^(666|999)\t#{util}\n/, lines[1])
    assert_not_equal(lines[0], lines[1])

    assert_equal ".\n", @client.gets
  end if `which iostat 2>/dev/null`.chomp.size != 0 &&
         RUBY_PLATFORM !~ /kfreebsd-gnu/

  def sparse_file_prepare(big = nil)
    Dir.mkdir("#@tmpdir/dev666")
    if nil == big
      big = 1024 * 1024 * 500 # only 500M
      big /= 10 if ENV["VALGRIND"] # valgrind slows us down enough :P
    end
    File.open("#@tmpdir/dev666/sparse-file.fid", "w") do |fp|
      begin
        fp.seek(big - 1)
      rescue Errno::EINVAL, Errno::ENOSPC
        big /= 2
        warn "trying large file size: #{big}"
        retry
      end
      fp.write('.')
    end
  end
end
