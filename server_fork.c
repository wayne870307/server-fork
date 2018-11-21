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

char design[] = 
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html><head><title>server_fork</title>\r\n"
"<img src=\"TestPicture.jpg\"></center>\r\n";

int main(int argc, char *argv[])
{
	struct sockaddr_in server_addr, client_addr;
	socklen_t sin_len = sizeof(client_addr);
	int fd_server , fd_client;
	char buf[2048];
	int fdimg, pid;
	int on = 1;

	/*取得socket descriptor*/
	fd_server = socket(AF_INET, SOCK_STREAM, 0);
	if(fd_server < 0)
	{
		perror("socket");
		exit(1);
	}

	/*第三個參數表示除非已經有 listening socket 綁定到這個 port，不然可以允許其它的 sockets bind() 這個 port。
	這樣重新啟動 server 時便不會遇到 "Address already in use" 這個錯誤訊息。*/
	setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(80);

	/*將socket綁定到執行程式的主機上, port為80*/
	if(bind(fd_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
	{
		perror("bind");
		close(fd_server);
		exit(1);
	}

	/*進入的連線將會在這個佇列中排隊等待，直到accept()它們，排隊上限為10名*/
	if(listen(fd_server, 10) == -1) 
	{
		perror("listen");
		close(fd_server);
		exit(1);
	}

	while(1)
	{
		/*接受一個進入的連線，並準備和fd_client這個新的socket descriptor溝通*/
		fd_client = accept(fd_server, (struct sockaddr *) &client_addr, &sin_len);

		if(fd_client == -1)
		{
			perror("Connection failed....\n");
			continue;
		}

		/*產生子行程*/
		pid = fork();
		/*child process, pid == 0*/
		if(!pid)
		{
			close(fd_server);
			memset(buf, 0, 2048);
			read(fd_client, buf, 2047);

			//printf("%s\n", buf);

			if(!strncmp(buf, "GET /TestPicture.jpg", 16))
			{
				fdimg = open("TestPicture.jpg", O_RDONLY);
				sendfile(fd_client, fdimg, NULL, 160000);
				close(fdimg);
			}
			else
			{
				write(fd_client, design, sizeof(design) -1);
			}
			close (fd_client);
			exit(0);
		}
		/*parent process, pid > 0*/
		if(pid) 
		{
			wait(NULL);
			close(fd_client);
		}
	}
	return 0;
}
