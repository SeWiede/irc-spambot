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

#include "bot.h"
#include "admin.h"

#include "server.local.h"


#define MATCH_REACT_MAX (10)

int conn;
int twitch = 0;
char sbuf[GLOBAL_BUFSIZE];
volatile static sig_atomic_t quit = 0;

struct Match{
	const char *keyw;
	const char *react[MATCH_REACT_MAX];
	size_t used;
};

struct{
	struct Match *m_buf;
	size_t used, size;
} matchlist = {
	.m_buf = NULL,
	.used = 0,
	.size = 0
};

char* cut_token(char *s)
{
	static const char STOP[] = "\r\n \t";
	char *r = s;
	while(*s != '\0' && strchr(STOP, *s) == NULL){
		s++;
	}
	*s = '\0';
	return r;
}
char* skip_whitespace(char *s)
{
	while(*s != '\0' && isspace(*s)){
		s++;
	}
	return s;
}

void free_resources(void){
	free_adminlist();
	if(matchlist.m_buf != NULL){
		free(matchlist.m_buf);
	}
}

static void signal_handler(int sig)
{
	quit=1;
}
void vraw(char *fmt, va_list ap)
{
	vsnprintf(sbuf, GLOBAL_BUFSIZE, fmt, ap);
	printf("<< %s", sbuf);
	write(conn, sbuf, strlen(sbuf));
}
void raw(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vraw(fmt, ap);
	va_end(ap);
}
void privmsg(const struct MsgInfo *const info, const char *fmt, ...)
{
	char PRIVMSG_FMT[32] = ":%s!%s@%s.%s PRIVMSG %s :";

	va_list ap;
	char cfmt[GLOBAL_BUFSIZE];

	assert(channel != NULL);

	if(info->user != NULL && twitch){
		/* we send a twitch whisper */
		strcat(PRIVMSG_FMT, "/w %s ");
		snprintf(cfmt, GLOBAL_BUFSIZE, PRIVMSG_FMT, nick, nick, nick, myhostname, info->channel, info->user);
	}else{
		/* normal IRC message */
		const char *dst = info->channel;
		if(info->user != NULL) dst = info->user;
		snprintf(cfmt, GLOBAL_BUFSIZE, PRIVMSG_FMT, nick, nick, nick, myhostname, dst);
	}

	strncat(cfmt, fmt, GLOBAL_BUFSIZE - strlen(sbuf) - 1);

	va_start(ap, fmt);
	vraw(cfmt, ap);
	va_end(ap);
}

struct Match* get_match(const char *keyw)
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

struct Match* alloc_match(const char *keyw)
{
	struct Match *m;
	
	if(matchlist.used == matchlist.size){
		matchlist.size += 1024;
		matchlist.m_buf = realloc(matchlist.m_buf, matchlist.size*sizeof(struct Match));
		assert(matchlist.m_buf != NULL);
	}
	m = &matchlist.m_buf[matchlist.used++];
	m->keyw = strdup(keyw);
	assert(m->keyw != NULL);
	m->used = 0;
	return m;
}

int add_match(char *msg, const struct MsgInfo *const info)
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

void remove_match(struct Match *m)
{
	size_t i;
	for(i = 0; i < m->used; i++){
		free((void*)m->react[i]);
	}
	memmove(m, m+1, (&matchlist.m_buf[matchlist.used]-m)*sizeof(struct Match));
	matchlist.used--;
}

int del_match(char *msg, const struct MsgInfo *const info)
{
	char *keyw = strstr(msg, "<");
	if(keyw == NULL) return 0;
	char *react = strstr(keyw+1, "<");
	
	char *s;
	struct Match *m;
	size_t i;
	++keyw;
	if(react != NULL) ++react;

	s = strstr(keyw, ">");
	if(s == NULL) return 0;
	*s = '\0';
	if(react != NULL){
		s = strstr(react, ">");
		if(s == NULL) return 0;
		*s = '\0';
	}

	str_lower(keyw);
	if((m = get_match(keyw)) == NULL) return 0;
	
	if(react != NULL){
		for(i = 0; i < m->used; i++){
			if(strstr(m->react[i], react)){
				privmsg(info, "removing reaction %i\r\n", i);
				free((void*)m->react[i]);
				memmove(&m->react[i], &m->react[i+1], (m->used - i)*sizeof(const char*));
				i--;
				m->used--;
			}
		}
		if(m->used == 0){
			remove_match(m);
			privmsg(info, "removed match completely\r\n");
		}
	}else{
		remove_match(m);
		privmsg(info, "removed match completely\r\n");
	}
	return 1;
}

void drop_all_matches()
{
	struct Match *m;
	size_t i,j;
	for(i = 0; i < matchlist.used; i++){
		m = &matchlist.m_buf[i];
		for(j = 0; j < m->used; j++){
			free((void*)m->react[j]);
		}
		free((void*)m->keyw);
	}
	free(matchlist.m_buf);
	matchlist.m_buf = NULL;
	matchlist.size = matchlist.used = 0;
}

int show_match(char *msg, const struct MsgInfo *const info)
{
	const char *keyw = strstr(msg, "<");
	struct Match *m;
	size_t i;
	char *s;
	if(keyw == NULL) return 0;
	++keyw;
	s = strstr(keyw, ">");
	if(s == NULL) return 0;
	*s = '\0';

	if((m = get_match(keyw)) == NULL){
		privmsg(info, "%s\r\n", "NOT FOuND");
		return 0;
	}
	
	privmsg(info, "KEYWORD <%s>\r\n", m->keyw);
	for(i = 0; i < m->used; i++){
		privmsg(info, "[%i] %s\r\n", i, m->react[i]);
	}
	return 1;
}

