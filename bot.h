#pragma once

struct MsgInfo{
	const char *channel;
	const char *user;
};

void vraw(char *fmt, va_list ap);
void raw(char *fmt, ...);
void privmsg(const struct MsgInfo *const info, const char *fmt, ...);

void str_lower(char*);

