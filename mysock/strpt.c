/* $Id: strpt.c,v 1.1 2017/01/18 06:46:57 root Exp $ */
#include "mysock.h"
#include <nlist.h> /* knlist() */
#include <netinet/ip_var.h>
#define TCPSTATES
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#define TCPTIMERS
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h> /* struct tcpcb */
#include <netinet/tcpip.h>
#define TANAMES
#include <netinet/tcp_debug.h> /* struct tcp_debug */
#define PRUREQUESTS
#include <sys/protosw.h>

#define KMEM "/dev/kmem"
#define LSEEK lseek64
#define OFF_T off64_t
#define N_TCP_NDEBUG	0
#define N_TCP_DEBUG	1
#define N_TCP_DEBX	2
#define K_TCP_NDEBUG	"tcp_ndebug"
#define K_TCP_DEBUG	"tcp_debug"
#define K_TCP_DEBX	"tcp_debx"

#define pf(flag, string) { 				\
	if (th->th_flags & flag) { 			\
		printf("%s%s", cp, string); 		\
		cp = ","; 				\
	} 						\
}

typedef u_longlong_t KA_T; /* kernel memory address type */

struct nlist64 nl[] = {
	{K_TCP_NDEBUG,	0, 0, 0, 0, 0}, /* 0 */
	{K_TCP_DEBUG,	0, 0, 0, 0, 0}, /* 1 */
	{K_TCP_DEBX,	0, 0, 0, 0, 0}, /* 2 */
	{NULL, 0, 0, 0, 0, 0},
};

int Kd; /* KMEM FD */
long tcp_ndebug;
int tcp_debx;
struct tcp_debug *tcp_debug;
static caddr_t tcp_pcb;
static caddr_t *tcp_pcbp;
static void dotrace(caddr_t);
static KA_T var;
static n_time ntime;
static int follow;

void
tcp_trace(short, short, struct tcpcb *, int, void *, struct tcphdr *, int);

int numeric(const void *, const void *);

static size_t kread(KA_T addr, char *buf, size_t len)
{
	if (LSEEK(Kd, (OFF_T)addr, SEEK_SET) == (OFF_T)-1)
		err_sys("LSEEK error");
	return read(Kd, buf, len);
}

static size_t Kread(KA_T addr, char *buf, size_t len)
{
	size_t n;
	if ((n = kread(addr, buf, len)) != len)
		err_quit("kread return %d, expect %d", n, len);
	return n;
}

int main(int argc, char *argv[])
{

	register int i, j;
	register struct tcp_debug *td;
	int npcbs = 0;

	if (argc > 2)
		err_quit("strpt [PCB/ADDR]");

	if (argc == 2 && strcmp(argv[1], "version") == 0) {
#define P(s) (void)fputs(s "\n", stderr);
#include "ver.h"
#undef P
		exit(0);
	}
	if ((Kd = open(KMEM, O_RDONLY)) < 0)
		err_sys("open KMEM error");

	if (knlist((struct nlist *)nl, 3, sizeof(struct nlist64)) < 0)
		err_sys("knlist error");

	Kread((KA_T)nl[N_TCP_NDEBUG].n_value, (char *)&tcp_ndebug, sizeof(tcp_ndebug));

	if ((tcp_debug = calloc(tcp_ndebug, sizeof(struct tcp_debug))) == NULL)
		err_sys("calloc of tcp_debug error");
	if ((tcp_pcbp = calloc(tcp_ndebug, sizeof(caddr_t))) == NULL)
		err_sys("calloc of tcp_pcb error");

	Kread((KA_T)nl[N_TCP_DEBUG].n_value, (char *)&var, sizeof(var));
	Kread((KA_T)var, (char *)tcp_debug, sizeof(struct tcp_debug)*tcp_ndebug);
	Kread((KA_T)nl[N_TCP_DEBX].n_value, (char *)&tcp_debx, sizeof(tcp_debx));

	if (setvbuf(stdout, NULL, _IONBF, 0) != 0)
		err_sys("setvbuf error");

	if (argc == 2) {
		++follow;
		sscanf(argv[1], "%p", (KA_T *)&tcp_pcb);
		dotrace(tcp_pcb);
	} else {
		/*
 		 * If no control blocks have been specified, figure
		 * out how many distinct one we have and summarize
		 * them in tcp_pcbp for sorting the trace records
		 * below.
		 */
		for (i = 0; i < tcp_ndebug; i++) {
			td = tcp_debug + i;
			if (td->td_tcb == 0)
				continue;
			for (j = 0; j < npcbs; j++)
				if (tcp_pcbp[j] == td->td_tcb)
					break;
			if (j >= npcbs)
				tcp_pcbp[npcbs++] = td->td_tcb;
		}

		if (!npcbs)
			err_quit("can't find control block");

		qsort(tcp_pcbp, npcbs, sizeof(caddr_t), numeric);
		for (i = 0; i < npcbs; i++) {
			printf("\n%p:\n", tcp_pcbp[i]);
			dotrace(tcp_pcbp[i]);
		}
	}

	return(0);
}

