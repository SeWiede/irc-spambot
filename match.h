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

int add_match(char *msg, const struct MsgInfo *const info);
int del_match(char *msg, const struct MsgInfo *const info);
int show_match(char *msg, const struct MsgInfo *const info);
int list_matches(char *msg, const struct MsgInfo *const info);
void drop_all_matches(void);
const char* find_match(const char *msg);

