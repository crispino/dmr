/* xmlescape.h - helper routines for escaping XML strings
 *
 * Copyright (C) 2007   Ivo Clarysse
 *
 * This file is part of GMediaRender.
 *
 * GMediaRender is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GMediaRender is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GMediaRender; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 * MA 02110-1301, USA.
 *
 */

#include <string.h>
#include <stdlib.h>

#include "xmlescape.h"
#include "output_gstreamer.h"

#include "debug.h"


static void xmlescape_real(const char *str, char *target, int * length,
                           int attribute)
{
    int tmpLen = 0;
	if (target != NULL) {
		int len = 0;
        debug_printf(MSG_INFO, "in xmlescape_real\n", len);
		for (; *str; str++) {
			if (length != NULL)
			{
			    if (len >= *length)
			    {
			        debug_printf(MSG_ERROR, 
						"pout:%p, pstr:%p, str:%s\n", target, str, str);
					target[len] = '\0';
					break;
			    }
			}
			if (*str == '<') {
				memcpy(target + len, "&lt;", 4);
				len += 4;
			} else if (attribute && (*str == '"')) {
				memcpy(target + len, "%22", 3);
				len += 3;
			} else if (*str == '>') {
				memcpy(target + len, "&gt;", 4);
				len += 4;
			} else if (*str == '&') {
				memcpy(target + len, "&amp;", 5);
				len += 5;
			} else {
				target[len++] = *str;
			}
			tmpLen++;
			//printf("%d:%c ", tmpLen, *str);
			
		}
		debug_printf(MSG_INFO, "xmlescape_real len:%d\n", len);
		target[len] = '\0';

		if (length != NULL)
			*length = len;
	} else if (length != NULL) {
		int len = 0;

		for (; *str; str++) {
			if (*str == '<') {
				len += 4;
			} else if (attribute && (*str == '"')) {
				len += 3;
			} else if (*str == '>') {
				len += 4;
			} else if (*str == '&') {
				len += 5;
			} else {
				len++;
			}
		}

		*length = len;
	}
}

char *xmlescape(const char *str, int attribute)
{
	int len;
	char *out = NULL;
	if (NULL == str)
	{
	    debug_printf(MSG_ERROR, "xmlescape str null\n");
	    return NULL;
	}
	debug_printf(MSG_INFO, "str1:%s, attribute:%d,strlen:%d\n",
		  str, attribute, strlen(str));

	xmlescape_real(str, NULL, &len, attribute);
	debug_printf(MSG_INFO, "start malloc, len:%d\n", len);
	out = malloc(len + 1);
	if (NULL == out)
	{
	    debug_printf(MSG_ERROR, "malloc fail\n");
	    return NULL;
	}
	debug_printf(MSG_INFO, "end malloc:%p\n", out);
	debug_printf(MSG_INFO, 
						"pout:%p, pstr:%p, str:%s\n", out, str, str);
	xmlescape_real(str, out, &len, attribute);
	debug_printf(MSG_INFO, "xmlescape_real end\n");
	return out;
}
