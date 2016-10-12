#pragma once

#include <stdlib.h>

#define MATCH_REACT_MAX 10
struct Match{
	const char *keyw;
	const char *react[MATCH_REACT_MAX];
	size_t used;
};


extern struct MatchList{
	struct Match *m_buf;
	size_t used, size;
} matchlist;

int match_add(char *msg, const struct MsgInfo *const info);
int match_del(char *msg, const struct MsgInfo *const info);
int match_show(char *msg, const struct MsgInfo *const info);
int match_list(char *msg, const struct MsgInfo *const info);
void match_dropall(void);
const char* match_find(const char *msg);
int match_readfile(const char *filename);
int match_writefile(const char *filename);

