#ifndef ADMINS_H_
#define ADMINS_H_

#include <string>
#include <vector>
#include "IrcBot.h"

using namespace std;


class Admins
{
public:
	Admins();
	Admins(vector<string> _admins);
	Admins(string predefined_admins[]); 
 	virtual ~Admins();
	bool add_admin(string& msg);
	bool del_admin(string& msg);
	void list_admins(string& msg);
	bool is_admin(string name);
	void load_admins(vector<string> a);
	void load_admins(string a[]);
	int count_admins();
private:
	vector<string> admins;	
};
 
#endif /* ADMINS_H_ */
