/*Thx to Tyler Allen
*/
#include "IrcBot.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <stdarg.h>
#include <getopt.h>
#include <time.h>
#include <signal.h>

#include "server.local.h"
 
using namespace std;

#define COUNT_OF(x) (sizeof(x)/sizeof(x[0]))
 
#define GLOBAL_BUFSIZE 512
#define MATCH_REACT_MAX 10

char sbuf[GLOBAL_BUFSIZE];
volatile static sig_atomic_t quit = 0;

struct Match{
	const char *keyw;
	const char *react[MATCH_REACT_MAX];
	size_t used;
};

static struct{
	struct Match *m_buf;
	size_t used, size;
} matchlist = {
	.m_buf = NULL,
	.used = 0,
	.size = 0
};

static void signal_handler(int sig)
{
	quit=1;
}
 
IrcBot::IrcBot(char *_nick, char *_usr, char *_host, char *_channel, bool _twitch)
{
    nick = _nick;
    usr = _usr;
	host = _host;
	channel = _channel;
	twitch = _twitch;
}
 
IrcBot::~IrcBot()
{
    close (conn);
}

void str_lower(char *str){
	char *c;
	for(c = str; *c != '\0'; c++){
		*c = *c <= 127 ? tolower(*c) : *c;
	}
}
 
void IrcBot::start()
{
	static const int signals[] = {SIGINT, SIGTERM};
	struct sigaction s;
	s.sa_handler = signal_handler;
	s.sa_flags = 0;

	if(sigfillset(&s.sa_mask) < 0){
		printf("error sigfillset\n");
		exit(1);
	}
	for(int i=0; i < COUNT_OF(signals); i++){
		if(sigaction(signals[i], &s, NULL) < 0){
			printf("error sigaction!\n");
			exit(1);
		}
	}
	
    char curmsg[GLOBAL_BUFSIZE+1];
	char buf[GLOBAL_BUFSIZE+1];
	char *user, *command, *where, *message, *sep, *target;
	int i, j, l, sl, o = -1, start, wordcount;	
	struct addrinfo hints, *res;
	time_t last_msg = time(NULL);
	struct MsgInfo msg_info= {
		.channel = channel,
		.user = NULL
	};


	load_from_file();

    port = (char*)"6667";
 
    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	printf("getting data from: %s port %s\n", host, port);
	getaddrinfo(host, port, &hints, &res);
	printf("creating socket....\n");
	conn = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	printf("connecting...\n");
	connect(conn, res->ai_addr, res->ai_addrlen);
	printf("done!\n");
 
    //We dont need this anymore
    freeaddrinfo(res);
 
	if(twitch){
		printf("twitch was chosen!\n");
		raw("PASS %s\r\n", auth);  /* <- twitch */
		raw("NICK %s\r\n", nick);	
	}else{
		printf("default\n");
		raw("USER %s 0 0 :%s\r\n", nick, nick);
		raw("NICK %s\r\n", nick);
	}
	 
 
    int count = 0;
    while (!quit && (sl = read(conn, sbuf, GLOBAL_BUFSIZE))) {
		for (i = 0; i < sl; i++) {
			o++;
			buf[o] = sbuf[i];
			if ((i > 0 && sbuf[i] == '\n' && sbuf[i - 1] == '\r') || o == GLOBAL_BUFSIZE) {
				buf[o + 1] = '\0';
				l = o;
				o = -1;

				printf(">> %s", buf);

				if (!strncmp(buf, "PING", 4)) {
					buf[1] = 'O';
					raw(buf);
				} else if (buf[0] == ':') {
					wordcount = 0;
					user = command = where = message = NULL;
					for (j = 1; j < l; j++) {
						if (buf[j] == ' ') {
							buf[j] = '\0';
							wordcount++;
							switch(wordcount) {
								case 1: user = buf + 1; break;
								case 2: command = buf + start; break;
								case 3: where = buf + start; break;
							}
							if (j == l - 1) continue;
							start = j + 1;
						} else if (buf[j] == ':' && wordcount == 3) {
							if (j < l - 1) message = buf + j + 1;
							break;
						}
					}

					if (wordcount < 2) continue;

					if (!strncmp(command, "001", 3) && channel != NULL) {
						raw("JOIN %s\r\n", channel);
						if(twitch){
							raw("CAP REQ :twitch.tv/membership\n");
							raw("CAP REQ :twitch.tv/commands\n");	
						}
					} else if (!strncmp(command, "PRIVMSG", 7) || !strncmp(command, "NOTICE", 6)) {
						if (where == NULL || message == NULL) 
							continue;
						if ((sep = strchr(user, '!')) != NULL) 
							user[sep - user] = '\0';
						if (where[0] == '#' || where[0] == '&' || where[0] == '+' || where[0] == '!') 
							target = where; 
						else 
							target = user;
						printf("[from: %s] [reply-with: %s] [where: %s] [reply-to: %s] %s", 
													user, command, where, target, message);
					}

					if(strcmp(command, "PRIVMSG") == 0){
						strcpy(curmsg, message);
						curmsg[GLOBAL_BUFSIZE] = '\0';
						if(curmsg[0] == '!'){
							msg_info.user = user;
							//cmd_interpret(curmsg, &msg_info);
						}else{
							find_answer(curmsg, user);
							if(curmsg[0] != '\0'){
								strcat(curmsg, "\r\n");
								msg_info.user = NULL;
								privmsg(&msg_info, "%s", curmsg);
							}
						}
						last_msg = time(NULL);
					}
				}

				if(time(NULL) - last_msg > 300){
					const char *s;// = rand_msg_starter();
					if(s != NULL){
						strcpy(curmsg, s);
						strcat(curmsg, "\r\n");
						msg_info.user = NULL;
						privmsg(&msg_info, "%s", curmsg);
					}
					last_msg = time(NULL)+150;
				}

			}
		}

	}
	cout << "----------------------CONNECTION CLOSED---------------------------"<< endl;
    cout << timeNow() << endl;
	printf("Writing to file %s was : %s - Bye :) \n", addLog, (write_to_file() ? "unsuccessful":"successful"));
}

