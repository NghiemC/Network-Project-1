#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <cnaiapi.h>

#if defined(LINUX) || defined(SOLARIS)
#include <sys/time.h>
#endif

#define BUFFSIZE    256
#define SERVER_NAME "CNAI Demo Web Server"

#define ERROR_400   "<head></head><body><html><h1>Error 400</h1><p>Th\
e server couldn't understand your request.</html></body>\n"

#define ERROR_404   "<head></head><body><html><h1>Error 404</h1><p>Do\
cument not found.</html></body>\n"

#define HOME_PAGE   "<head></head><body><html><h1>Welcome to the CNAI\
 Demo Server</h1><p>Why not visit: <ul><li><a href=\"http://netbook.cs.pu\
rdue.edu\">Netbook Home Page</a><li><a href=\"http://www.comerbooks.com\"\
>Comer Books Home Page<a></ul></html></body>\n"

#define TIME_PAGE   "<head></head><body><html><h1>The current date is\
: %s</h1></html></body>\n"

int recvln(connection, char*, int);
void send_head(connection, int, int);

void handle_arbitrary_file_request(connection conn, char* filepath);

int main(int argc, char* argv[]) {
    connection conn;
    int n;
    char buff[BUFFSIZE], cmd[16], path[64], vers[16], * timestr;

    if (argc != 2) {
        (void)fprintf(stderr, "usage: %s <appnum>\n", argv[0]);
        exit(1);
    }

    while (1) {
        conn = await_contact((appnum)atoi(argv[1]));
        if (conn < 0)
            exit(1);

        n = recvln(conn, buff, BUFFSIZE);
        sscanf(buff, "%s %s %s", cmd, path, vers);

        while ((n = recvln(conn, buff, BUFFSIZE)) > 0) {
            if (n == 2 && buff[0] == '\r' && buff[1] == '\n')
                break;
        }

        if (n < 1) {
            (void)send_eof(conn);
            continue;
        }

        if (strcmp(cmd, "GET") || (strcmp(vers, "HTTP/1.0") && strcmp(vers, "HTTP/1.1"))) {
            send_head(conn, 400, strlen(ERROR_400));
            (void)send(conn, ERROR_400, strlen(ERROR_400), 0);
            (void)send_eof(conn);
            continue;
        }

        if (strcmp(path, "/") == 0) {
            send_head(conn, 200, strlen(HOME_PAGE));
            (void)send(conn, HOME_PAGE, strlen(HOME_PAGE), 0);
        }
        else if (strcmp(path, "/time") == 0) {
#if defined(LINUX) || defined(SOLARIS)
            struct timeval tv;
            gettimeofday(&tv, NULL);
            timestr = ctime(&tv.tv_sec);
#elif defined(WIN32)
            time_t tv;
            time(&tv);
            timestr = ctime(&tv);
#endif
            char buff[BUFFSIZE];
            sprintf(buff, TIME_PAGE, timestr);
            send_head(conn, 200, strlen(buff));
            (void)send(conn, buff, strlen(buff), 0);
        }
        else {
            // Handle arbitrary file request
            handle_arbitrary_file_request(conn, path);
        }

        (void)send_eof(conn);
    }
}

void handle_arbitrary_file_request(connection conn, char* filepath) {
    char fullpath[256]; // Adjust the size as needed

    // Construct the file path by concatenating "./" (assuming files are in the same directory as the executable) with the requested path.
    sprintf(fullpath, "./%s", filepath);

    FILE* file = fopen(fullpath, "r");

    if (file) {
        struct stat st;
        stat(fullpath, &st);
        int file_size = st.st_size;

        send_head(conn, 200, file_size);

        char* file_content = malloc(file_size);
        fread(file_content, 1, file_size, file);
        (void)send(conn, file_content, file_size, 0);

        free(file_content);
        fclose(file);
    }
    else {
        send_head(conn, 404, strlen(ERROR_404));
        (void)send(conn, ERROR_404, strlen(ERROR_404), 0);
    }
}

void send_head(connection conn, int stat, int len)
{
    char* statstr, buff[BUFFSIZE];

    /* convert the status code to a string */

    switch (stat) {
    case 200:
        statstr = "OK";
        break;
    case 400:
        statstr = "Bad Request";
        break;
    case 404:
        statstr = "Not Found";
        break;
    default:
        statstr = "Unknown";
        break;
    }

    /*
     * send an HTTP/1.0 response with Server, Content-Length,
     * and Content-Type headers.
     */

    (void)sprintf(buff, "HTTP/1.0 %d %s\r\n", stat, statstr);
    (void)send(conn, buff, strlen(buff), 0);

    (void)sprintf(buff, "Server: %s\r\n", SERVER_NAME);
    (void)send(conn, buff, strlen(buff), 0);

    (void)sprintf(buff, "Content-Length: %d\r\n", len);
    (void)send(conn, buff, strlen(buff), 0);

    (void)sprintf(buff, "Content-Type: text/html\r\n");
    (void)send(conn, buff, strlen(buff), 0);

    (void)sprintf(buff, "\r\n");
    (void)send(conn, buff, strlen(buff), 0);
}