/*!
    @file mkdir_p.c

    @brief Source code for "mkdir -p" like function.

    @timestamp Wed, 15 Dec 2021 15:34:50 +0000

    @author Patrick Head  mailto:patrickhead@gmail.com

    @copyright Copyright (C) 2013,2015,2021  Patrick Head

    @license
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.@n
    @n
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.@n
    @n
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

  /*!

     @brief Create a directory hierarchy in same spirit as "mkdir -p" command.
    
     This utility function will create a directory hierarchy much like the 
     POSIX "mkdir -p" command, filling in any missing directory elements
     necessary to complete the directory creation.
    
     @param pathname string containing path of new directory(s) to create
     @param mode     mode_t permissions for newly created directories
    
     @retval  0 success
             -1 failure

  */

int mkdir_p(const char *pathname, mode_t mode)
{
  char *pn, *base_pn;
  char *dir;
  struct stat st;
  int r;
#if !defined(_WIN32) && !defined(WIN32)
  extern int errno;
#endif

  if (!pathname)
    return -1;

  r = stat(pathname, &st);
  if (!r)
  {
    if (S_ISDIR(st.st_mode))
      return 0;
    else
      return -1;
  }

  if (errno != ENOENT)
    return -1;

  pn = strdup(pathname);
  dir = dirname(pn);

  r = mkdir_p(dir, mode);
  if (r)
  {
    free(pn);
    return -1;
  }

  base_pn = strdup(pathname);

#if defined(_WIN32) || defined(WIN32)
  r = mkdir(pathname);
#else
  r = mkdir(pathname, mode);
#endif

  if (pn)
    free(pn);
  if (base_pn)
    free(base_pn);

  return r;
}

