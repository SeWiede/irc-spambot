#pragma once


int admin_add(char *msg, const struct MsgInfo *const info);
int admin_del(char *msg, const struct MsgInfo *const info);
int admin_list(char *msg, const struct MsgInfo *const info);
int is_admin(const char *name);
void admin_init(void);
void admin_uninit(void);


