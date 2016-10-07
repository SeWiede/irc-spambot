#include <iostream>
#include "IrcBot.h"


using namespace std;


int main()
{
	IrcBot bot = IrcBot((char *)"wiedebot", (char *)"wiedebot",
								 (char*)"irc.twitch.tv", (char*)"#wiede5335", true);
	bot.start();

  return 0;

}
