/*!
    @file which.c

    @brief Source code for which command like function

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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <limits.h>

#include "file_info.h"
#include "which.h"

#if defined(_WIN32) || defined(WIN32)
  #define SEP_CHAR '\\'
  #define PATH_CHAR ';'
#else
  #define SEP_CHAR '/'
  #define PATH_CHAR ':'
#endif

int is_me(char *path);
char *match(char *exec_name);

char *which(char *exec_name)
{
  char *target = NULL;
  char *path = NULL;
  int len;
#if defined(_WIN32) || defined(WIN32)
  char cwd[PATH_MAX];
  int tn_len;
#endif

  if (!exec_name) return NULL;

  target = strdup(exec_name);
  len = strlen(target);
  target = (char *)realloc(target, len + 5);
#if defined(_WIN32) || defined(WIN32)
  if (len > 3)
  {
    if (!strcasecmp(&target[len-4], ".exe"))
      target[len-4] = 0;
  }
  strcat(target, ".EXE");
#endif

  if (target[0] == '.')
    path = strdup(target);
  else if (strrchr(target, SEP_CHAR))
    path = strdup(target);

  if (path) return path;

#if defined(_WIN32) || defined(WIN32)
  getcwd(cwd, PATH_MAX);
  if (!strlen(cwd)) return NULL;
  tn_len = strlen(cwd) + 1 + strlen(target) + 1;
  path = (char *)malloc(tn_len);
  memset(path, 0, tn_len);
  sprintf(path, "%s%c%s", cwd, SEP_CHAR, target);

  if (!is_me(path))
  {
    free(path);
    path = NULL;
  }
#endif

  if (path) return path;

  path = match(target);

  return path;
}

int is_me(char *path)
{
  mode_t mode;

  if (!path) return 0;

  mode = get_file_type_and_mode(path);

  return mode;
}

char *match(char *exec_name)
{
  char *env_path = NULL;
  char *beg, *end;
  static char path[PATH_MAX];
  int last;

  memset(path, 0, PATH_MAX);

  env_path = getenv("PATH");
  if (!env_path) return NULL;

  beg = end = env_path;
  while (*end)
  {
    if (*end == PATH_CHAR)
    {
      while (*beg == PATH_CHAR) ++beg;
      if (beg >= end)
      {
        end = beg + 1;
        continue;
      }
      memset(path, 0, PATH_MAX);
      strncpy(path, beg, end-beg);
      ++end;
      beg = end;
      last = strlen(path);
      if (path[last - 1] != SEP_CHAR)
        path[last] = SEP_CHAR;
      strcat(path, exec_name);
      if (is_me(path)) return path;
    }
    ++end;
  }

  if (beg != end)
  {
    while (*beg == PATH_CHAR) ++beg;
    if (beg < end)
    {
      memset(path, 0, PATH_MAX);
      strncpy(path, beg, end-beg);
      last = strlen(path);
      if (path[last - 1] != SEP_CHAR)
        path[last] = SEP_CHAR;
      strcat(path, exec_name);
      if (is_me(path)) return path;
    }
  }

  return NULL;
}

