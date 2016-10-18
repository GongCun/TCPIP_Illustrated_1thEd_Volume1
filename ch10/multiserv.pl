#!/usr/bin/perl -w
# $Id$
use strict;
use Socket;
use IO::Handle;

my $socket;
my $addr;
my $buf = "";

if ($#ARGV != 1) {
        print STDERR "Usage: $0 <multicast ip> <port>\n";
        exit 1
}

my $mcast_addr = inet_aton($ARGV[0]);
my $port = $ARGV[1];
my $local_addr = INADDR_ANY;
my $ip_mreq = $mcast_addr . $local_addr;

socket($socket, PF_INET, SOCK_DGRAM, getprotobyname('UDP')) or die "socket: $!";

setsockopt($socket, SOL_SOCKET, SO_REUSEADDR, 1) or die "Can't reuse the socket: $!";

$addr = sockaddr_in($port, $local_addr);
bind($socket, $addr);

my $ip_level = getprotobyname('IP');
# grep -e IP_ADD_MEMBERSHIP -e IP_MULTICAST_LOOP /usr/include/netinet/in.h | grep '^#define'
my $IP_ADD_MEMBERSHIP = 12;
my $IP_MULTICAST_LOOP = 11;

setsockopt($socket, $ip_level, $IP_ADD_MEMBERSHIP, $ip_mreq) or die "Can't join group: $!";
setsockopt($socket, $ip_level, $IP_MULTICAST_LOOP, 0) or die "Can't deactive the message to loop back: $!";


while (1) {
	defined(my $recvfrom = recv($socket, $buf, 1024, 0))
		|| die "recv: $!";
        my($port, $ipaddr) = sockaddr_in($recvfrom);
        printf "from %s: %s", inet_ntoa($ipaddr), $buf;
}
