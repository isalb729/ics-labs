/*
 * proxy.c - ICS Web proxy
 * Name: Yu Yajie
 * Sn: 517021910851
 */

#include "csapp.h"
#include <stdarg.h>
#include <sys/select.h>

/* 
 * Function prototypes
 */

int parse_uri(char *uri, char *target_addr, char *path, char *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, size_t size);
/*Deal with each thread*/
void doit(int connfd, struct sockaddr_in *sockaddr);

/*Wrapper function avoid being terminated by errors*/
ssize_t Rio_readlineb_w(rio_t *fp, void *userbuf, size_t maxlen);
ssize_t Rio_readnb_w(rio_t *fp, void *userbuf, size_t n);
ssize_t Rio_writen_w(int fd, void *userbuf, size_t n);

/*For concurrent users*/
void *thread(void *targ);
/*Global control of printf */
sem_t mutex;
/*Almost no use but transmit the arguments*/
typedef struct targs {
    // pthread_t tid; No use!!
    int connfd;
    struct sockaddr_in clientaddr;
} threadargs;
/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{  
    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }
    /* Ignore the pipe signal and initialize mutex*/
    Signal(SIGPIPE, SIG_IGN);
    Sem_init(&mutex, 0, 1);
    // int tcnt = 0;
    int listenfd, connfd;
    socklen_t clientlen = sizeof(struct sockaddr_storage);
    pthread_t tid;
    threadargs *targ;
    /*Work as a server*/
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        /*Avoid a race*/
        targ = Malloc(sizeof(threadargs));
        connfd = Accept(listenfd, (SA *)(&(targ->clientaddr)), &clientlen);
        /*Tid++*/
        // targ->tid = tcnt++;No use!!
        targ->connfd = connfd;
        Pthread_create(&tid, NULL, thread, targ);
    }
    Close(listenfd);
    exit(0);
}
void* thread(void* targ)
{
    /*Using detach instead of join*/
    Pthread_detach(Pthread_self());
    threadargs *arg = (threadargs*)targ;
    doit(arg->connfd, (struct sockaddr_in *)(&(arg->clientaddr)));
    /*Free the memory when it is of no use*/
    Free(targ);
    return NULL;
}
void doit(int connfd, struct sockaddr_in *sockaddr){
    
    rio_t rio, targetrio;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char header[MAXLINE], hostname[MAXLINE], pathname[MAXLINE], port[MAXLINE], logstring[MAXLINE];
    int targetfd, content_length = 0, size = 0;
    size_t n;
    Rio_readinitb(&rio, connfd);

    /*Get the main request line and check its validation*/
    if (Rio_readlineb_w(&rio, buf, MAXLINE) <= 0 || sscanf(buf, "%s %s %s", method, uri, version) != 3 || parse_uri(uri, hostname, pathname, port)){
        Close(connfd);
        printf("Get request line failed!\n");
        fflush(stdout);
        return;
    }

    /*Other methods are not discussed here*/
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) {
        Close(connfd);
        printf("Not valid method!\n");
        fflush(stdout);
        return;
    }

    /*Build the request header Get names*/
    sprintf(header, "%s /%s %s\r\n", method, pathname, version);
    
    /*Use lower-case function to avoid being terminated*/
    if ((targetfd = open_clientfd(hostname, port)) < 0){
        Close(connfd);
        printf("Connection failed!\n");
        fflush(stdout);
        return;
    }

    /*Begin to transmit the request header to targetfd*/
    Rio_writen_w(targetfd, header, strlen(header));
    while((n = Rio_readlineb_w(&rio, buf, MAXLINE)) > 0) {
        Rio_writen_w(targetfd, buf, n);
    /*Get the content length to ease the count of size*/
        if(!strncmp("Content-Length", buf, strlen("Content-Length"))) 
            content_length = atoi(buf + 15);
        if(!strcmp("\r\n", buf)) break;
    }

    // if (content_length != 0 && Rio_readnb_w(&rio, buf, content_length) != 0) Rio_writen_w(targetfd, buf, content_length);
    
    /*POST method has the http request body*/
    if (strcmp("GET", method) && content_length && (n = Rio_readnb_w(&rio, buf, content_length)) > 0)
        /*Get body three ways*/
            // while (content_length--)
            //     if ((n = Rio_readnb_w(&rio, buf, 1)) > 0)
            //         Rio_writen_w(targetfd, buf, 1);
                Rio_writen_w(targetfd, buf, n);
            // for (int i = 0;i < content_length; i++)
            // {
            //     if (Rio_readnb_w(&rio, buf, 1) > 0)
            //     Rio_writen_w(targetfd, buf, 1);
            // }

    /*To reuse it*/
    content_length = 0;

    /*Get the response*/ 
    /*Similiar to the last phase*/
    Rio_readinitb(&targetrio, targetfd);
    while((n = Rio_readlineb_w(&targetrio, buf, MAXLINE)) != 0) {
        Rio_writen_w(connfd, buf, n);
        size += n;
        if(!strcmp("\r\n", buf)) break;
        if(!strncmp("Content-Length", buf, strlen("Content-Length")))
            content_length = atoi(buf + 15);
    }

    /*Get content and size*/
    // if (content_length != 0 && Rio_readnb_w(&targetrio, buf, content_length) != 0) Rio_writen_w(connfd, buf, content_length);
        while (content_length--)
            if ((n = Rio_readnb_w(&targetrio, buf, 1)) > 0 && (++size))
                Rio_writen_w(connfd, buf, 1);

        /*This way failed occasionally I don't know why*/
        // if ((n = Rio_readnb_w(&targetrio, buf, content_length)) > 0)
        //     Rio_writen_w(connfd, buf, n);
        // size += n;    

    //Close and log
    //Notice the lock
    Close(targetfd);

    P(&mutex);
    format_log_entry(logstring, sockaddr, uri, size);
    printf("%s\n", logstring); 
    fflush(stdout);
    V(&mutex);

    Close(connfd);
}

/*
 * parse_uri - URI parser
 *
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, char *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
        hostname[0] = '\0';
        return -1;
    }

    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    if (hostend == NULL)
        return -1;
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';

    /* Extract the port number */
    if (*hostend == ':') {
        char *p = hostend + 1;
        while (isdigit(*p))
            *port++ = *p++;
        *port = '\0';
    } else {
        strcpy(port, "80");
    }

    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
        pathname[0] = '\0';
    }
    else {
        pathbegin++;
        strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring.
 *
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), the number of bytes
 * from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr,
                      char *uri, size_t size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /*
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 12, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;
    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s %zu", time_str, a, b, c, d, uri, size);
}

/*Three wrapper functions*/
ssize_t Rio_writen_w(int fd, void *userbuf, size_t n)
{
	ssize_t rc;
	if ((rc = rio_writen(fd, userbuf, n)) != n)
	{
		fprintf(stderr, "%s: %s\n", "Rio writen error", strerror(errno));
		return 0;
	}
	return rc;
}
ssize_t Rio_readnb_w(rio_t *fp, void *userbuf, size_t n)
{
    ssize_t rc;
    if ((rc = rio_readnb(fp, userbuf, n)) < 0)
    {
        fprintf(stderr, "%s: %s\n", "Rio readnb error", strerror(errno));
        return 0;
    }
    return rc;
}
ssize_t Rio_readlineb_w(rio_t *fp, void *userbuf, size_t maxlen)
{
    ssize_t rc;
    if ((rc = rio_readlineb(fp, userbuf, maxlen)) < 0)
    {
        fprintf(stderr, "%s: %s\n", "Rio readlineb error", strerror(errno));
        return 0;
    }
    return rc;
} 