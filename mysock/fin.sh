[ $# -lt 5 ] && { echo "`basename $0` <src> <dst> <sport> <dport> <dev> [<pcap>]" >&2; exit 1; }
src=$1
dst=$2
sport=$3
dport=$4
dev=$5
file=${6-"./sock.pcap"}

dir=`dirname $0`
cd $dir

sudo tcpdump -w $file -i $dev &
pid=$!
./sock -v -b$sport $dst $dport
sleep 3
sudo su - root -c "kill -TERM $pid"
wait

cmd=`sudo tcpdump -n -r $file -S -vv "dst host $dst and tcp[13:1] & 1 != 0" 2>&1 | sed '1d' | \
tr ',' '\n' | sed 's/^ *//' | awk '{
   if (/^id/) {
      id = $2
   } else if (/^cksum/) {
      split($0, a)
      if ((n = match(a[4], ":")) > 0) {
         seq=substr(a[4], 1, n-1)
         ack=a[6]
	 exit
      }
   } else if (/^seq/) {
      seq = $2
   } else if (/^ack/) {
      ack = $2
   }
}END{
   printf "sudo sock -o \"fin:%s:%s:%s:%s\" -b%s %s %s", src,seq,ack,id,sport,dst,dport
}' src=$src sport=$sport dst=$dst dport=$dport`

echo $cmd
eval $cmd
