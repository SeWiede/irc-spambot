#include "Match.h"
#include <stdlib.h>
#include <algorithm>

Match::Match()
{
	empty = true;
}

Match::Match(std::string _keyw)
{
	empty = false;
	keyw = _keyw;
}

Match::~Match()
{

}

const std::string Match::get_keyw()
{
	return keyw;
}

bool Match::add_react(const std::string _react)
{
	if(react_vect.size() >= MATCH_REACT_MAX)
		return false;
	
	react_vect.push_back(_react);
	return true;
}

bool Match::del_react(std::string _react){
	react_vect.erase(remove(react_vect.begin(), react_vect.end(), _react));
	if (react_vect.size() == 0)
		empty = true;
	return true;
}

const std::string Match::get_react()
{
	if(react_vect.size() <= 0)
		return NULL;
	int pos = rand() % react_vect.size();
	return react_vect[pos]; 	
}

const std::string Match::get_react(uint8_t i){
	if( (i < 0) || (i >= react_vect.size()) )
		return NULL;
	return react_vect[i];
}

int Match::get_reactNum(){
	return react_vect.size();
}

bool Match::is_empty(){
	return empty;
}
