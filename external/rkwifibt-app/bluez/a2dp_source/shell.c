/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2017  Intel Corporation. All rights reserved.
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include <stdio.h>
#include "util.h"
#include "shell.h"

static void print_string(const char *str, void *user_data)
{
	pr_info("%s\n", str);
}

void bt_shell_hexdump(void *buf, size_t len)
{
	util_hexdump(' ', buf, len, print_string, NULL);
}

void bt_shell_usage()
{
	return;
}

void bt_shell_prompt_input(const char *label, const char *msg,
			bt_shell_prompt_input_func func, void *user_data)
{
	return;
}

int bt_shell_release_prompt(const char *input)
{
	return 0;
}

void bt_shell_init(int argc, char **argv, const struct bt_shell_opt *opt)
{
	return;
}

int bt_shell_run(void)
{
	return 0;
}

void bt_shell_cleanup(void)
{
}

void bt_shell_quit(int status)
{
	return;
}

void bt_shell_noninteractive_quit(int status)
{
}

bool bt_shell_set_menu(const struct bt_shell_menu *menu)
{
	return true;
}

bool bt_shell_add_submenu(const struct bt_shell_menu *menu)
{
	return true;
}

void bt_shell_set_prompt(const char *string)
{
	return;
}

bool bt_shell_attach(int fd)
{
	return true;
}

bool bt_shell_detach(void)
{
	return true;
}

void bt_shell_set_env(const char *name, void *value)
{
	return;
}

void *bt_shell_get_env(const char *name)
{
	return NULL;
}
