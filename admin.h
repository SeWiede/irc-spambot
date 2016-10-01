#pragma once


int add_admin(char *msg, const struct MsgInfo *const info);
int del_admin(char *msg, const struct MsgInfo *const info);
int list_admins(char *msg, const struct MsgInfo *const info);
int is_admin(const char *name);
void free_adminlist();
void load_admins();


