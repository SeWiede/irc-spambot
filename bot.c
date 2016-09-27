#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <stdarg.h>

#include "server.local.h"

int conn;
char sbuf[512];


void raw(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(sbuf, 512, fmt, ap);
	va_end(ap);
	printf("<< %s", sbuf);
	write(conn, sbuf, strlen(sbuf));
}

#define MATCH_REACT_MAX 10
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

int add_match(char *msg)
{
	const char *keyw = strstr(msg, "<");
	const char *react = strstr(keyw+1, "<");
	char *s;
	struct Match *m;
	if(keyw == NULL) return 0;
	if(react == NULL) return 0;
	++keyw;
	++react;
	
	s = strstr(keyw, ">");
	if(s == NULL) return 0;
	*s = '\0';
	s = strstr(react, ">");
	if(s == NULL) return 0;
	*s = '\0';

	if((m = get_match(keyw))){
		if(m->used >= MATCH_REACT_MAX) return 0;
	}else{
		m = alloc_match(keyw);
	}
	s = strdup(react);
	assert(s != NULL);
	m->react[m->used++] = s;
	printf("added to match '%s' reaction '%s'\n", keyw, s);
	
	return 1;
}

void remove_match(struct Match *m)
{
	size_t i;
	for(i = 0; i < m->used; i++){
		free((void*)m->react[i]);
	}
	memmove(m, m+1, &matchlist.m_buf[matchlist.used]-m-1);
}

int del_match(char *msg)
{
	const char *keyw = strstr(msg, "<");
	const char *react = strstr(keyw+1, "<");
	char *s;
	struct Match *m;
	size_t i;
	if(keyw == NULL) return 0;
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

	if((m = get_match(keyw)) == NULL) return 0;
	
	if(react != NULL){
		for(i = 0; i < m->used; i++){
			if(strstr(m->react[i], react)){
				free((void*)m->react[i]);
				memmove(&m->react[i], &m->react[i+1], m->used - i - 1);
				i--;
				m->used--;
			}
		}
		if(m->used == 0){
			remove_match(m);
		}
	}else{
		remove_match(m);
	}

	return 1;
}

int show_match(char *msg, const char *user)
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
		raw(":%s!%s@%s.tmi.twitch.tv PRIVMSG %s :NOT FOUND\r\n",nick,nick,nick,user);
		return 0;
	}
	
	raw(":%s!%s@%s.tmi.twitch.tv PRIVMSG %s :KEYWORD '%s'\r\n",nick,nick,nick,user,m->keyw);
	for(i = 0; i < m->used; i++){
		raw(":%s!%s@%s.tmi.twitch.tv PRIVMSG %s :[%i] %s\r\n",nick,nick,nick,user,i,m->react[i]);
	}
	return 1;
}

const char* find_match(const char *msg)
{
	size_t i;
	for(i = 0; i < matchlist.used; i++){
		const struct Match *m = &matchlist.m_buf[i];
		if(strstr(msg, m->keyw)){
			return m->react[rand() % m->used];
		}
	}
	return NULL;
}

void cmd_interpret(char *msg, const char *user)
{
	if(strncmp(msg, "!add", 4) == 0){
		add_match(msg+5);
	}else if(strncmp(msg, "!del", 4) == 0){
		del_match(msg+5);
	}else if(strncmp(msg, "!show", 5) == 0){
		show_match(msg+6, user);
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

char find_answer(char *msg, const char *user)
{
	char *c;
	int i, count, r, z;

	for(c = msg; *c != '\0'; c++){
		*c = *c <= 127 ? tolower(*c) : *c;
	}
	if(strstr(msg, "hello") || strstr(msg, "hi ") || strstr(msg, "hey ")){
		if(strstr(msg, nick)){
			greet(msg, user);
		}
		return;
	}

	const char *s = find_match(msg);
	if(s != NULL){
		printf("found '%s'\n", s);
		strcpy(msg, s);
		return;
	}
	
	msg[0] = '\0';
}

int main()
{
	char curmsg[513];

	char *user, *command, *where, *message, *sep, *target;
	int i, j, l, sl, o = -1, start, wordcount;
	char buf[513];
	struct addrinfo hints, *res;
	time_t last_msg = time(NULL);

	matchlist.size = 4096;
	matchlist.used = 0;
	matchlist.m_buf = calloc(sizeof(struct Match), matchlist.size);
	assert(matchlist.m_buf != NULL);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(host, port, &hints, &res);
	conn = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	connect(conn, res->ai_addr, res->ai_addrlen);

	/* raw("PASS oauth:...\r\n"); */  /* <- twitch */
	raw("USER %s 0 0 :%s\r\n", nick, nick);
	raw("NICK %s\r\n", nick);

	while ((sl = read(conn, sbuf, 512))) {
		for (i = 0; i < sl; i++) {
			o++;
			buf[o] = sbuf[i];
			if ((i > 0 && sbuf[i] == '\n' && sbuf[i - 1] == '\r') || o == 512) {
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
					} else if (!strncmp(command, "PRIVMSG", 7) || !strncmp(command, "NOTICE", 6)) {
						if (where == NULL || message == NULL) continue;
						if ((sep = strchr(user, '!')) != NULL) user[sep - user] = '\0';
						if (where[0] == '#' || where[0] == '&' || where[0] == '+' || where[0] == '!') target = where; else target = user;
						printf("[from: %s] [reply-with: %s] [where: %s] [reply-to: %s] %s", user, command, where, target, message);
					}

					if(strcmp(command, "PRIVMSG") == 0){
						strcpy(curmsg, message);
						curmsg[512] = '\0';
						if(curmsg[0] == '!'){
							cmd_interpret(curmsg, user);
						}else{
							find_answer(curmsg, user);
							if(curmsg[0] != '\0'){
								raw(":%s!%s@%s.tmi.twitch.tv PRIVMSG %s :%s\r\n",nick,nick,nick,channel,curmsg);
							}
						}

						last_msg = time(NULL);
					}		
				}
				if(time(NULL) - last_msg > 300){
					const char *s = rand_msg_starter();
					if(s != NULL){
						raw(":%s!%s@%s.tmi.twitch.tv PRIVMSG %s :%s\r\n",nick,nick,nick,channel,s);
					}
					last_msg = time(NULL)+150;
				}

			}
		}

	}

	return 0;
}
