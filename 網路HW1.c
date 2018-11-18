#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 8096

struct {
    char *ext;
    char *filetype;
} extensions [] = {
    {"gif", "image/gif" },
    {"jpg", "image/jpeg"},
    {"jpeg","image/jpeg"},
    {"png", "image/png" },
    {"zip", "image/zip" },
    {"gz",  "image/gz"  },
    {"tar", "image/tar" },
    {"htm", "text/html" },
    {"html","text/html" },
    {"exe","text/plain" },
    {0,0} };

void handle_socket(int fd)
{
    int j, file_fd, buflen, len;
    long i, ret;
    char * fstr;
    static char buffer[BUFSIZE+1];

    ret = read(fd,buffer,BUFSIZE);   /* Ū���s�����n�D */
    if (ret==0||ret==-1) {
     /* �����s�u�����D�A�ҥH������{ */
        exit(3);
    }

    /* �{���ޥ��G�bŪ���쪺�r�굲���ɪŦr���A��K����{���P�_���� */
    if (ret>0&&ret<BUFSIZE)
        buffer[ret] = 0;
    else
        buffer[0] = 0;

    /* ��������r�� */
    for (i=0;i<ret;i++) 
        if (buffer[i]=='\r'||buffer[i]=='\n')
            buffer[i] = 0;
    
    /* �u���� GET �R�O�n�D */
    if (strncmp(buffer,"GET ",4)&&strncmp(buffer,"get ",4))
        exit(3);
    
    /* �ڭ̭n�� GET /index.html HTTP/1.0 �᭱�� HTTP/1.0 �ΪŦr���j�} */
    for(i=4;i<BUFSIZE;i++) {
        if(buffer[i] == ' ') {
            buffer[i] = 0;
            break;
        }
    }

    /* �ɱ��^�W�h�ؿ������|�y..�z */
    for (j=0;j<i-1;j++)
        if (buffer[j]=='.'&&buffer[j+1]=='.')
            exit(3);

    /* ��Ȥ�ݭn�D�ڥؿ���Ū�� index.html */
    if (!strncmp(&buffer[0],"GET /\0",6)||!strncmp(&buffer[0],"get /\0",6) )
        strcpy(buffer,"GET /index.html\0");

    /* �ˬd�Ȥ�ݩҭn�D���ɮ׮榡 */
    buflen = strlen(buffer);
    fstr = (char *)0;

    for(i=0;extensions[i].ext!=0;i++) {
        len = strlen(extensions[i].ext);
        if(!strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
            fstr = extensions[i].filetype;
            break;
        }
    }

    /* �ɮ׮榡���䴩 */
    if(fstr == 0) {
        fstr = extensions[i-1].filetype;
    }

    /* �}���ɮ� */
    if((file_fd=open(&buffer[5],O_RDONLY))==-1)
  write(fd, "Failed to open file", 19);

    /* �Ǧ^�s�������\�X 200 �M���e���榡 */
    sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
    write(fd,buffer,strlen(buffer));


    /* Ū���ɮפ��e��X��Ȥ���s���� */
    while ((ret=read(file_fd, buffer, BUFSIZE))>0) {
        write(fd,buffer,ret);
    }

    exit(1);
}

main(int argc, char **argv)
{
    int i, pid, listenfd, socketfd;
    size_t length;
    static struct sockaddr_in cli_addr;
    static struct sockaddr_in serv_addr;

    /* �ϥ� /tmp ������ڥؿ� */
    if(chdir("/tmp") == -1){ 
        printf("ERROR: Can't Change to directory %s\n",argv[2]);
        exit(4);
    }

    /* �I���~����� */
    if(fork() != 0)
        return 0;

    /* ������{�������ݤl��{���� */
    signal(SIGCLD, SIG_IGN);

    /* �}�Һ��� Socket */
    if ((listenfd=socket(AF_INET, SOCK_STREAM,0))<0)
        exit(3);

    /* �����s�u�]�w */
    serv_addr.sin_family = AF_INET;
    /* �ϥΥ���b��������~ IP */
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    /* �ϥ� 80 Port */
    serv_addr.sin_port = htons(80);

    /* �}�Һ�����ť�� */
    if (bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr))<0)
        exit(3);

    /* �}�l��ť���� */
    if (listen(listenfd,64)<0)
        exit(3);

    while(1) {
        length = sizeof(cli_addr);
        /* ���ݫȤ�ݳs�u */
        if ((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length))<0)
            exit(3);

        /* ���X�l��{�B�z�n�D */
        if ((pid = fork()) < 0) {
            exit(3);
        } else {
            if (pid == 0) {  /* �l��{ */
                close(listenfd);
                handle_socket(socketfd);
            } else { /* ����{ */
                close(socketfd);
            }
        }
    }
}
