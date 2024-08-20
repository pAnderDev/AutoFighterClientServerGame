#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

int main(int argc, char *argv[])
{
    int sockfd = 0, n = 0;
    char recvBuff[1024];
    char sendBuff[1024];
	struct sockaddr_in serv_addr; 

    if(argc != 3)
    {
        printf("\n Usage: %s <ip of server> <port #> \n",argv[0]);
        return 1;
    } 

    memset(recvBuff, '0',sizeof(recvBuff));
    memset(recvBuff, '0',sizeof(sendBuff));
    
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
	int port = atoi(argv[2]);
    serv_addr.sin_port = htons(port); 

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    } 
    
	
	while(1)
    {
		n = read(sockfd, recvBuff, sizeof(recvBuff) - 1);
		if(n > 0) {
			recvBuff[n] = '\0';
			printf(recvBuff);
		}
		else {
			printf("\n Error reading from server\n");
			break;
		}
	
		//read user input and send to server
		fgets(sendBuff, sizeof(sendBuff), stdin);
		write(sockfd, sendBuff, strlen(sendBuff));
    } 
	
	close(sockfd);
    return 0;
}
