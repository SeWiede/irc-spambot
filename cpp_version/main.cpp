#include <iostream>
#include "IrcBot.h"


using namespace std;


int main()
{
	string predef_ads[] = {"wiede5335"};
	IrcBot bot = IrcBot("wiedebot", "wiedebot",
						"irc.twitch.tv", "#wiede5335", true, 
						predef_ads);
	bot.start();

  return 0;

}
