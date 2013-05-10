#!/usr/bin/perl -w
use strict;
use warnings;
use Test::More;
use IO::Socket::INET;
use Data::Dumper;
use File::Temp qw/tempdir/;
eval { require MogileFS::Client; };
sub quit {
	my ($msg) = @_;
	plan skip_all => $msg;
	exit 0;
}

sub wait_for_socket {
	my ($addr) = @_;
	my %opts = (
		PeerAddr => $addr,
		Proto => "tcp"
	);
	foreach my $i (0..666) {
		my $sock = IO::Socket::INET->new(%opts);
		return if $sock;
		select(undef, undef, undef, 0.05);
	}
	fail("Failed to connect to $addr: $!");
}

quit("Failed to load MogileFS::Client Perl module $@") if $@;
foreach my $exe (qw(mogilefsd mogadm mogdbsetup)) {
	my $found = `which $exe 2>/dev/null`;
	chomp($found);
	quit("`$exe' executable not found") if (length($found) == 0);
}
my $docroot = tempdir(CLEANUP => 1);
my @cmd = qw(mogdbsetup --yes --type=SQLite);
push @cmd, "--dbname=$docroot/mogilefsd.sqlite3";
quit("Failed to setup MogileFS tracker database: $?") if (system(@cmd) != 0);

my ($cmogstored_pid, $mogilefsd_pid);
END {
	if (defined $cmogstored_pid) {
		kill("TERM", $cmogstored_pid);
		waitpid($cmogstored_pid, 0);
		warn "cmogstored exited with: $?" if $?;
	}
	if (defined $mogilefsd_pid) {
		kill("TERM", $mogilefsd_pid);
		waitpid($mogilefsd_pid, 0);
		warn "mogilefsd exited with: $?" if $?;
	}
	0;
}

our $TEST_HOST = $ENV{TEST_HOST} || "127.0.0.1";
my %lopts = (
	LocalAddr => $TEST_HOST,
	LocalPort => 0,
	Proto => "tcp",
	ReuseAddr => 1,
	Listen => 1024
);

my $tracker = IO::Socket::INET->new(%lopts);
my $tracker_port = $tracker->sockport;
my $tracker_host = $tracker->sockhost;
my $mgmt = IO::Socket::INET->new(%lopts);
my $mgmt_host = $mgmt->sockhost;
my $mgmt_port = $mgmt->sockport;
my $http = IO::Socket::INET->new(%lopts);
my $http_host = $http->sockhost;
my $http_port = $http->sockport;
$tracker = $tracker_host . ':' . $tracker_port;

$mogilefsd_pid = fork;
fail("fork for mogilefsd failed: $!") unless defined $mogilefsd_pid;
if ($mogilefsd_pid == 0) {
	$http = $mgmt = undef;
	my $mogilefsd_conf = "$docroot/mogilefsd.conf";
	open(my $fh, ">", $mogilefsd_conf);
	$fh or die "failed to open $mogilefsd_conf: $!";
	print { $fh } <<EOF
db_dsn DBI:SQLite:$docroot/mogilefsd.sqlite3
conf_port $tracker_port
listen $tracker_host
replicate_jobs 1
fsck_jobs 1
query_jobs 1
mogstored_stream_port $mgmt_port
node_timeout 10
EOF
	;
	close($fh) or die "failed to close $mogilefsd_conf: $!";
	exec("mogilefsd", "--config=$mogilefsd_conf");
}

wait_for_socket($tracker);

our $TRACKER = $tracker;
sub mogadm {
	if (system("mogadm", "-t", $TRACKER, @_) != 0) {
		fail("mogadm @cmd failed");
	}
}

mogadm(qw/host add localhost/, "--ip=$http_host", "--port=$http_port");
mogadm(qw/device add localhost dev1/);
mogadm(qw/device add localhost dev2/);
ok(mkdir("$docroot/dev1"), "mkdir $docroot/dev1");
ok(mkdir("$docroot/dev2"), "mkdir $docroot/dev2");

$mgmt = $mgmt_host . ':' . $mgmt_port;
$http = $http_host . ':' . $http_port;
$cmogstored_pid = fork;
fail("fork for cmogstored failed: $!") unless defined $cmogstored_pid;
if ($cmogstored_pid == 0) {
	my @cmd = ("cmogstored", "--docroot=$docroot", "--maxconns=500",
	           "--mgmtlisten=$mgmt", "--httplisten=$http");
	my $vg = $ENV{VALGRIND};
	if ($vg) {
		my @vg = split(/\s+/, $vg);
		@cmd = (@vg, @cmd);
	}
	exec @cmd;
}

wait_for_socket($mgmt);
wait_for_socket($http);

mogadm(qw/host mark localhost alive/);
mogadm(qw/device mark localhost dev1 alive/);
mogadm(qw/device mark localhost dev2 alive/);
mogadm(qw/domain add testdom/);
my $usage_file = "$docroot/dev1/usage";
foreach my $i (0..666) {
	last if (-f $usage_file);
	select(undef, undef, undef, 0.05);
}
ok(-f $usage_file, "usage file: $usage_file not found");

my $mogc = MogileFS::Client->new(domain => "testdom", hosts  => [ $tracker ]);

{
	my $fh = $mogc->new_file("empty");
	ok($fh, "got new file handle");
	ok(close($fh), "closed file");
	my $dataref = $mogc->get_file_data("empty");
	is($$dataref, "", "returned an empty file");
}

{
	my $fh = $mogc->new_file("something");
	ok($fh, "got new file handle");
	ok((print $fh "something"), "printed 'something'");
	ok(close($fh), "close file");
	my $dataref = $mogc->get_file_data("something");
	is($$dataref, "something", "returned a 'something'");
}

# ensure largefile via Content-Range works, since the Perl client
# is the leading implementation of this
{
	my $expect = "";
	my $fh = $mogc->new_file("largefile", "", undef, {largefile => 1});
	ok($fh, "got largefile handle");
	for my $i (0..1000) {
		$expect .= "$i\n";
		print $fh "$i\n" or die "failed to write $!\n";
	}
	ok(close($fh), "close file");
	my $dataref = $mogc->get_file_data("largefile");
	is($$dataref, $expect, "returned expected data");

	my $fid;
	do {
		select(undef, undef, undef, 0.1);
		$fid = $mogc->file_info("largefile");
	} while ($fid->{devcount} < 2);
	my @paths = $mogc->get_paths("largefile");
	my $ua = LWP::UserAgent->new;
	my $a = $ua->get($paths[0]);
	my $b = $ua->get($paths[1]);
	is($a->content, $expect, "mogilefs replication succeeded (a)");
	is($b->content, $expect, "mogilefs replication succeeded (b)");

	$mogc->delete("largefile");

	foreach my $path (@paths) {
		my $r;
		foreach my $i (0..666) {
			$r = $ua->get($path);
			last if ($r->code == 404);
			select(undef, undef, undef, 0.02);
		}
		is($r->code, 404, "deleted key did not get devfid removed");
	}
}

done_testing();
