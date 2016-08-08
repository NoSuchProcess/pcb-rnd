/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2004, 2006 Dan McMahill
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include "compat_dl.h"
#include "global.h"

#if !defined(HAVE_DLFCN_H) && defined(WIN32)
#include <windows.h>

void *dlopen(const char *f, int ATTRIBUTE_UNUSED flag)
{
	return LoadLibrary(f);
}

void dlclose(void *h)
{
	FreeLibrary((HINSTANCE) h);
}

char *dlerror()
{
	static LPVOID lpMsgBuf = NULL;
	DWORD dw;

	/* free the error message buffer */
	if (lpMsgBuf)
		LocalFree(lpMsgBuf);

	/* get the error code */
	dw = GetLastError();

	/* get the corresponding error message */
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
								FORMAT_MESSAGE_FROM_SYSTEM |
								FORMAT_MESSAGE_IGNORE_INSERTS,
								NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) & lpMsgBuf, 0, NULL);

	return (char *) lpMsgBuf;
}

void *dlsym(void *handle, const char *symbol)
{
	return (void *) GetProcAddress((HMODULE) handle, symbol);
}


#endif
