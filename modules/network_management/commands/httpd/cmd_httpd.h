#ifndef CMD_HTTPD_H
#define CMD_HTTPD_H

/*
 * cmd_httpd.h - `httpd start [port] | stop | status`
 *
 * A small web server in the background, serving the files in /var/www.
 */

void cmd_httpd(const char *args);

#endif