const char* IrcBot::find_match(const char *msg)
{
	size_t i;
	for(i = 0; i < matchlist.used; i++){
		const struct Match *m = &matchlist.m_buf[i];
		if((m->keyw[0] != '\0' && strstr(msg, m->keyw)) || 
			(m->keyw[0] == '\0' && msg[0] == '\0')){
			return m->react[rand() % m->used];
		}
	}
	return NULL;
}

void IrcBot::greet(char *msg, const char *user)
{
	strcpy(msg, "Greetings");
	if(user != NULL){
		strcat(msg, ", ");
		strcat(msg, user);
	}
	strcat(msg, "!");
}

void IrcBot::find_answer(char *msg, const char *user)
{
	if(strstr(msg, "hello") || strstr(msg, "hi ") || strstr(msg, "hey ")){
		if(strstr(msg, nick)){
			greet(msg, user);
			return;
		}
	}

	const char *s = find_match(msg);
	if(s != NULL){
		strcpy(msg, s);
		return;
	}
	
	msg[0] = '\0';
}

bool IrcBot::isConnected(char *buf)
{//returns true if "/MOTD" is found in the input strin
    //If we find /MOTD then its ok join a channel
    if (strcmp(buf,"/MOTD") == true)
        return true;
    else
        return false;
}
 
char * IrcBot::timeNow()
{//returns the current date and time
    time_t rawtime;
    struct tm * timeinfo;
 
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
 
    return asctime (timeinfo);
}

void IrcBot::load_from_file(void)
{
	FILE *file;
	char line[GLOBAL_BUFSIZE];
	int lastpos;

	if(addLog == NULL){
		return;
	}
	if( (file = fopen(addLog, "r")) == NULL){
		printf("error opening File %s\n", addLog);
		return;
	}
	while(fgets(line, GLOBAL_BUFSIZE, file) != NULL){

		line[GLOBAL_BUFSIZE-1] = '\0';
		lastpos = strlen(line)-1;

		if(line[lastpos] == '\n' || line[lastpos] == '\0')
			line[lastpos] = '\0';
		else{
			printf("come on you shit! I dont want to deal with that - file ignored!\n");
			file = NULL;
			break;
		}
		
		if(strncmp(line, "!add", 4) == 0){
			if(add_match(line, NULL)){
				printf("%s successfully added to commands\n", line);
			}else{
				printf("error while adding %s to commands\n", line);
			}
		}else if(line[0] == '!'){
			printf("invalid file format!\n");
			break;
		}
	}
	fclose(file);
}

