/*Thx to Tyler Allen
*/
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
#include <getopt.h>

#include <string>
#include "IrcBot.h"
#include <iostream>
#include <fstream>
#include <algorithm>

#include "server.local.h"

 
using namespace std;

#define COUNT_OF(x) (sizeof(x)/sizeof(x[0]))
 
#define GLOBAL_BUFSIZE 512

char sbuf[GLOBAL_BUFSIZE];
volatile static sig_atomic_t quit = 0;

vector<Match> matchlist;
Admins admins;


static void signal_handler(int sig)
{
	quit=1;
}
 
IrcBot::IrcBot(string _nick, string _usr, string _host, string _channel, bool _twitch)
{
    nick = _nick;
    usr = _usr;
	host = _host;
	channel = _channel;
	twitch = _twitch;
}

IrcBot::IrcBot(string _nick, string _usr, string _host, string _channel, bool _twitch,
				const string predef_ads[])
{
    nick = _nick;
    usr = _usr;
	host = _host;
	channel = _channel;
	twitch = _twitch;
	admins = Admins(predef_ads);
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
 
const string IrcBot::find_match(const string msg)
{
	for(Match m : matchlist){
		if(msg.find(m.get_keyw()) != string::npos){
			return m.get_react();
		}
	}
	return string();
}

void IrcBot::greet(string& msg, const string user)
{
	msg = "Greetings";
	if(!user.empty()){
		msg += ", " + user + "!";
	}
}

void IrcBot::find_answer(string& msg, const string user)
{
	if(msg.find("hello ") != string::npos || msg.find("hi ") != string::npos 
	|| msg.find("hey ") != string::npos){
		if(msg.find(nick) != string::npos){
			greet(msg, user);
			return;
		}
	}

	const string s = find_match(msg.c_str());
	if(!s.empty()){
		msg = s;
		return;
	}

	msg.clear();
}
 
string IrcBot::timeNow()
{//returns the current date and time
    time_t rawtime;
    struct tm * timeinfo;
 
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
 
    return asctime (timeinfo);
}

void IrcBot::load_from_file(void)
{
	string line;

	if(addLog.empty()){
		return;
	}

	ifstream file(addLog.c_str());
	
	if(!file.is_open()){
		cout << "error opening File " + addLog + "\n";
		return;
	}

	while(getline(file, line)){

		if(line.length() > GLOBAL_BUFSIZE){
			printf("come on you shit! I dont want to deal with that - file ignored!\n");
			file.close();
			break;
		}
		
		if(line.find("!add",0, 4) == 0){
			if(add_match(line, NULL)){
				//printf("%s successfully added to commands\n", line);
			}else{
				cout << "error while adding " + line + " to commands\n";
			}
		}else if(line.find("!" ,0 ,1) == 0){
			printf("invalid file format!\n");
			break;
		}
	}
	file.close();
}

bool IrcBot::write_to_file(void)
{
	if(addLog.empty()){
		return 0;
	}
	cout << "saving used commands in file " + addLog + "\n";
	ofstream file(addLog.c_str());
	if(!file.is_open()){
		cout << "could not open file " + addLog + " for writing\n";
		return 1;
	}

	for(Match m : matchlist){
		for(int j = 0 ;j < m.get_reactNum();j++){
			file << "!add <" + m.get_keyw() + "> <" + m.get_react(j)+ ">\n";
		}	
	}
	file.close();
	return 0;
}

int IrcBot::get_matchindex(const string keyw)
{	
	std::vector<Match>::iterator it = matchlist.begin();
	for(uint8_t i=0; i < matchlist.size(); i++, ++it){
		if((*it).get_keyw().compare(keyw) == 0){
			return i;
		}
	}
	return -1;
}

bool IrcBot::add_match_helper(const string keyw, const string react){
	int i = get_matchindex(keyw);
	if(i == -1){
		Match *m = new Match(keyw);
		(*m).add_react(react);
		matchlist.push_back(*m);
	}else{
		Match& m = matchlist[i];
		if(!m.add_react(react))
			return false;
	}
	return true;
}

int IrcBot::add_match(string msg, const struct MsgInfo *const info)
{
	int keypos = msg.find("<");
	if(keypos == -1) return 0;
	int reactpos = msg.find("<", keypos+2, 1);
	if(reactpos == -1) return 0;

	int keyend = msg.find(">", 1);
	if(keyend == -1 || keyend > reactpos || keypos >= keyend) return 0;
	int reactend = msg.find(">", reactpos + 1, 1);
	if(reactend == -1 || reactpos >= reactend) return 0;

	//str_lower(keyw);
	
	string keyw = msg.substr(keypos+1, keyend-keypos-1);
	string react = msg.substr(reactpos+1, reactend-reactpos-1);

	if(!add_match_helper(keyw, react))
		return 0;
	
	cout << "added \"" + keyw + "\" with reaction: \"" + react + "\" to commands \n";
	
	if(info != NULL){
		privmsg(info, "added to match <" + keyw + "> reaction: '" + react +" '\r\n");
	}	
	return 1;
}

bool IrcBot::del_match_helper(const string keyw, const string react){
	int i = get_matchindex(keyw);
	if(i == -1){
		return false;
	}else{
		Match& m = matchlist[i];
		if(!m.del_react(react))
			return false;
		if(m.is_empty()){
			matchlist.erase(matchlist.begin() + i);
		}
	}
	return true;
}

int IrcBot::del_match(string& msg)
{
	int keypos = msg.find("<");
	if(keypos == -1) return 0;
	int reactpos = msg.find("<", keypos+2, 1);
	if(reactpos == -1) return 0;

	int keyend = msg.find(">", 1);
	if(keyend == -1 || keyend > reactpos || keypos >= keyend) return 0;
	int reactend = msg.find(">", reactpos + 1, 1);
	if(reactend == -1 || reactpos >= reactend) return 0;

	//str_lower(keyw);
	
	string keyw = msg.substr(keypos+1, keyend-keypos-1);
	string react = msg.substr(reactpos+1, reactend-reactpos-1);

	if(!del_match_helper(keyw, react))
		return 0;
	msg = "deleted \"" + keyw + "\" with reaction: \"" + react + "\" from commands ";
	cout << msg << endl;	
	return 1;
}

void IrcBot::raw(string send)
{
	//printf("<< %s", sbuf);
	cout << "<< " + send; //+ "\n";
	int len = send.length();
	if(write(conn, send.c_str(), len) < len)
		cout << "error sending " + send;
}

void IrcBot::privmsg(const struct MsgInfo *const info, string msg)
{
	assert(!channel.empty());
	string send;
	if(!info->user.empty() && twitch){
		/* we send a twitch whisper */
		send = ":" + nick +"!"+ nick + "@" + nick + "." + host + " PRIVMSG " + 
					info->channel + " :/w " + info->user + " ";
	}else{
		/* normal IRC message */
		string dst = info->channel;
		if(!info->user.empty()) dst = info->user;
		send = ":" + nick +"!"+ nick + "@" + nick + "." + host + " PRIVMSG " + 
					dst + " :";
	}
	raw(send + msg);
}

void IrcBot::drop_all_matches(){
	matchlist.clear();
}

void IrcBot::cmd_interpret(string& msg, const struct MsgInfo *const info)
{
	int error = 0;

	if(!admins.is_admin(info->user)){
		privmsg(info, "you don't have the permission to do that!\r\n");
		return;
	}
	if(msg.find("!add",0 ,4) == 0){
		error = add_match(msg, info);
	}else if(msg.find("!del",0 ,4) == 0){
		error = del_match(msg);
	}else if(msg.find("!show",0 ,5) == 0){
		//error = show_match(msg+6, info);
	}else if(msg.find("!list",0 ,5) == 0){
		//error = list_matches(msg+6, info);
	}else if(msg.find("!perm",0 , 5) == 0){
		error = admins.add_admin(msg);
	}else if(msg.find("!unperm",0 ,7) == 0){
		error = admins.del_admin(msg);
	}else if(msg.find("!commanders",0 ,11) == 0){
		admins.list_admins(msg);
		error =1;
	}else if(msg.find("!dropall",0 ,8) == 0){
		error = 1;
		drop_all_matches();
		msg = "dropped everything!";
		cout << msg << endl;	
	}

	if(error == 0){
		privmsg(info, "error!\r\n");
	}else{
		privmsg(info, msg + "\r\n");
	}
}

void IrcBot::start()
{
	static const int signals[] = {SIGINT, SIGTERM};
	struct sigaction s;
	//const char *get_react();
	s.sa_handler = signal_handler;
	s.sa_flags = 0;

	if(sigfillset(&s.sa_mask) < 0){
		printf("error sigfillset\n");
		exit(1);
	}
	for(uint8_t i=0; i < COUNT_OF(signals); i++){
		if(sigaction(signals[i], &s, NULL) < 0){
			printf("error sigaction!\n");
			exit(1);
		}
	}
	
    string curmsg;
	char buf[GLOBAL_BUFSIZE+1];
	char *user, *command, *where, *message, *sep, *target;
	int i, j, l, sl, o = -1, start, wordcount;	
	struct addrinfo hints, *res;
	time_t last_msg = time(NULL);
	struct MsgInfo msg_info= {
		.channel = channel,
		//.user = NULL
	};


	load_from_file();

    port = "6667";
 
    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	cout << "getting data from: " + host + " at port " + port + "\n";
	getaddrinfo(host.c_str(), port.c_str(), &hints, &res);
	cout << "creating socket....\n";
	conn = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	cout << "connecting...\n";
	connect(conn, res->ai_addr, res->ai_addrlen);
	cout << "done!\n";
 
    //We dont need this anymore
    freeaddrinfo(res);
 
	if(twitch){
		printf("twitch was chosen!\n");
		raw("PASS " + auth + "\r\n");  /* <- twitch */
		raw("NICK " + nick + "\r\n");	
	}else{
		printf("default\n");
		raw("USER " + nick + " 0 0 :"+ nick +"\r\n");
		raw("NICK " + nick + "\r\n");
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
					if (!strncmp(command, "001", 3) && !channel.empty()) {
						raw("JOIN " + channel + "\r\n");
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
						curmsg = message;
						if(curmsg.find("!",0,1) == 0){
							msg_info.user = user;
							cmd_interpret(curmsg, &msg_info);
						}else{
							find_answer(curmsg, user);
							if(!curmsg.empty()){
								curmsg += "\r\n";
								msg_info.user.clear();
								privmsg(&msg_info, curmsg);
							}
						}
						last_msg = time(NULL);
					}
				}

				if(time(NULL) - last_msg > 300){
					const char *s;// = rand_msg_starter();
					if(s != NULL){
						curmsg = string(s) + "\r\n";
						msg_info.user.clear();
						privmsg(&msg_info, curmsg);
					}
					last_msg = time(NULL)+150;
				}

			}
		}

	}
	cout << "----------------------CONNECTION CLOSED---------------------------"<< endl;
    cout << timeNow() << endl;
	cout << "Writing to file " + addLog + " was : " + (write_to_file() ? "unsuccessful":"successful")+
			 " - Bye :) \n";
}
