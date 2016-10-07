/*
 * IrcBot.h
 *
 *  Thx to Tyler Allen
 */
 
#ifndef IRCBOT_H_
#define IRCBOT_H_

#include <stdarg.h>


using namespace std;
struct MsgInfo{
	const char *channel;
	const char *user;
};
 
class IrcBot
{
public:
    IrcBot(char *_nick, char *_usr, char *_host, char *_channel, bool _twitch);
    virtual ~IrcBot();
 
    bool setup;
 
    void start();
    bool charSearch(char *toSearch, char *searchFor);
 
private:
	char *host;
    char *port;
	char *channel;	
	int conn;
 
    char *nick;
    char *usr;
	
	bool twitch; 

    bool isConnected(char *buf);
    char * timeNow();
	void raw(char const *fmt, ...);
	void vraw(char const *fmt, va_list ap);
	void privmsg(const struct MsgInfo *const info, const char *fmt, ...);

	void find_answer(char *msg, const char *user);
	void greet(char *msg, const char *user);
	const char* find_match(const char *msg);

	void load_from_file(void);
	int write_to_file(void);

	struct Match* alloc_match(const char *keyw);
	int add_match(char *msg, const struct MsgInfo *const info);
	struct Match* get_match(const char *keyw);
};
 
#endif /* IRCBOT_H_ */
