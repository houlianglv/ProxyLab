/*
 * proxy.c - CS:APP Web proxy
 *
 * student ID: 10300720146
 * Name: houliang lv
 * The proxy is tested with cURL
 * if you want to test in localhost, you could try:
 * unix>curl www.example.com -x http://localhost:8080
 *
 */

#include "csapp.h"
#include "sbuf.h"
#include "cache.h"
#define NTHREADS 4
#define SBUFSIZE 16
#define PROXYPORT 8080

/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
void* thread(void* arg);
void proxy_handler(struct thread_arg* arg);

/*
 * Global Variables
 */
static struct Cache cache; /* cache linked list */
FILE *log_file;
sbuf_t sbuf; /* Shared buffer for connected descriptors */
static char *connectionheader = "Connection: close\r\n";
static char *proxyconnectionheader = "Proxy-Connection: close\r\n";

/*
 * main - Main thread routine for the proxy server
 */
int main(int argc, char **argv)
{
    int i, listenfd, connfd, clientlen, port = 0;
    struct hostent *hp;
    struct sockaddr_in sock;
    struct thread_arg thread_arg;
    char *haddrp;
    pthread_t tid;

    Signal(SIGPIPE, SIG_IGN);
    cache_init(&cache);
    log_file = fopen("./proxy.log", "a");
    if(!log_file){
        perror("open file error");
    }

    port = PROXYPORT;
    sbuf_init(&sbuf, SBUFSIZE);
    //listening the port
    listenfd = Open_listenfd(port);
    if (listenfd < 0) {
        fprintf(stderr, "failed to listen to port: %d\n", port);
        exit(1);
    }

    for (i = 0; i < NTHREADS; i++) {
        Pthread_create(&tid, NULL, thread, NULL);
    }

    while (1) {
        clientlen = sizeof(sock);
        connfd = Accept(listenfd, (SA *)&sock, (socklen_t *)&clientlen);
        hp = Gethostbyaddr((const char*)&sock.sin_addr.s_addr, sizeof(sock.sin_addr.s_addr), AF_INET);
        haddrp = inet_ntoa(sock.sin_addr);
        printf("proxy server connected to %s (%s)\n", hp->h_name, haddrp);
        thread_arg.connfd = connfd;
        thread_arg.clientaddr = sock;
        sbuf_insert(&sbuf, &thread_arg);
    }
}

void client_error(int fd, char *cause, char *errnum,
                  char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "%s: %s\r\n", errnum, shortmsg);
    sprintf(body, "%s%s: %s", body, longmsg, cause);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void* thread(void* arg){

    Pthread_detach(Pthread_self());
    while (1) {
        struct thread_arg *thread_arg = Malloc(sizeof(struct thread_arg));
        struct thread_arg *item = sbuf_remove(&sbuf);
        *thread_arg = *item;
        proxy_handler(thread_arg);
        Free(thread_arg);
    }

}

void proxy_handler(struct thread_arg *arg){

    int clientfd = arg->connfd;
    struct sockaddr_in sock = arg->clientaddr;
    char buf[MAXLINE];

    rio_t rio;
    Rio_readinitb(&rio, clientfd);
    Rio_readlineb(&rio, buf, MAXLINE);

    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];

    if (sscanf(buf, "%s %s %s", method, uri, version) < 3)
    {
        fprintf(stderr, "bad request, sscanf error");
        fprintf(stderr, "req line = %s\n", buf);
        client_error(clientfd, method, "400", "Bad Request", "sscanf error");
        return;
    }

    if (strcmp(method, "GET"))
    {
        fprintf(stderr, "Not implement: %s\n", method);
        fprintf(stderr, "req line = %s\n", buf);
        client_error(clientfd, method, "501", "Not Implemented", "Not Implemented");
        return;
    }

    char hostname[MAXLINE], pathname[MAXLINE];
    int port;

    if (parse_uri(uri, hostname, pathname, &port))
    {
        fprintf(stderr, "parse_uri error\n");
        client_error(clientfd, method, "400", "Bad Request", "parse_uri error");
        return;
    }

    //look for the cache
    int length;
    char data[MAX_OBJECT_SIZE];
    if(cache_get(&cache, uri, &length, (void *)data) == 0){
        printf("Cache hit!\n");
        Rio_writen(clientfd, data, length);
        Close(clientfd);
        return;
    }


    //open connection to the real server
    rio_t server_rio;
    int to_server_fd = Open_clientfd(hostname, port);
    Rio_readinitb(&server_rio, to_server_fd);
    char line[MAXLINE];
    sprintf(line, "%s %s %s\r\n", method, pathname, version);
    Rio_writen(to_server_fd, line, strlen(line));

    //send request headers to the real server
    ssize_t n = 0;
    while ((n = Rio_readlineb(&rio, line, MAXLINE)) > 2){
        if (strstr(line, "Proxy-Connection")){
            Rio_writen(to_server_fd, proxyconnectionheader, strlen(proxyconnectionheader));
        }else if(strstr(line, "Connection")){
            Rio_writen(to_server_fd, connectionheader, strlen(connectionheader));
        }
        printf("header from client: %s", line);
        Rio_writen(to_server_fd, line, strlen(line));

    }
    Rio_writen(to_server_fd, "\r\n", 2);

    // read content from server
    int l = 0, put_to_cache = 1;
    while ((n = Rio_readlineb(&server_rio, buf, MAXLINE)) != 0){
        if(l + n < MAX_OBJECT_SIZE){
            memcpy(data + l, buf, n);
            l += n;
        }else{
            put_to_cache = 0;
        }
        Rio_writen(clientfd, buf, strlen(buf));
    }
    if(put_to_cache){
        printf("Caching uri %s length %d\n", uri, l);
        cache_put(&cache, uri, l, data);
    }

    Close(to_server_fd);
    Close(clientfd);
    printf("proxy_handler success!\n");
    return;
}


/*
 * parse_uri - URI parser
 *
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    long len;

    if (strncasecmp(uri, "http://", 7) != 0) {
        hostname[0] = '\0';
        return -1;
    }

    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';

    /* Extract the port number */
    *port = 80; /* default */
    if (*hostend == ':')
        *port = atoi(hostend + 1);

    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
        pathname[0] = '\0';
    }
    else {
        strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring.
 *
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr,
                      char *uri, int size)
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
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s", time_str, a, b, c, d, uri);
}


