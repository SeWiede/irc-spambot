/*
 * IrcBot.h
 *
 *  Thx to Tyler Allen
 */
 
#ifndef IRCBOT_H_
#define IRCBOT_H_

#include <stdarg.h>
#include <string>
#include "Match.h"
#include "Admins.h"

using namespace std;

struct MsgInfo{
	string channel;
	string user;
};
 
class IrcBot
{
public:
    IrcBot(string _nick, string _usr, string _host, string _channel, bool _twitch);
	IrcBot(string _nick, string _usr, string _host, string _channel, bool _twitch,
		string predef_ads[]);
    virtual ~IrcBot();
 
    void start();
    bool charSearch(char *toSearch, char *searchFor);
 
private:
	string host;
    string port;
	string channel;	 
    string nick;
    string usr;
	bool twitch; 

	int conn;


    string timeNow();
	void raw(string send);
	void privmsg(const struct MsgInfo *const info, string msg);

	void find_answer(string& msg, string user);
	void greet(string& msg, string user);
	string find_match(string msg);

	void load_from_file(void);
	bool write_to_file(void);

	int add_match(string msg, const struct MsgInfo *const info);
	bool add_match_helper(string keyw, string react);

	int del_match(string& msg);
	bool del_match_helper(string keyw, string react);

	void drop_all_matches();

	int get_matchindex(string keyw);

	void cmd_interpret(string& msg, const struct MsgInfo *const info);
};
 
#endif /* IRCBOT_H_ */