int list_matches(char *msg, const struct MsgInfo *const info)
{
	struct Match *m;
	size_t i;
	for(i = 0; i < matchlist.used; i++){
		m = &matchlist.m_buf[i];
		privmsg(info, "[%i] <%s> (%i reactions)\r\n",i,m->keyw,m->used);
	}
	if(matchlist.used == 0){
		privmsg(info, "<empty>\r\n");
	}
	return 1;
}

const char* find_match(const char *msg)
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

void cmd_interpret(char *msg, const struct MsgInfo *const info)
{
	int error = 0;

	if(!is_admin(info->user)){
		privmsg(info, "you don't have the permission to do that!\r\n");
		return;
	}

	if(strncmp(msg, "!add", 4) == 0){
		error = add_match(msg+5, info);
	}else if(strncmp(msg, "!del", 4) == 0){
		error = del_match(msg+5, info);
	}else if(strncmp(msg, "!show", 5) == 0){
		error = show_match(msg+6, info);
	}else if(strncmp(msg, "!list", 5) == 0){
		error = list_matches(msg+6, info);
	}else if(strncmp(msg, "!perm", 5) == 0){
		error = add_admin(msg+6, info);
	}else if(strncmp(msg, "!unperm", 7) == 0){
		error = del_admin(msg+8, info);
	}else if(strncmp(msg, "!commanders", 11) == 0){
		error = list_admins(msg+12, info);
	}else if(strncmp(msg, "!dropall", 8) == 0){
		error = 1;
		drop_all_matches();
		privmsg(info, "dropped everything\r\n");
	}

	if(error == 0){
		privmsg(info, "error!\r\n");
	}
}

const char* rand_msg_starter()
{
	const char *s;
	if((s = find_match(""))){
		return s;
	}else{
		return NULL;
	}

}

void greet(char *msg, const char *user)
{
	strcpy(msg, "Greetings");
	if(user != NULL){
		strcat(msg, ", ");
		strcat(msg, user);
	}
	strcat(msg, "!");
}
void str_lower(char *str){
	char *c;
	for(c = str; *c != '\0'; c++){
		*c = *c <= 127 ? tolower(*c) : *c;
	}
}
void find_answer(char *msg, const char *user)
{
	if(strstr(msg, "hello") || strstr(msg, "hi ") || strstr(msg, "hey ")){
		if(strstr(msg, shortnick)){
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

void load_from_file(void)
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

int write_to_file(void)
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

void setup_signalhandler(void)
{
	static const int signals[] = {SIGINT, SIGTERM};
	int i;
	struct sigaction s;
	s.sa_handler = signal_handler;
	s.sa_flags = 0;

	if(sigfillset(&s.sa_mask) < 0){
		printf("error sigfillset\n");
		exit(1);
	}
	for(i=0; i < COUNT_OF(signals); i++){
		if(sigaction(signals[i], &s, NULL) < 0){
			printf("error sigaction!\n");
			exit(1);
		}
	}	
}

int main(int argc, char* argv[])
{

	char curmsg[GLOBAL_BUFSIZE+1];
	char *user, *command, *where, *message, *sep, *target;
	int i, j, l, sl, o = -1, start, wordcount;
	char buf[GLOBAL_BUFSIZE+1];
	struct addrinfo hints, *res;
	time_t last_msg = time(NULL);
	struct MsgInfo msg_info= {
		.channel = channel,
		.user = NULL
	};

	atexit(free_resources);

	setup_signalhandler();

	matchlist.size = 4096;
	matchlist.used = 0;
	matchlist.m_buf = calloc(sizeof(struct Match), matchlist.size);
	assert(matchlist.m_buf != NULL);

	load_from_file();
	load_admins();

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
	printf("checking arguments for connection: ...\n");
	while(1){
		static struct option opts[] = {
			{"twitch", no_argument, 0, 't'}, //, ...
			{0,0,0,0}
			//...
		};
		int opt_index =0;
		signed char c = getopt_long(argc, argv, "t", opts, &opt_index);
		if(c==-1)
			break;
		switch(c) {
		case 't':
			twitch =1;	
			break;
		default:
			break;
		}
	}

	if(twitch){
		printf("twitch was chosen!\n");
		raw("PASS %s\r\n", auth);  /* <- twitch */
		raw("NICK %s\r\n", nick);	
	}else{
		printf("default\n");
		raw("USER %s 0 0 :%s\r\n", nick, nick);
		raw("NICK %s\r\n", nick);
	}

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
						if (where == NULL || message == NULL) continue;
						if ((sep = strchr(user, '!')) != NULL) user[sep - user] = '\0';
						if (where[0] == '#' || where[0] == '&' || where[0] == '+' || where[0] == '!') target = where; else target = user;
						printf("[from: %s] [reply-with: %s] [where: %s] [reply-to: %s] %s", user, command, where, target, message);
					}

					if(strcmp(command, "PRIVMSG") == 0){
						strcpy(curmsg, message);
						curmsg[GLOBAL_BUFSIZE] = '\0';
						if(curmsg[0] == '!'){
							msg_info.user = user;
							cmd_interpret(curmsg, &msg_info);
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
					const char *s = rand_msg_starter();
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
	int error_code = write_to_file();
	printf("BYE :) \n");
	return error_code;
}
