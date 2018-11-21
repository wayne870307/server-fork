#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/wait.h>

#define SVR_PORT                        80
#define BUF_SIZE                        1024

char design[] = 
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html><head><title>server_select</title>\r\n"
"<img src=\"TestPicture.jpg\"></center>\r\n";

int main (int argc, char **argv) 
{
	struct sockaddr_in  server_addr;
	socklen_t           len;
	fd_set              active_fd_set;
	int                 sock_fd;
	int                 max_fd;
	int		    fdimg;
	int                 flag = 1;
	char                buff[BUF_SIZE];

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(SVR_PORT);
	len = sizeof(struct sockaddr_in);

	/* Create endpoint */
	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{
		perror("socket()");
		return -1;
	} 

	/* Set socket option */
	if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0) 
	{
		perror("setsockopt()");
		return -1;
	}

	/* Bind */
	if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) 
	{
		perror("bind()");
		return -1;
	} 

	/* Listen */
	if (listen(sock_fd, 128) == -1) 
	{
		perror("listen()");
		return -1;
	}

	FD_ZERO(&active_fd_set);
	FD_SET(sock_fd, &active_fd_set);
	max_fd = sock_fd;

	while (1) 
	{
		int             ret;
		struct timeval  tv;
		fd_set          read_fds;

		/* Set timeout */
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		/* Copy fd set */
		read_fds = active_fd_set;
		ret = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
		if (ret == -1) 
		{
			perror("select()");
			return -1;
		} 
		else if (ret == 0) 
		{
			continue;
		} 
		else 
		{
			int i;

			/* Service all sockets */
			for (i = 0; i < FD_SETSIZE; i++) 
			{
				if (FD_ISSET(i, &read_fds)) 
				{
					if (i == sock_fd) 
					{
						/* Connection request on original socket. */
						struct sockaddr_in  client_addr;
						int                 new_fd;

						/* Accept */
						new_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &len);
						if (new_fd == -1) 
						{
							perror("accept()");
							return -1;
						} 
						else 
						{
							memset(buff,0,1024);
							read(new_fd, buff, 1023);
							if(!strncmp(buff, "GET /TestPicture.jpg", 19))
							{
								fdimg = open("TestPicture.jpg", O_RDONLY);
								sendfile(new_fd, fdimg, NULL, 160000);
								close(fdimg);
							}
							else
							{
								write(new_fd,design,sizeof(design)-1);
							}
							/* Add to fd set */
							FD_SET(new_fd, &active_fd_set);
							if (new_fd > max_fd)
							{
								max_fd = new_fd;
							}
						}
					} 
					else 
					{
						/* Data arriving on an already-connected socket */
						int recv_len;

						/* Receive */
						memset(buff, 0, sizeof(buff));
						recv_len = recv(i, buff, sizeof(buff), 0);
						if (recv_len == -1)
						{
							perror("recv()");
							return -1;
						} 
						else if(recv_len == 0)
						{
						}
						else 
						{
							/* Send (In fact we should determine when it can be written)*/
							send(i, buff, recv_len, 0);
						}

						/* Clean up */
						close(i);
						FD_CLR(i, &active_fd_set);
					}
				} 
			} 
		} 
	} 
	return 0;
}
