RB_TESTS_FAST = test/cmogstored-cfg.rb test/http_dav.rb test/http_range.rb \
  test/http_put.rb test/http_getonly.rb test/inherit.rb test/upgrade.rb
RB_TESTS_SLOW = test/mgmt-usage.rb test/mgmt.rb test/mgmt-iostat.rb \
 test/http.rb test/http_put_slow.rb test/http_chunked_put.rb \
 test/graceful_quit.rb test/http_idle_expire.rb
RB_TESTS = $(RB_TESTS_FAST) $(RB_TESTS_SLOW)
