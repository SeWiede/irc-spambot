
#include "bot.h"
#include "admin.h"

#ifndef NO_ADMIN

#include <assert.h>
#include <stdlib.h>
#include <string.h>


static const char *predefined_admins[] = {"chtis", "wiede5335"};
static char **admin_str = NULL;
static int admin_num = 0;

static int find_admin_str_pos(const char *name)
{
	int i;
	for(i = 0; i < admin_num; i++){
		if(strcmp(admin_str[i], name) == 0){
			return i;
		}
	}
	return -1;
}

int add_admin(char *msg, const struct MsgInfo *const info)
{
	char *name = skip_whitespace(msg);
	cut_token(name);

	if(name[0] == '\0'){
		return 0;
	}
	if(is_admin(name)){
		privmsg(info, "%s already has command permission\r\n", name);
		return 1;
	}
	admin_str = (char **)realloc(admin_str, (++admin_num)* sizeof(char*));
	if(admin_str == NULL){
		exit(1);
	}
	admin_str[admin_num-1] = strdup(name);
	if(admin_str[admin_num-1] == NULL){
		exit(1);
	}

	privmsg(info, "%s is now an admin!\r\n", name);

	struct MsgInfo tmpinfo = *info;
	tmpinfo.user = name;

	privmsg(&tmpinfo, "you're now permitted to command me!\r\n");

	return 1;
}

int list_admins(char *msg, const struct MsgInfo *const info)
{
	struct MsgInfo tmpinfo = *info;
	char admin_cat_str[GLOBAL_BUFSIZE];

	tmpinfo.user = NULL; 
	memset(&admin_cat_str[0], 0, sizeof admin_cat_str);
	for(int i = 0; i < admin_num; i++){
		strcat(admin_cat_str, admin_str[i]);
		strcat(admin_cat_str, "  ");	
	}	
	privmsg(&tmpinfo, "%s\r\n", admin_cat_str);
	return 1;
}

int del_admin(char *msg, const struct MsgInfo *const info)
{
	char *name = skip_whitespace(msg);
	cut_token(name);

	if(name[0] == '\0'){
		return 0;
	}
	if(!is_admin(name)){
		privmsg(info, "I dont take commands from that guy anyway!\r\n");
		return 1;
	}
	if(admin_num == 1){
		/* name is the last admin */
		admin_num =0;
		free(admin_str[0]);
		free(admin_str);
		admin_str = NULL;
		return 1;
	}

	int pos = find_admin_str_pos(name);
	assert(pos >= 0); /* we checked that some lines above with is_admin */


	memmove(&admin_str[pos], &admin_str[pos+1], sizeof(char*)*(admin_num - pos - 1));
	admin_str = (char**) realloc(admin_str, (--admin_num) * sizeof(char*));
	if(admin_str == NULL){
		exit(1);
	}
	return 1;
}

int count_admins()
{
	return admin_num;
}

int is_admin(const char *name){
	return find_admin_str_pos(name) >= 0;
}

void free_adminlist()
{
	if(admin_str != NULL){
		for(int i = 0; i < admin_num; i++){
			free(admin_str[i]);
		}
		free(admin_str);
	}
}

void load_admins(){
	admin_str = (char **)malloc(COUNT_OF(predefined_admins)* sizeof(char *));
	if(admin_str == NULL){
		exit(1);
	}
	for(int i = 0; i < COUNT_OF(predefined_admins); i++){
		admin_str[i] = strdup(predefined_admins[i]);
		if(admin_str[i] == NULL){
			exit(1);
		}
		admin_num++;
	}
}

#else /* ifdef NO_ADMIN */

int add_admin(char *msg, const struct MsgInfo *const info)
{
	return 1;
}
int del_admin(char *msg, const struct MsgInfo *const info)
{
	return 1;
}
int list_admins(char *msg, const struct MsgInfo *const info)
{
	return 1;
}
int is_admin(const char *name)
{
	return 1;
}
void free_adminlist()
{
}
void load_admins()
{
}

#endif
