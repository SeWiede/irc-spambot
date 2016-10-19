#ifndef MACHT_H
#define MATCH_H

#include <vector>
#include <string>

#define MATCH_REACT_MAX 10

/* original struct 
struct Match{
	const char *keyw;
	const char *react[MATCH_REACT_MAX];
	size_t used;
};
i*/


class Match
{
bool operator==(Match& rhs) {
	std::string a= this->get_keyw();
	std::string b= rhs.get_keyw();
	return a.compare(b) == 0;
}
public:
	Match();
    Match(std::string _keyw);
    virtual ~Match();

	const std::string get_keyw();
	
	bool add_react(std::string _react);
	bool del_react(std::string _react);

	const std::string get_react();
	const std::string get_react(uint8_t i);
	int get_reactNum();

	bool is_empty();
private:
	bool empty;
	std::string keyw;
	std::vector<std::string> react_vect;	
};

#endif
