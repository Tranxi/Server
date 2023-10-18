#include<iostream>
#include<string.h>
#include<WinSock2.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<stdio.h>
#pragma comment(lib,"WS2_32.lib")
#pragma warning(disable:4996)
#define PRINTF(str) printf("[%s-%d]"#str"=%s\n",__func__,__LINE__,str);//ƴ�� ��#����ַ���
//�޷�����ͼƬ ��ʱδ���
void error_die(const char* str)
{
	perror(str);
	exit(1);
}
void not_found(int client)
{
	
	;//to do
}

void headers(int client)//��Ӧ����ͷ��Ϣ
{
	char buff[1024];
	strcpy_s(buff, "HTTP/1.0 200 OK\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy_s(buff, "Server: Httpd/0.1\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy_s(buff, "Content-Type:text/html\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy_s(buff, "\r\n");
	send(client, buff, strlen(buff), 0);

}

void cat(int client, FILE* resource)
{
	char buff[4096];
	int count = 0;
	while (1)
	{
		int ret = fread(buff, sizeof(char), sizeof(buff), resource);
		if (ret <= 0)
		{
			break;
		}
		send(client,buff,ret,0);
		count += ret;
	}
	printf("%d Bytes are send to web\n", count);
}
int get_line(int sock, char* buff, int size)
{
	char c = 0;
	int i = 0;
	while (c != '\n' && i < size - 1) // ��ֹԽ�� �س������������ַ�
	{
		int n = recv(sock, &c, 1, 0);
		if (n > 0)
		{
			if (c == '\r')
			{
				n = recv(sock, &c, 1, MSG_PEEK);//MSG_PEEK����ȡ��ֻ�ǿ���һ���ַ���ʲô����ֹ������
				if (n > 0 && c == '\n')
					recv(sock, &c, 1, 0);
				else
					c = '\n';
			}
			buff[i++] = c;
		}
		else
		{
			c = '\n';
		}
	}
	buff[i] = '\0';
	return 0;
}
void server_file(int client, const char* fileName)
{
	long long int numChars = 1;
	char buff[1024];
	while (numChars > 0 && strcmp(buff, "\n"))
	{
		numChars = get_line(client, buff, sizeof(buff));
		PRINTF(buff);
	}
	FILE* resource = NULL;
	int type = 0;
	if (strcmp(fileName, "htmlDocs/index.html") == 0)
	{
		resource = fopen(fileName, "r");
	}
	else
	{
		resource = fopen(fileName, "rb");
	}
	if (resource == NULL)
		not_found(client);
	else
	{
		//������Դ������� ����Ҫ����ͷ��Ϣ

		headers(client);

		//�����������Դ��Ϣ
		cat(client, resource);

		printf("��Դ�������");
	}

	fclose(resource);
}


DWORD WINAPI accept_request(LPVOID arg)
{
	char buff[1024];

	int client = (SOCKET)arg; 
	//��ȡһ������
	int numChars = get_line(client, buff, sizeof(buff));
	PRINTF(buff);

	char method[255];
	int j = 0, i = 0;

	while (!isspace(buff[j]) && i < sizeof(method) - 1)
	{
		method[i++] = buff[j++]; 
	}
	method[i] = '\0';
	PRINTF(method);

	//�������ķ���
	if (_stricmp(method, "GET") && _stricmp(method, "POST")) 
	{
		//��������ͻ��˷��ش�����ʾҳ��

		return 0;//�߳̽���
	}

	char url[255];
	i = 0;
	while (isspace(buff[j]) && i < sizeof(buff) )
	{
		j++;
	}
	while (!isspace(buff[j]) && i < sizeof(url) - 1 && j < sizeof(buff))
	{
		url[i++] = buff[j++];
	}
	url[i] = '\0';
	PRINTF(url);
	//ƴ�ӳ�����·��
	char path[512] = "";
	sprintf_s(path,sizeof(path), "htmlDocs%s", url);
	if (path[strlen(path) - 1] == '/')
		strcat_s(path,sizeof(path), "index.html");//�����ļ���Ŀ¼
	PRINTF(path);
	struct stat status;
	if (stat(path, &status) == -1)
	{
		//����˵�ʣ�����ݶ�ȡ���
		while (numChars > 0 && strcmp(buff, "\n"))
		{
			numChars = get_line(client, buff, sizeof(buff));
		}

		not_found(client);
	}
	else
	{
		if ((status.st_mode & S_IFMT) == S_IFDIR)//λ���� ����Ŀ¼
			strcat_s(path,sizeof(path), "/index.html");

		server_file(client, path);
	}
	closesocket(client);
	return 0;
}
//����ֵΪ�������˵��׽���
//port��ʾ�˿�,��*portֵΪ0���Զ�������ö˿�
int start_up(unsigned short* port)
{
	//����ͨ�ų�ʼ�� windowsϵͳ��linuxϵͳ����Ҫ
	WSADATA data;
	int ret = WSAStartup(MAKEWORD(1,1),&data);
	if (ret)//ret!=0
	{
		error_die("WSASartup");
	}

	int server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);//�׽�������,���ݰ���������/���ݱ�),����ͨ��Э��
	if (server_socket == -1)
	{
		error_die("socket");
	}

	//�׽��ֶ˿ڿɸ�������
	int opt = 1;

	ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,(const char*)&opt, sizeof(opt));
	if (ret == -1)
	{
		error_die("setsockopt");
	}
	//���÷������˵������ַ
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(*port);//�����ֽ���תΪ�����ֽ��򣨴��/С�ˣ�
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);//���ܷ���

	//���׽���
	ret = bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (ret < 0)
		error_die("bind");

	//��̬����˿�
	int nameLen = sizeof(server_addr);
	if (*port == 0)
	{
		if(getsockname(server_socket, (struct sockaddr*)&server_addr, &nameLen) < 0 )
			error_die("getsockname");

		*port = server_addr.sin_port;
	}

	//������������
	if (listen(server_socket, 5) < 0)
		error_die("listen");

	return server_socket;
}

int main()
{	
	unsigned short port = 9292;
	int server_socket = start_up(&port);
	printf("http�������������ڼ���%d�˿�...", port);

	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	while (1)
	{
		//����ʽ�ȴ��û�ͨ��������������
		int client_sock = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
		if (client_sock == -1)
			error_die("accept");

		//�����߳�
		DWORD threadId = 0;
		CreateThread(0, 0, accept_request, (void*)client_sock, 0, &threadId);
	}

	closesocket(server_socket);
	return 0;
}