/*
 * (C) Copyright 2013
 * Stefano Babic, DENX Software Engineering, sbabic@denx.de
 * 	on behalf of ifm electronic GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "generated/autoconf.h"
#include "swupdate.h"
#include "parsers.h"

#ifdef CONFIG_LUAEXTERNAL
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "util.h"
#include "lua_util.h"

#ifndef CONFIG_SETEXTPARSERNAME
#define LUA_PARSER	"lua-tools/extparser.lua"
#else
#define LUA_PARSER	(CONFIG_EXTPARSERNAME)
#endif

static void sw_append_stream(struct img_type *img, const char *key,
	       const char *value)
{
    TRACE("Parsing '%s = %s'", key, value);

	if (!strcmp(key, "type"))
    {
		strncpy(img->type, value, sizeof(img->type));
    }
	else if (!strcmp(key, "filename"))
    {
		strncpy(img->fname, value, sizeof(img->fname));
		img->required = 1;
	}
	else if (!strcmp(key, "name"))
    {
		strncpy(img->id.name, value, sizeof(img->id.name));
	}
	else if (!strcmp(key, "version"))
    {
		strncpy(img->id.version, value, sizeof(img->id.version));
	}
	else if (!strcmp(key, "mtdname") || !strcmp(key, "dest"))
    {
		strncpy(img->path, value, sizeof(img->path));
	}
	else if (!strcmp(key, "filesystem"))
    {
		strncpy(img->filesystem, value, sizeof(img->filesystem));
	}
	else if (!strcmp(key, "volume"))
    {
		strncpy(img->volname, value, sizeof(img->volname));
	}
	else if (!strcmp(key, "device_id"))
    {
		strncpy(img->device, value, sizeof(img->device));
	}
	else if (!strcmp(key, "device"))
    {
		strncpy(img->device, value, sizeof(img->device));
	}
	else if (!strcmp(key, "script"))
    {
		img->is_script = 1;
	}
	else if (!strcmp(key, "path"))
    {
		strncpy(img->path, value, sizeof(img->path));
	}
	else if (!strcmp(key, "sha256"))
    {
		ascii_to_hash(img->sha256, value);
	}
	else if (!strcmp(key, "encrypted"))
    {
		img->is_encrypted = 1;
	}
	else if (!strcmp(key, "compressed"))
    {
		img->compressed = 1;
	}
	else if (!strcmp(key, "installed-directly"))
    {
		img->install_directly = 1;
	}
	else if (!strcmp(key, "install-if-different"))
    {
		img->id.install_if_different = 1;
    }
    else
    {
        ERROR("Unrecognised key %s", key);
    }
}

int parse_external(struct swupdate_cfg *software, const char *filename)
{
	int ret;
	unsigned int nstreams;
	struct img_type *image;
	struct hw_type hardware;

	lua_State *L = luaL_newstate(); /* opens Lua */
	luaL_openlibs(L); /* opens the standard libraries */

	if (luaL_loadfile(L, LUA_PARSER)) {
		ERROR("ERROR loading %s", LUA_PARSER);
		lua_close(L);
		return 1;
	}

	ret = lua_pcall(L, 0, 0, 0);
	if (ret) {
		LUAstackDump(L);
		ERROR("ERROR preparing Parser in LUA %d", ret);
		
		return 1;
	}

	if (-1 == get_hw_revision(&hardware))
	{
	    ERROR("ERROR getting hw revision");
	    return 1;
	}

	lua_getglobal(L, "xmlparser");

	/* passing arguments */
	lua_pushstring(L, filename);
	lua_pushstring(L, hardware.boardname);
	lua_pushstring(L, hardware.revision);

	if (lua_pcall(L, 3, 4, 0)) {
		LUAstackDump(L);
		ERROR("ERROR Calling XML Parser in LUA");
		lua_close(L);
		return 1;
	}

	if (lua_type(L, 1) == LUA_TSTRING)
		strncpy(software->name, lua_tostring(L, 1),
				sizeof(software->name));

	if (lua_type(L, 2) == LUA_TSTRING)
		strncpy(software->version, lua_tostring(L, 2),
				sizeof(software->version));
	nstreams = 0;
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		printf("%s - %s\n",
		lua_typename(L, lua_type(L, -2)),
			lua_typename(L, lua_type(L, -1)));

		if (lua_type(L, -1) == LUA_TTABLE) {
			lua_pushnil(L);
			image = (struct img_type *)calloc(1, sizeof(struct img_type));	
			if (!image) {
				ERROR( "No memory: malloc failed\n");
				return -ENOMEM;
			}
			while (lua_next(L, -2) != 0) {
				sw_append_stream(image, lua_tostring(L, -2),
					       lua_tostring(L, -1));

	       			lua_pop(L, 1);
			}
			if (image->is_script)
				LIST_INSERT_HEAD(&software->scripts, image, next);
			else
				LIST_INSERT_HEAD(&software->images, image, next);
			nstreams++;
		}

	       /* removes 'value'; keeps 'key' for next iteration */
	       lua_pop(L, 1);
	}

	LUAstackDump(L);

	lua_close(L);

	TRACE("Software: %s %s", software->name, software->version);
	LIST_FOREACH(image, &software->images, next) {
		TRACE("\tName: %s Type: %s", image->fname,
				image->type);
	}

	return !(nstreams > 0);
}
#else

int parse_external(struct swupdate_cfg __attribute__ ((__unused__)) *software,
			const char __attribute__ ((__unused__)) *filename)
{
	return -1;
}
#endif