void dotrace(register caddr_t tcpcb)
{
	register struct tcp_debug *td;
	register int i;
	int prev_debx = tcp_debx, family;

again: if (--tcp_debx < 0)
		tcp_debx = tcp_ndebug - 1;

	for (i = prev_debx % tcp_ndebug; i < tcp_ndebug; i++) {
		td = tcp_debug + i;
		if ((tcpcb && td->td_tcb != tcpcb) || td->family != AF_INET)
			continue;

		ntime = ntohl(td->td_time);
		tcp_trace(td->td_act, td->td_ostate, &td->td_cb, td->family,
				&td->td_ti.ti_i, &td->td_ti.ti_t, td->td_req);

		if (i == tcp_debx)
			goto done;
	}

	for (i = 0; i <= tcp_debx % tcp_ndebug; i++) {
		td = tcp_debug + i;
		if ((tcpcb && td->td_tcb != tcpcb) || td->family != AF_INET)
			continue;

		ntime = ntohl(td->td_time);
		tcp_trace(td->td_act, td->td_ostate, &td->td_cb, td->family,
				&td->td_ti.ti_i, &td->td_ti.ti_t, td->td_req);
	}

done:	if (follow) {
		prev_debx = tcp_debx + 1;
		if (prev_debx >= tcp_ndebug)
			prev_debx = 0;
		do {
			sleep(1);
			Kread((KA_T) nl[N_TCP_DEBX].n_value, (char *) &tcp_debx, sizeof(tcp_debx));
		} while (tcp_debx == prev_debx);

		Kread((KA_T) nl[N_TCP_DEBUG].n_value, (char *) &var, sizeof(var));
		Kread((KA_T) var, (char *) tcp_debug, sizeof(struct tcp_debug)*tcp_ndebug);
		goto again;
	}
}

void tcp_trace(short act, short ostate, struct tcpcb *tp, int family,
		void *ip, struct tcphdr *th, int req)
{
	tcp_seq seq, ack;
	int flags, len, win, timer;
	struct ip *ip4;
	const char *cp = "<";
	register int i;

	ip4 = (struct ip *)ip;
	/* ntime - iptime(): ms since midnight, UTC */
	printf("%.3f %s:%s ", ntime/1000.000, tcpstates[ostate], tanames[act]);

	switch(act) {
		case TA_INPUT:
		case TA_OUTPUT:
		case TA_DROP:
			printf("(src=%s,%u, ", inet_ntoa(ip4->ip_src), ntohs(th->th_sport));
			printf("dst=%s,%u) ", inet_ntoa(ip4->ip_dst), ntohs(th->th_dport));
			seq = th->th_seq;
			ack = th->th_ack;
			len = ip4->ip_len;
			win = th->th_win;
			if (act == TA_OUTPUT) {
				seq = ntohl(seq);
				ack = ntohl(ack);
				len = ntohs(len);
				len -= sizeof(struct ip) + th->th_off*4;
				win = ntohs(win);
			}
			if (len)
				printf("seq %lu:%lu(%d), ", (u_long)seq, (u_long)(seq+len), len);
			else
				printf("seq %lu, ", (u_long)seq);
			printf("ack %lu ", (u_long)ack);
			if (win)
				printf("(win=%d) ", win);
			flags = th->th_flags;
			if (flags) {
				pf(TH_SYN, "SYN");
				pf(TH_ACK, "ACK");
				pf(TH_FIN, "FIN");
				pf(TH_FIN, "RST");
				pf(TH_RST, "RST");
				pf(TH_PUSH, "PUSH");
				pf(TH_URG, "URG");
				printf(">");
			}
			break;
		case TA_USER:
			timer = req >> 8;
			req &= 0xff;
			printf("%s", prurequests[req]);
			if (req == PRU_FASTTIMO || req == PRU_SLOWTIMO)
				printf("<%s>", tcptimers[timer]);
			break;
		default:
			printf("Unknown action: %d\n", act);
	}
	printf(" -> %s\n", tcpstates[tp->t_state]);
	printf("\tsnd_una %lu, snd_nxt %lu, snd_max %lu\n",
			(u_long)tp->snd_una, (u_long)tp->snd_nxt, (u_long)tp->snd_max);
	printf("\tsnd_wnd %lu, snd_wnd_scale %d, snd_cwnd %lu, snd_ssthresh %lu\n",
			(u_long)tp->snd_wnd, tp->snd_wnd_scale,
			(u_long)tp->snd_cwnd, (u_long)tp->snd_ssthresh);
	printf("\trcv_nxt %lu, rcv_wnd %lu, rcv_wnd_scale %d, rcv_adv %lu\n",
			(u_long)tp->rcv_nxt, (u_long)tp->rcv_wnd,
			tp->rcv_wnd_scale, (u_long)tp->rcv_adv);
	printf("\tt_rtt %d, t_rtseq %lu, t_srtt %d, t_rttvar %d, t_rxtcur %d\n",
			tp->t_rtt, (u_long)tp->t_rtseq, tp->t_srtt, tp->t_rttvar, tp->t_rxtcur);

	cp = "\t";
	for (i = 0; i < TCPT_NTIMERS; i++) {
		if (tp->t_timer[i] == 0)
			continue;
		printf("%s%s=%d", cp, tcptimers[i], tp->t_timer[i]);
		if (i == TCPT_REXMT)
			printf(" (t_rxtshift=%d)", tp->t_rxtshift);
		cp = ", ";
	}
	if (*cp != '\t')
		putchar('\n');

}

int numeric(const void *v1, const void *v2)
{
	const caddr_t *c1 = v1, *c2 = v2;
	return(*c1 - *c2);
}
