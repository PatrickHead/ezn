/*!
    @file file_info.c

    @brief Source code for file information functions

    @timestamp Wed, 15 Dec 2021 15:34:50 +0000

    @author Patrick Head  mailto:patrickhead@gmail.com

    @copyright Copyright (C) 2021  Patrick Head

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

  /*
    Required system headers
  */

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

  /*
    Headers local to project
  */

#include "file_info.h"

mode_t get_file_type_and_mode(char *name)
{
  struct stat sb;

  if (!name) return 0;

  memset(&sb, 0, sizeof(struct stat));

  if (stat(name, &sb)) return 0;

  return sb.st_mode;
}

int get_file_size(char *name)
{
  struct stat sb;

  if (!name) return 0;

  memset(&sb, 0, sizeof(struct stat));

  if (stat(name, &sb)) return 0;

  return sb.st_size;
}

