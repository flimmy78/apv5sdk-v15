/***********************************************************
* ����:���Ļ�                                                                       *
* ʱ��:2010��7��28��                                                           *
* ���ƣ�myping.c                                                                      *
* ˵��:������������ʾping�����ʵ��ԭ��    *
***********************************************************/
#include <auteos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>

#define PACKET_SIZE     4096
#define MAX_WAIT_TIME   5
#define MAX_NO_PACKETS  4
char sendpacket[PACKET_SIZE];
char recvpacket[PACKET_SIZE];
int sockfd,datalen=56;
unsigned long pingcount = MAX_NO_PACKETS;
int nsend=0,nreceived=0;
struct sockaddr_in dest_addr;
pid_t pid;
struct sockaddr_in from;
struct timeval tvrecv;
void statistics(void);
unsigned short cal_chksum(unsigned short *addr,int len);
int pack(int pack_no);
void send_packet(void);
void recv_packet(void);
int unpack(char *buf,int len);
void tv_sub(struct timeval *out,struct timeval *in);
void statistics(void)
{
	autelan_printf("\n--------------------PING statistics-------------------\n");
	autelan_printf("%d packets transmitted, %d received , %d%% lost\n",pingcount,nreceived, (pingcount-nreceived)*100/pingcount);
	autelan_close(sockfd);
	exit(1);
}
/*У����㷨*/
unsigned short cal_chksum(unsigned short *addr,int len)
{
	int nleft=len;
	int sum=0;
	unsigned short *w=addr;
	unsigned short answer=0;
	/*��ICMP��ͷ������������2�ֽ�Ϊ��λ�ۼ�����*/
	while(nleft>1)
	{
		sum+=*w++;
		nleft-=2;
	}
	/*��ICMP��ͷΪ�������ֽڣ���ʣ�����һ�ֽڡ������һ���ֽ���Ϊһ��2�ֽ����ݵĸ��ֽڣ����2�ֽ����ݵĵ��ֽ�Ϊ0�������ۼ�*/
	if( nleft==1)
	{
		*(unsigned char *)(&answer)=*(unsigned char *)w;
		sum+=answer;
	}
	sum=(sum>>16)+(sum&0xffff);
	sum+=(sum>>16);
	answer=~sum;
	return answer;
}
/*����ICMP��ͷ*/
int pack(int pack_no)
{
	int i,packsize;
	struct icmp *icmp;
	struct timeval *tval;
	icmp=(struct icmp*)sendpacket;
	icmp->icmp_type=ICMP_ECHO;
	icmp->icmp_code=0;
	icmp->icmp_cksum=0;
	icmp->icmp_seq=pack_no;
	icmp->icmp_id=pid;
	packsize=8+datalen;
	tval= (struct timeval *)icmp->icmp_data;
	autelan_gettimeofday(tval,NULL);    /*��¼����ʱ��*/
	icmp->icmp_cksum=cal_chksum( (unsigned short *)icmp,packsize);
	/*У���㷨*/
	return packsize;
}
//Wait for echoRecv
int WaitForEchoReply(int socket, int timeout)
{
	struct timeval Timeout;
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(socket, &readfds);
	Timeout.tv_sec = timeout;
	Timeout.tv_usec = 0;
	return(autelan_select(socket+1, &readfds, NULL, NULL, &Timeout));
}
/*��������ICMP����*/
void send_packet()
{
	int packetsize;
	int timeout;
	while( nsend<MAX_NO_PACKETS)
	{
		nsend++;
		packetsize=pack(nsend);
		/*����ICMP��ͷ*/
		if(autelan_sendto(sockfd,sendpacket,packetsize,0, (struct sockaddr *)&dest_addr,sizeof(dest_addr) )<0)
		{
			autelan_printf("autelan_sendto error\n");
			perror("autelan_sendto error");
			continue;
		}
		timeout =1;
		if(WaitForEchoReply(sockfd, timeout) == -1)
		{
			autelan_printf("==============select error=========\n");
			perror("select");
			continue;
		}
		//              sleep(1); /*ÿ��һ�뷢��һ��ICMP����*/
	}

}
/*��������ICMP����*/
void recv_packet()
{
	int n,fromlen,timeout,ntimeout;
	extern int errno;
	//	signal(SIGALRM,statistics);
	fromlen=sizeof(from);
	ntimeout = 0;
	while((nreceived+ntimeout)<nsend)
	{
		//		alarm(MAX_WAIT_TIME);
		timeout = 3;
		int ret = WaitForEchoReply(sockfd, timeout);
		if(ret <0)
			autelan_printf("select error\n");
		else if (ret == 0)
		{
//			printf("receive time out\n");
			ntimeout++;
		}
		else
		{
			if( (n=autelan_recvfrom(sockfd,recvpacket,sizeof(recvpacket),0, (struct sockaddr *)&from,&fromlen)) <0)
			{
				if(errno==EINTR)continue;
				perror("autelan_recvfrom error");
				continue;
			}
			autelan_gettimeofday(&tvrecv,NULL);
			/*��¼����ʱ��*/
			if(unpack(recvpacket,n)==-1)
				continue;
			nreceived++;
		}
	}
}
/*��ȥICMP��ͷ*/
int unpack(char *buf,int len)
{
	int i,iphdrlen;
	struct ip *ip;
	struct icmp *icmp;
	struct timeval *tvsend;
	double rtt;
	ip=(struct ip *)buf;
	iphdrlen=ip->ip_hl<<2;
	/*��ip��ͷ����,��ip��ͷ�ĳ��ȱ�־��4*/
	icmp=(struct icmp *)(buf+iphdrlen);
	/*Խ��ip��ͷ,ָ��ICMP��ͷ*/
	len-=iphdrlen;
	/*ICMP��ͷ��ICMP���ݱ����ܳ���*/
	if( len<8)
	/*С��ICMP��ͷ�����򲻺���*/
	{
		autelan_printf("ICMP packets\'s length is less than 8\n");
		return -1;
	}
	/*ȷ�������յ����������ĵ�ICMP�Ļ�Ӧ*/
	if( (icmp->icmp_type==ICMP_ECHOREPLY) && (icmp->icmp_id==pid) )
	{
		tvsend=(struct timeval *)icmp->icmp_data;
		tv_sub(&tvrecv,tvsend);  /*���պͷ��͵�ʱ���*/
		rtt=tvrecv.tv_sec*1000+tvrecv.tv_usec/1000;  /*�Ժ���Ϊ��λ����rtt*/
		/*��ʾ�����Ϣ*/
		autelan_printf("%d byte from %s: icmp_seq=%u ttl=%d rtt=%.3f ms\n", len, autelan_inet_ntoa(from.sin_addr), icmp->icmp_seq, ip->ip_ttl,rtt);
	}
	else
	return -1;
}
void send_recv_packet()
{
	int packetsize;
	int timeout;
	int n,fromlen;
	extern int errno;
	fromlen=sizeof(from);
	
	while(nsend<pingcount)
	{
		nsend++;
		packetsize=pack(nsend);
		/*����ICMP��ͷ*/
		if(autelan_sendto(sockfd,sendpacket,packetsize,0, (struct sockaddr *)&dest_addr,sizeof(dest_addr) )<0)
		{
			autelan_printf("autelan_sendto error\n");
			perror("autelan_sendto error");
			continue;
		}
		timeout =1;
		if(WaitForEchoReply(sockfd, timeout) == -1)
		{
//			printf("==============select error=========\n");
			perror("select");
			continue;
		}

		timeout = 3;
		int ret = WaitForEchoReply(sockfd, timeout);
		if(ret <0)
			autelan_printf("select error\n");
		else if (ret == 0)
		{
//			printf("receive time out\n");
		}
		else
		{
			if( (n=autelan_recvfrom(sockfd,recvpacket,sizeof(recvpacket),0, (struct sockaddr *)&from,&fromlen)) <0)
			{
				if(errno==EINTR)continue;
				perror("autelan_recvfrom error");
				continue;
			}
			autelan_gettimeofday(&tvrecv,NULL);
			/*��¼����ʱ��*/
			if(unpack(recvpacket,n)==-1)
			continue;
			nreceived++;
		}
		
		autelan_sleep(1); /*ÿ��һ�뷢��һ��ICMP����*/
	}

}
main(int argc,char *argv[])
{
	struct hostent *host;
	struct protoent *protocol;
	unsigned long inaddr=0l;
	int waittime=MAX_WAIT_TIME;
	int size=50*1024;
	
	if(argc<2)
	{
		autelan_printf("usage:%s hostname/IP address [-c count] [-s size]\n",argv[0]);
		exit(1);
	}
	
	if( (protocol=autelan_getprotobyname("icmp") )==NULL)
	{
		perror("autelan_getprotobyname");
		exit(1);
	}
	/*����ʹ��ICMP��ԭʼ�׽���,�����׽���ֻ��root��������*/
	if( (sockfd=socket(AF_INET,SOCK_RAW,protocol->p_proto) )<0)
	{
		perror("socket error");
		exit(1);
	}
	/* ����rootȨ��,���õ�ǰ�û�Ȩ��*/
	setuid(getuid());
	/*�����׽��ֽ��ջ�������50K��������ҪΪ�˼�С���ջ���������Ŀ�����,��������pingһ���㲥��ַ��ಥ��ַ,������������Ӧ��*/
	autelan_setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size) );
	bzero(&dest_addr,sizeof(dest_addr));
	dest_addr.sin_family=AF_INET;
	host=autelan_gethostbyname(argv[1]);
	/*�ж�������������ip��ַ*/
	if( inaddr=autelan_inet_addr(argv[1])==INADDR_NONE)
	{
		if(host==NULL) /*��������*/
		{
			perror("autelan_gethostbyname error");
			exit(1);
		}
		memcpy( (char *)&dest_addr.sin_addr,host->h_addr,host->h_length);
	}
	else    /*��ip��ַ*/
	{
		dest_addr.sin_addr = *((struct in_addr *)host->h_addr);
	//             memcpy( (char *)&dest_addr,(char *)&inaddr,host->h_length);
	}
	if(argc == 4)
	{
//		printf("argv[2]:%c  argv[3]:%d\n", &argv[2], atoi(argv[3]));
		if(0 == memcmp(argv[2], "-c", 2))
		{
			pingcount = atoi(argv[3]);
		}
		else if(0 == memcmp(argv[2], "-s", 2))
		{
			datalen = atoi(argv[3]);
		}
	}
	else if(argc == 6)
	{
//		printf("argv[2]:%c  argv[3]:%d argv[4]:%c  argv[5]:%d\n", argv[2], atoi(argv[3]), argv[4], atoi(argv[5]));
		if(0 == memcmp(argv[2], "-c", 2))
		{
			pingcount = atoi(argv[3]);
			if(0 == memcmp(argv[4], "-s", 2))
				datalen = atoi(argv[5]);
		}
		else if(0 == memcmp(argv[2], "-s", 2))
		{
			datalen = atoi(argv[3]);
			if(0 == memcmp(argv[4], "-c", 2))
				pingcount = atoi(argv[5]);
		}
	}
	/*��ȡmain�Ľ���id,��������ICMP�ı�־��*/
	pid=autelan_getpid();
	autelan_printf("PING %s(%s): %d bytes data in ICMP packets.\n",argv[1], autelan_inet_ntoa(dest_addr.sin_addr),datalen);
	//	send_packet();  /*��������ICMP����*/
	//	recv_packet();  /*��������ICMP����*/
	send_recv_packet();
	statistics(); /*����ͳ��*/
	return 0;
}
/*����timeval�ṹ���*/
void tv_sub(struct timeval *out,struct timeval *in)
{
	if( (out->tv_usec-=in->tv_usec)<0)
	{
		--out->tv_sec;
		out->tv_usec+=1000000;
	}
	out->tv_sec-=in->tv_sec;
}
/*------------- The End -----------*/

