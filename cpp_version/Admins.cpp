#include "Admins.h"
#include <algorithm>
#include <iostream>
Admins::Admins(){

}
Admins::Admins(vector<string> _admins){
	admins = _admins;
}
Admins::Admins(string predef_ads[]){
	admins = vector<string>(predef_ads, predef_ads + sizeof(predef_ads) / sizeof(predef_ads[0]));	
}

Admins::~Admins(){
	
}

bool Admins::add_admin(string& msg){
	int wspos =	msg.find(' ', 0);	
	if(wspos == 0)
		return false;
	int wspos2 = msg.find(' ' ,wspos+1);
	
	string name = msg.substr(wspos+1, wspos2 == -1 ? msg.length()-wspos-2: wspos2-wspos);
	
	if(is_admin(name)){
		return false;
	}
	admins.push_back(name);
	
	msg = name + " is now admin!";
	
	cout << msg << endl;
	
	return true;
}
bool Admins::del_admin(string& msg){
	int wspos =	msg.find(' ', 0);	
	if(wspos == 0)
		return false;
	int wspos2 = msg.find(' ' ,wspos+1);
	
	string name = msg.substr(wspos+1, wspos2 == -1 ? msg.length()-wspos-2: wspos2-wspos);
	
	if(!is_admin(name)){
		return false;
	}
	
	admins.erase(remove(admins.begin(), admins.end(), name));
	
	msg = name + " is now no admin anymore!";
	cout << msg << endl;

	return true;
}
void Admins::list_admins(string& msg){
	msg.clear();
	for(auto& admin : admins){
		msg += admin + ", ";
	}
	msg = msg.substr(0, msg.length()-2);
}

bool Admins::is_admin(string name){
	return !admins.empty() && (find(admins.begin(), admins.end(), name) != admins.end());
}

void Admins::load_admins(vector<string> a){
	admins.insert(admins.end(), a.begin(), a.end());
}
void Admins::load_admins(string a[]){
	admins.insert(admins.begin(),a , a + sizeof(a) / sizeof(a[0]));
}

int Admins::count_admins(){
	return admins.size();
}
	
