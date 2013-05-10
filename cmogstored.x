[SIGNALS]
SIGQUIT - gracefully shutdown the server
SIGUSR2 - upgrade executable on the fly

[UPGRADING]
Upgrading cmogstored on-the-fly is possible since version 1.2.0.
The procedure is close to the one used by nginx users: send SIGUSR2
followed by SIGQUIT to the old process.  Use of SIGWINCH and SIGHUP
are currently not supported (but may be in the future).
Procedures for the nginx upgrade are documented here:
http://wiki.nginx.org/CommandLine#Upgrading_To_a_New_Binary_On_The_Fly

[ENVIRONMENT]
MOG_IOSTAT_CMD - command-line for invoking iostat(1).  This is used
by the sidechannel to report disk utilization to trackers.
Default: "iostat -dx 1 30"

[REPORTING BUGS]
Only bug reports in plain-text email will be read.
You may also report bugs publically to the MogileFS mailing list
mogile@googlegroups.com.

[COPYRIGHT]
Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>.
License: GNU GPL version 3 or later <https://www.gnu.org/licenses/gpl.html>.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

[SEE ALSO]
cmogstored website: http://bogomips.org/cmogstored/README
You can learn more about MogileFS: http://mogilefsd.org/

cmogstored source code is available via git:

	git clone git://bogomips.org/cmogstored.git
