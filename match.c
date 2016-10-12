
#include "bot.h"
#include "match.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct MatchList matchlist = {
	.m_buf = NULL,
	.used = 0,
	.size = 0
};

static struct Match* get_match(const char *keyw)
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

static struct Match* alloc_match(const char *keyw)
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

int match_add(char *msg, const struct MsgInfo *const info)
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

static void remove_match(struct Match *m)
{
	size_t i;
	for(i = 0; i < m->used; i++){
		free((void*)m->react[i]);
	}
	memmove(m, m+1, (&matchlist.m_buf[matchlist.used]-m)*sizeof(struct Match));
	matchlist.used--;
}

int match_del(char *msg, const struct MsgInfo *const info)
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

void match_dropall(void)
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

int match_show(char *msg, const struct MsgInfo *const info)
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

int match_list(char *msg, const struct MsgInfo *const info)
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

const char* match_find(const char *msg)
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

int match_writefile(const char *filename)
{
	FILE *file;
	int i,j;

	if(filename == NULL){
		return 0;
	}
	if( (file = fopen(filename, "w")) == NULL){
		return 0;
	}
	for(i=0; i < matchlist.used; i++){
		const struct Match *m = &matchlist.m_buf[i];
		for(j = 0 ;j < m->used ;j++){
			fprintf(file, "!add <%s> <%s>\n", m->keyw, m->react[j]);
		}
	}
	fclose(file);
	return 1;
}

int match_readfile(const char *filename)
{
	FILE *file;
	char line[GLOBAL_BUFSIZE];
	int errorcode = 1;

	if(filename == NULL || (file = fopen(filename , "r")) == NULL){
		return 0;
	}

	while(fgets(line, GLOBAL_BUFSIZE, file) != NULL){

		char *nl = strchr(line, '\n');
		if(nl != NULL){
			*nl = '\0';
		}

		if(strncmp(line, "!add", 4) == 0){
			match_add(line, NULL);
		}else if(line[0] == '!'){
			errorcode = 0;
			break;
		}
	}
	fclose(file);
	return errorcode;
}

