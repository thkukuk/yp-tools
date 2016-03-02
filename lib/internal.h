/* Copyright (C) 2014, 2016 Thorsten Kukuk
   Author: Thorsten Kukuk <kukuk@suse.de>

   This library is free software: you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   in version 2.1 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _INTERNAL_H_
#define _INTERNAL_H_

extern struct ypbind3_binding *__host2ypbind3_binding (const char *__host);
extern struct ypbind3_binding *__ypbind3_binding_dup (struct ypbind3_binding *__src);
extern void __ypbind3_binding_free (struct ypbind3_binding *ypb);

#endif
