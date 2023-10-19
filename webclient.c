/* webclient.c */

#include <stdlib.h>
#include <stdio.h>
#include <cnaiapi.h>

#define BUFFSIZE	256

/*-----------------------------------------------------------------------
 *
 * Program: webclient
 * Purpose: fetch page from webserver and dump to stdout with headers
 * Usage:   webclient <compname> <path> [appnum]
 * Note:    Appnum is optional. If not specified the standard www appnum
 *          (80) is used.
 *
 *-----------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{

  	computer	comp;
	appnum		app;
	connection	conn;
	char		buff[BUFFSIZE];
	int		len;

	if (argc < 3 || argc > 4) {
		(void) fprintf(stderr, "%s%s%s", "usage: ", argv[0], 
			       " <compane> <path> [appnum]\n");
		exit(1);
	}

	/* convert arguments to binary computer and appnum */

	comp = cname_to_comp(argv[1]);
	if (comp == -1)
		exit(1);

	if (argc == 4) 
		app = (appnum) atoi(argv[3]);
	else
		if ((app = appname_to_appnum("www")) == -1)
			exit(1);
	
	/* contact the web server */

	conn = make_contact(comp, app);
	if (conn < 0) 
		exit(1);

	/* send an HTTP/1.0 request to the webserver */

	len = sprintf(buff, "GET %s HTTP/1.0\r\n\r\n", argv[2]);
	(void) send(conn, buff, len, 0);

	/* dump all data received from the server to stdout */

	while((len = recv(conn, buff, BUFFSIZE, 0)) > 0)
		(void) write(STDOUT_FILENO, buff, len);

	return 0;
}
