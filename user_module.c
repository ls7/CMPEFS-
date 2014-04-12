#include <sys/socket.h>
#include <linux/netlink.h>
#include <string.h>
#include <stdio.h>

#define NETLINK_USER 31

#define MAX_PAYLOAD 1024 /* maximum payload size*/
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh;// = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;
int file_found =0;
#define TRUE 1
#define FALSE 0
struct File{
	char name[20];
	char data[100];

}files[10],*filp;
char *in_msg;
char temp_str[100];
char name[20];
int file_number = 0;
void main()
{
	int k,i =0;
	char p[20];
	int filIndex =0;
	int dataIndex = 0;

sock_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
if(sock_fd<0)
return -1;

memset(&src_addr, 0, sizeof(src_addr));
src_addr.nl_family = AF_NETLINK;
src_addr.nl_pid = getpid(); /* self pid */

bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

memset(&dest_addr, 0, sizeof(dest_addr));
memset(&dest_addr, 0, sizeof(dest_addr));
dest_addr.nl_family = AF_NETLINK;
dest_addr.nl_pid = 0; /* For Linux Kernel */
dest_addr.nl_groups = 0; /* unicast */

nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
nlh->nlmsg_pid = getpid();
//nlh->nlmsg_flags = 0;
nlh->nlmsg_flags |= NLM_F_ACK;

strcpy(NLMSG_DATA(nlh), "Hello");

iov.iov_base = (void *)nlh;
iov.iov_len = nlh->nlmsg_len;
msg.msg_name = (void *)&dest_addr;

msg.msg_namelen = sizeof(dest_addr);
msg.msg_iov = &iov;
msg.msg_iovlen = 1;

printf("Sending message to kernel\n");
sendmsg(sock_fd,&msg,0);
//recvmsg(sock_fd, &msg, 0);
while(1){
printf("Waiting for message from kernel\n");

/* Read message from kernel */
recvmsg(sock_fd, &msg, 0);
//strcpy(in_msg,NLMSG_DATA(nlh));
in_msg = NLMSG_DATA(nlh);
printf("Message recieved tmp buffer %s",in_msg);

if(in_msg[0] == 'W')
{
	i = 2;


	filp = &files[file_number%10];
	//filp = malloc(sizeof(struct file));
	filIndex =0;
	while(in_msg[i] != ' ')
	{
		filp->name[filIndex++] = in_msg[i++];
		
	}
	filp->name[filIndex] = '\0';
	printf("File name: %s \n",filp->name);
	i = i+1;
	dataIndex = 0;
	while(in_msg[i] != '\0')
	{
			filp->data[dataIndex++] = in_msg[i++];

	}
	filp->data[dataIndex] = '\0';
	printf("File Data: %s \n",filp->data);
	file_number+=1;
}
else if(in_msg[0]== 'R')
{
	i =2;
	filIndex = 0;
	while(in_msg[i] != '\0')
	{
			name[filIndex++] = in_msg[i++];

	}
	name[filIndex] ='\0';
	for(k =0;k<10;k++)
	{

		if(strcmp(name,files[k].name) == 0)
		{
			file_found = TRUE;
			break;
		}

	}
	if(file_found == TRUE)
	{
		printf("Sending: %s\n", files[k].data);
                strcpy(temp_str,"R ");
		strcpy(temp_str+2,files[k].data);
		printf("Sending message to kernel\n");
		//strcpy(NLMSG_DATA(nlh), temp_str);
		//sendmsg(sock_fd,&msg,0);

		
		nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
		memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
		nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
		nlh->nlmsg_pid = getpid();
		nlh->nlmsg_flags = 0;

		strcpy(NLMSG_DATA(nlh), temp_str);

		iov.iov_base = (void *)nlh;
		iov.iov_len = nlh->nlmsg_len;
		msg.msg_name = (void *)&dest_addr;
		msg.msg_namelen = sizeof(dest_addr);
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		printf("Sending message to kernel\n");
		sendmsg(sock_fd,&msg,0);



	}

}
else
{
	printf("******Invalid command******\n");
}




//printf("Received message payload: %s\n", NLMSG_DATA(nlh));
}

close(sock_fd);
}
