#pragma once
#include <stdarg.h>

#define GLOBAL_BUFSIZE (512)

#define COUNT_OF(x) (sizeof(x)/sizeof(x[0]))
#define FORCE_ASSERT(X)	do{ \
				if(!(X)){ \
					fprintf(stderr,"FORCE_ASSERT at line %d failed\nAborting...\n",__LINE__); \
					abort(); \
				} \
			}while(0)
struct MsgInfo{
	const char *channel;
	const char *user;
};

void vraw(char *fmt, va_list ap);
void raw(char *fmt, ...);
void privmsg(const struct MsgInfo *const info, const char *fmt, ...);

void str_lower(char*);
char* cut_token(char *s);
char* skip_whitespace(char *s);
