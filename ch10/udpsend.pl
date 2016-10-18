#!/usr/bin/perl -w
use strict;
use Socket;
use IO::Handle;

my $socket;
my $addr;
my $buf = "";

if ($#ARGV != 1) {
	print STDERR "Usage: $0 <ipaddress> <port>\n";
	exit 1;
}
my $ip = $ARGV[0];
my $port = $ARGV[1];

socket($socket, PF_INET, SOCK_DGRAM, getprotobyname('UDP'))
	|| die "socket: $!";
$addr = sockaddr_in($port, inet_aton($ip));

while (defined(my $line = <STDIN>)) {
	send($socket, $line, 0, $addr);
}