int IrcBot::write_to_file(void)
{
	FILE *file;
	int i,j;

	if(addLog == NULL){
		return 0;
	}
	printf("saving used commands in file %s\n", addLog);
	if( (file = fopen(addLog, "w")) == NULL){
		printf("could not open file %s for writing\n", addLog);
		return 1;
	}
	for(i=0; i < matchlist.used; i++){
		const struct Match *m = &matchlist.m_buf[i];
		for(j = 0 ;j < m->used ;j++){
			fprintf(file, "!add <%s> <%s>\n", m->keyw, m->react[j]);
		}	
	}
	fclose(file);
	return 0;
}

struct Match* IrcBot::alloc_match(const char *keyw)
{
	struct Match *m;
	
	if(matchlist.used == matchlist.size){
		matchlist.size += 1024;
		matchlist.m_buf = (struct Match *)realloc(matchlist.m_buf, matchlist.size*sizeof(struct Match));
		assert(matchlist.m_buf != NULL);
	}
	m = &matchlist.m_buf[matchlist.used++];
	m->keyw = strdup(keyw);
	assert(m->keyw != NULL);
	m->used = 0;
	return m;
}

struct Match* IrcBot::get_match(const char *keyw)
{
	struct Match *m;
	size_t i;
	for(i = 0; i < matchlist.used; i++){
		m = &matchlist.m_buf[i];
		if(strcmp(keyw, m->keyw) == 0){
			return m;
		}
	}
	return NULL;
}

int IrcBot::add_match(char *msg, const struct MsgInfo *const info)
{
	char *keyw = strstr(msg, "<");
	if(keyw == NULL) return 0;
	char *react = strstr(keyw+1, "<");
	if(react == NULL) return 0;

	char *s;
	struct Match *m;
	++keyw;
	++react;
	s = strstr(keyw, ">");
	if(s == NULL) return 0;
	*s = '\0';
	s = strstr(react, ">");
	if(s == NULL) return 0;
	*s = '\0';

	str_lower(keyw);
	if((m = get_match(keyw))){
		if(m->used >= MATCH_REACT_MAX) return 0;
	}else{
		m = alloc_match(keyw);
	}
	s = strdup(react);
	assert(s != NULL);
	m->react[m->used++] = s;
	if(info != NULL){
		privmsg(info, "added to match <%s> reaction: '%s'\r\n", keyw, s);
	}
	
	return 1;
}

void IrcBot::vraw(char const *fmt, va_list ap)
{
	vsnprintf(sbuf, GLOBAL_BUFSIZE, fmt, ap);
	printf("<< %s", sbuf);
	write(conn, sbuf, strlen(sbuf));
}
void IrcBot::raw(char const *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vraw(fmt, ap);
	va_end(ap);
}
void IrcBot::privmsg(const struct MsgInfo *const info, const char *fmt, ...)
{
	char PRIVMSG_FMT[32] = ":%s!%s@%s.%s PRIVMSG %s :";

	va_list ap;
	char cfmt[GLOBAL_BUFSIZE];

	assert(channel != NULL);

	if(info->user != NULL && twitch){
		/* we send a twitch whisper */
		strcat(PRIVMSG_FMT, "/w %s ");
		snprintf(cfmt, GLOBAL_BUFSIZE, PRIVMSG_FMT, nick, nick, nick, host, info->channel, info->user);
	}else{
		/* normal IRC message */
		const char *dst = info->channel;
		if(info->user != NULL) dst = info->user;
		snprintf(cfmt, GLOBAL_BUFSIZE, PRIVMSG_FMT, nick, nick, nick, host, dst);
	}

	strncat(cfmt, fmt, GLOBAL_BUFSIZE - strlen(sbuf) - 1);

	va_start(ap, fmt);
	vraw(cfmt, ap);
	va_end(ap);
}
