/*!
    @file ezn.c

    @brief Source code for ezn application

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

  /*
    Headers local to project
  */

#include "mkdir_p.h"
#include "file_info.h"
#include "which.h"

  /*
    Defines type of markers stored in archive
  */

typedef enum
{
  marker_none,
  marker_data,
  marker_global,
  marker_header,
  marker_end
} marker_type;

  /*
    Currently handled file types
  */

typedef enum
{
  file_none,
  file_regular,
  file_directory
} file_type;

  /*
    GLOBAL section in archive
  */

typedef struct global_s
{
  char *exec;
  int cleanup;
} global_s;

  /*
    HEADER section in archive
  */

typedef struct header_s
{
  char *name;
  file_type type;
  mode_t mode;
  int length;
  int _file_pos;
} header_s;

  /*
    Generic section, for section list
  */

typedef struct section_s
{
  marker_type type;
  union
  {
    global_s *global;
    header_s *header;
  };
} section_s;

  /*
    Section list
  */

typedef struct sections_s
{
  int n;
  section_s **sections;
} sections_s;

  /*
    File information, for file list
  */

typedef struct file_s
{
  file_type type;
  char *name;
  mode_t mode;
  int length;
} file_s;

  /*
    File list
  */

typedef struct files_s
{
  int n;
  file_s **files;
} files_s;

  /*
    Function declarations
  */

void usage(void);

marker_type read_marker(FILE *f);

global_s *global_new(void);
void global_free(global_s *g);
global_s *global_copy(global_s *g);
global_s *global_read(FILE *f);
void global_contents(FILE *f, global_s *global);
void global_dump(FILE *f, global_s *global);

header_s *header_new(void);
void header_free(header_s *h);
header_s *header_copy(header_s *h);
header_s *header_read(FILE *f);
void header_contents(FILE *f, header_s *header);
void header_dump(FILE *f, header_s *header);

section_s *section_new(void);
void section_free(section_s *s);
void section_add_global(section_s *s, global_s *g);
void section_add_header(section_s *s, header_s *h);
void section_contents(FILE *f, section_s *s);
void section_dump(FILE *f, section_s *s);

sections_s *sections_new(void);
void sections_free(sections_s *ss);
void sections_add_section(sections_s *ss, section_s *s);
sections_s *sections_list(char *filename);
void sections_contents(FILE *f, sections_s *ss);
void sections_dump(FILE *f, sections_s *ss);

file_s *file_new(void);
void file_free(file_s *f);
void file_set_name(file_s *f, char *name);
void file_set_type(file_s *f, file_type type);
void file_set_mode(file_s *f, mode_t mode);
void file_set_length(file_s *f, int length);
char *file_get_name(file_s *f);
file_type file_get_type(file_s *f);
mode_t file_get_mode(file_s *f);
int file_get_length(file_s *f);
void file_fill(file_s *f, char *name);
void file_dump(FILE *of, file_s *f);

files_s *files_new(void);
void files_free(files_s *fs);
void files_add_file(files_s *fs, file_s *f);
file_s *files_find_file(files_s *fs, char *name);
int files_expand_directory(files_s *fs, char *dirname);
void files_dump(FILE *of, files_s *fs);

int install(char *filename);

int extract(FILE *f, sections_s *ss);
int run(sections_s *ss);
int cleanup(sections_s *ss);

int build(int argc, char **argv);

void strip(char *s);
void strip_slash(char *s);
char *file_type_to_string(file_type type);
file_type file_string_to_type(char *s);

int create_installer(char *me, char *installer_name, global_s *global, files_s *fs);
void emit_data(FILE *f);
void emit_global(FILE *f, global_s *global);
void emit_header(FILE *f, header_s *header);
void emit_file(FILE *f, header_s *header);

  /*!

     @brief Main for ezn application
    
     This application is intended to provide a simple and easy way to create
     self extracting installers.  The name "ezn" is pronounced "easy in", which
     is short for "easy installer".   Currently, this application and the
     installers it creates are very minimalistic.  ezn will create an archive
     containing everything needed to install an application.  A command can
     be specified during the archive creation that will be executed once the
     installer has extracted its contents.  This command could be a member of
     the archive itself, or a system command.  Command line options are allowed
     in the defined command.
    
     @param argc     integer count of command line arguments
     @param argv     array of command line arguments
    
     @retval  0 success
              1 failure

  */

int main(int argc, char **argv)
{
  int r;

  if (argc < 2)
    r = install(argv[0]);
  else
    r = build(argc, argv);

  return r;
}

void usage(void)
{
  fprintf(stderr, "Please note that all references to 'install.exe' below can be\n"
                  "the name of any user created EZN installer.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "install.exe\n");
  fprintf(stderr, "  - extract the contents of installer and execute the\n"
                  "    user defined command in the installer.\n");
  fprintf(stderr, "install.exe -h\n");
  fprintf(stderr, "  - display this help screen\n");
  fprintf(stderr, "install.exe -x\n");
  fprintf(stderr, "  - extract the contents of installer.\n");
  fprintf(stderr, "install.exe -l\n");
  fprintf(stderr, "  - list the contents of the installer.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "ezn.exe -h\n");
  fprintf(stderr, "  - display this help screen\n");
  fprintf(stderr, "ezn.exe [-c] [-o <installer name>] [-e <command name>] <file list>\n");
  fprintf(stderr, "  where:\n");
  fprintf(stderr, "    <file list> is a list of all files and directories to include\n"
                  "                in the installer.\n");
  fprintf(stderr, "    <installer name> is the name given to the installer that is\n"
                  "                     created by EZN.\n"
                  "                     Default: install.exe\n");
  fprintf(stderr, "    <command name> is the name of one of the included files to\n"
                  "                   execute after the files are extracted.\n"
                  "                   Default: NONE (will extract only)\n");
  fprintf(stderr, "    -c - sets the installer to clean up all extracted files after\n"
                  "         the command (-e) is executed.  The clean up will not\n"
                  "         occur if the command fails.\n");

  return;
}

  // These are required so that no misleading markers are present in the
  // compiled EZN executable

char *_ezn_data = "DATA";
char *_ezn_glob = "GLOB";
char *_ezn_head = "HEAD";
char *_ezn_end = "END ";

marker_type read_marker(FILE *f)
{
  char b[17];
  int first_time = 1;
  static char ezn_data[17];
  static char ezn_glob[17];
  static char ezn_head[17];
  static char ezn_end[17];

    // Build actual marker strings used in installer build operations.
    // This must be done so that no misleading markers appear in the
    // compiled installer executable.

  if (first_time)
  {
    sprintf(ezn_data, "*** EZN %s ***", _ezn_data);
    sprintf(ezn_glob, "*** EZN %s ***", _ezn_glob);
    sprintf(ezn_head, "*** EZN %s ***", _ezn_head);
    sprintf(ezn_end, "*** EZN %s ***", _ezn_end);
    first_time = 0;
  }

  fread(b, 16, 1, f);

  if (!strncmp(b, ezn_data, 16))
    return marker_data;
  else if (!strncmp(b, ezn_glob, 16))
    return marker_global;
  else if (!strncmp(b, ezn_head, 16))
    return marker_header;
  else if (!strncmp(b, ezn_end, 16))
    return marker_end;

  return marker_none;
}

global_s *global_new(void)
{
  global_s *ng = NULL;

  ng = (global_s *)malloc(sizeof(global_s));
  if (ng)
    memset(ng, 0, sizeof(global_s));

  return ng;
}

void global_free(global_s *g)
{
  if (!g) return;

  if (g->exec) free(g->exec);
  free(g);

  return;
}

global_s *global_copy(global_s *g)
{
  global_s *ng;

  if (!g) return NULL;

  ng = global_new();
  if (!ng) return NULL;

  if (g->exec) ng->exec = strdup(g->exec);

  return ng;
}

global_s *global_read(FILE *f)
{
  global_s *ng = NULL;
  char b[4096];
  int loc = 0;
  marker_type mt;

  ng = global_new();
  if (!ng)
    return NULL;

  loc = ftell(f);

  while ((mt = read_marker(f)) != marker_end)
  {
    fseek(f, loc, SEEK_SET);
    if (fgets(b, 4096, f))
    {
      strip(b);
      if (strlen(b))
      {
        if (!strncmp(b, "exec ", 5))
          ng->exec = strdup(&b[5]);
        else if (!strncmp(b, "cleanup", 7))
          ng->cleanup = 1;
      }
    }
    loc = ftell(f);
  }

  return ng;
}

void global_dump(FILE *f, global_s *global)
{
  if (!global) return;

  if (!f) f = stdout;

  fprintf(f, "GLOBAL\n"); fflush(stderr);
  fprintf(f, "  exec='%s'\n", global->exec ? global->exec : "[NONE]"); fflush(stderr);
  fprintf(f, "  cleanup=%d\n", global->cleanup); fflush(stderr);

  return;
}

void global_contents(FILE *f, global_s *global)
{
  if (!global) return;

  if (!f) f = stdout;

  fprintf(f, "COMMAND: %s\n", global->exec ? global->exec : "[NONE]"); fflush(stderr);
  fprintf(f, "CLEANUP: %d\n", global->cleanup); fflush(stderr);

  return;
}

header_s *header_new(void)
{
  header_s *nh = NULL;

  nh = (header_s *)malloc(sizeof(header_s));
  if (nh)
    memset(nh, 0, sizeof(header_s));

  return nh;
}

void header_free(header_s *h)
{
  if (!h) return;

  if (h->name) free(h->name);
  free(h);

  return;
}

header_s *header_copy(header_s *h)
{
  header_s *nh;

  if (!h) return NULL;

  nh = header_new();

  memcpy(nh, h, sizeof(header_s));

  if (h->name) nh->name = strdup(h->name);
  else nh->name = NULL;

  return nh;
}

header_s *header_read(FILE *f)
{
  header_s *nh = NULL;
  char b[4096];
  int loc = 0;
  marker_type mt;

  nh = header_new();
  if (!nh)
    return NULL;

  loc = ftell(f);

  while (((mt = read_marker(f)) != marker_end) && !feof(f))
  {
    fseek(f, loc, SEEK_SET);
    if (fgets(b, 4096, f))
    {
      strip(b);
      if (strlen(b))
      {
        if (!strncmp(b, "name ", 5))
          nh->name = strdup(&b[5]);
        else if (!strncmp(b, "type ", 5))
          nh->type = file_string_to_type(&b[5]);
        else if (!strncmp(b, "mode ", 5))
          nh->mode = strtol(&b[5], NULL, 8);
        else if (!strncmp(b, "length ", 7))
          nh->length = atoi(&b[7]);
      }
    }
    loc = ftell(f);
  }

  loc = ftell(f);

  if (mt == marker_end)
    nh->_file_pos = ftell(f) + 1;

  return nh;
}

void header_dump(FILE *f, header_s *header)
{
  if (!header) return;

  if (!f) f = stdout;

  fprintf(f, "HEADER\n"); fflush(stderr);
  fprintf(f, "  name='%s'\n", header->name); fflush(stderr);
  fprintf(f, "  type=%s\n", file_type_to_string(header->type)); fflush(stderr);
  fprintf(f, "  mode=%0o\n", header->mode); fflush(stderr);
  fprintf(f, "  length=%d\n", header->length); fflush(stderr);
  fprintf(f, "  _file_pos=%d\n", header->_file_pos); fflush(stderr);

  return;
}

void header_contents(FILE *f, header_s *header)
{
  if (!header) return;

  if (!f) f = stdout;

  fprintf(f, "%s\n", header->name); fflush(stderr);

  return;
}

section_s *section_new(void)
{
  section_s *ns;

  ns = (section_s *)malloc(sizeof(section_s));
  if (!ns) return NULL;

  memset(ns, 0, sizeof(section_s));

  return ns;
}

void section_free(section_s *s)
{
  if (!s) return;

  switch (s->type)
  {
    case marker_global:
      global_free(s->global);
      break;

    case marker_header:
      header_free(s->header);
      break;

    default:
      break;
  }
}

void section_add_global(section_s *s, global_s *g)
{
  if (!s || !g) return;

  s->type = marker_global;
  s->global = g;

  return;
}

void section_add_header(section_s *s, header_s *h)
{
  if (!s || !h) return;

  s->type = marker_header;
  s->header = h;

  return;
}

void section_dump(FILE *f, section_s *s)
{
  if (!s) return;

  if (!f) f = stdout;

  switch (s->type)
  {
    case marker_global:
      global_dump(f, s->global);
      break;

    case marker_header:
      header_dump(f, s->header);
      break;

    default:
      fprintf(f, "Unknown SECTION\n"); fflush(stderr);
      break;
  }

  return;
}

void section_contents(FILE *f, section_s *s)
{
  if (!s) return;

  if (!f) f = stdout;

  switch (s->type)
  {
    case marker_global:
      global_contents(f, s->global);
      break;

    case marker_header:
      header_contents(f, s->header);
      break;

    default:
      break;
  }

  return;
}

sections_s *sections_new(void)
{
  sections_s *nss;

  nss = (sections_s *)malloc(sizeof(sections_s));
  if (!nss) return NULL;

  memset(nss, 0, sizeof(sections_s));

  return nss;
}

void sections_free(sections_s *ss)
{
  int i;

  if (!ss) return;

  for (i = 0; i < ss->n; i++)
    section_free(ss->sections[i]);

  free(ss);

  return;
}

void sections_add_section(sections_s *ss, section_s *s)
{
  if (!ss || !s) return;

  if (!ss->n)
    ss->sections = (section_s **)malloc(sizeof(section_s *));
  else
    ss->sections = (section_s **)realloc(ss->sections, sizeof(section_s *) * (ss->n + 1));

  ss->sections[ss->n] = s;

  ++ss->n;

  return;
}

sections_s *sections_list(char *filename)
{
  FILE *f;
  char *me;
  size_t i;
  size_t end;
  marker_type mt;
  global_s *global;
  header_s *header;
  section_s *section;
  sections_s *sections = NULL;

  if (!filename) return NULL;

  me = strdup(filename);

#if defined(_WIN32) || defined(WIN32)
  f = fopen(me, "rb");
#else
  f = fopen(me, "r");
#endif
  if (!f) return NULL;

  fseek(f, 0L, SEEK_END);
  end = ftell(f);
  rewind(f);

  sections = sections_new();

  for (i = 0; i < (end - 16); i++)
  {
    section = NULL;
    fseek(f, i, SEEK_SET);
    mt = read_marker(f);
    switch (mt)
    {
      case marker_data:
        break;

      case marker_global:
        global = global_read(f);
        if (global)
          i = ftell(f);
        section = section_new();
        section_add_global(section, global);
        break;

      case marker_header:
        header = header_read(f);
        if (header)
        {
          i = ftell(f);
          i += header->length;
          --i;
          fseek(f, i - i, SEEK_SET);
        }
        section = section_new();
        section_add_header(section, header);
        break;

      case marker_end:
        break;

      case marker_none:
      default:
        break;
    }

    if (section)
      sections_add_section(sections, section);
  }

  free(me);
  fclose(f);

  return sections;
}

void sections_dump(FILE *f, sections_s *ss)
{
  int i;

  if (!ss) return;

  if (!f) f = stdout;

  fprintf(f, "SECTIONS:\n"); fflush(stderr);

  for (i = 0; i < ss->n; i++)
    section_dump(f, ss->sections[i]);

  return;
}

void sections_contents(FILE *f, sections_s *ss)
{
  int i;

  if (!ss) return;

  if (!f) f = stdout;

  fprintf(f, "\n"); fflush(stderr);

  for (i = 0; i < ss->n; i++)
    section_contents(f, ss->sections[i]);

  return;
}

int extract(FILE *f, sections_s *ss)
{
  int i;
  int j;
  section_s *s;
  header_s *h;
  FILE *of;
  int c;
  char *name_t;
  char *dir;

  if (!f || !ss) return -1;

  for (i = 0; i < ss->n; i++)
  {
    s = ss->sections[i];

    switch (s->type)
    {
      case marker_header:

        h = s->header;
        if (!h) break;

        switch (h->type)
        {
          case file_regular:

            printf("Extracting %s ...\n", h->name); fflush(stdout);

              // Create directory path, if needed
            name_t = strdup(h->name);
            dir = dirname(name_t);
            if (dir)
              mkdir_p(dir, 0777);
            free(name_t);

              // Extract payload contents to named file
#if defined(_WIN32) || defined(WIN32)
            of = fopen(h->name, "wb");
#else
            of = fopen(h->name, "w");
#endif
            if (!of) break;
            fseek(f, h->_file_pos, SEEK_SET);
            for (j = 0; j < h->length; j++)
            {
              c = fgetc(f);
              if (c == EOF) break;
              fputc(c, of);
            }
            fclose(of);

          break;

          case file_directory:

            printf("Creating directory %s ...\n", h->name); fflush(stdout);

            name_t = strdup(h->name);
            mkdir_p(name_t, 0777);
            free(name_t);

            break;

          default:
            break;
        }

          // Change permissions on extracted payload

        chmod(h->name, h->mode);

        break;

      default:
        break;
    }
  }

  return 0;
}

int run(sections_s *ss)
{
  int i;
  section_s *s;
  global_s *g;
  int r = -1;

  if (!ss) return -1;

  for (i = 0; i < ss->n; i++)
  {
    s = ss->sections[i];

    switch (s->type)
    {
      case marker_global:

        g = s->global;
        if (!g) break;

        if (g->exec && strlen(g->exec))
        {
          printf("Executing %s\n", g->exec); fflush(stdout);

          r = system(g->exec);
          if (r)
          {
            fprintf(stderr, "install failed with %d\n", r);
            fflush(stderr);
          }
        }
        else
          r = 0;

        break;

      default:
        break;
    }
  }

  return r;
}

int cleanup(sections_s *ss)
{
  int i;
  section_s *s;
  global_s *g;
  int doclean = 0;
  header_s *h;

  if (!ss) return -1;

  for (i = 0; i < ss->n; i++)
  {
    s = ss->sections[i];

    switch (s->type)
    {
      case marker_global:

        g = s->global;
        if (!g) break;

        if (g->cleanup)
          doclean = 1;

        break;

      default:
        break;
    }
  }

  if (doclean)
  {
    printf("Cleaning up after run ...\n"); fflush(stdout);

    for (i = 0; i < ss->n; i++)
    {
      s = ss->sections[i];

      switch (s->type)
      {
        case marker_header:

          h = s->header;
          if (!h) break;

          switch (h->type)
          {
            case file_regular:
            case file_directory:

              printf("Removing '%s'\n", h->name);
              remove(h->name);

            break;

            default:
              break;
          }

          break;

        default:
          break;
      }
    }
  }

  return 0;
}

int install(char *filename)
{
  FILE *f;
  char *me;
  sections_s *sections = NULL;
  int r = -1;

  if (!filename) return -1;

  sections = sections_list(filename);

  me = strdup(filename);

#if defined(_WIN32) || defined(WIN32)
  f = fopen(me, "rb");
#else
  f = fopen(me, "r");
#endif
  if (!f) return -1;

  free(me);

  r = extract(f, sections);
  if (r)
  {
    fprintf(stderr, "extraction failed\n");
    fflush(stderr);
  }
  else
  {
    r = run(sections);
    if (r)
    {
      fprintf(stderr, "execution failed\n");
      fflush(stderr);
    }
    else
      cleanup(sections);
  }

  fclose(f);

  if (sections) sections_free(sections);

  return r;
}

file_s *file_new(void)
{
  file_s *f;

  f = (file_s *)malloc(sizeof(file_s));
  if (!f) return NULL;

  memset(f, 0, sizeof(file_s));

  return f;
}

void file_free(file_s *f)
{
  if (!f) return;

  if (f->name) free(f->name);

  free(f);

  return;
}

void file_set_name(file_s *f, char *name)
{
  if (!f || !name) return;

  if (f->name) free(f->name);

  f->name = strdup(name);

  return;
}

void file_set_type(file_s *f, file_type type)
{
  if (!f) return;

  f->type = type;
  
  return;
}

void file_set_mode(file_s *f, mode_t mode)
{
  if (!f) return;

  f->mode = mode;

  return;
}

void file_set_length(file_s *f, int length)
{
  if (!f) return;

  if (length < 0) length = 0;

  f->length = length;

  return;
}

char *file_get_name(file_s *f)
{
  if (!f) return NULL;

  return f->name;
}

file_type file_get_type(file_s *f)
{
  if (!f) return file_none;

  return f->type;
}

mode_t file_get_mode(file_s *f)
{
  if (!f) return 0;

  return f->mode;
}

int file_get_length(file_s *f)
{
  if (!f) return 0;

  return f->length;
}

void file_fill(file_s *f, char *name)
{
  mode_t tnm;

  if (!f || !name) return;

  file_set_name(f, name);
  strip_slash(f->name);

  tnm = get_file_type_and_mode(f->name);

  switch (tnm & S_IFMT)
  {
    case S_IFREG:
      file_set_type(f, file_regular);
      break;

    case S_IFDIR:
      file_set_type(f, file_directory);
      break;

    default:
      file_set_type(f, file_none);
      break;
  }

  file_set_mode(f, tnm & ~S_IFMT);

  switch (f->type)
  {
    case file_regular:
      file_set_length(f, get_file_size(f->name));
      break;

    default:
      break;
  }

  return;
}

void file_dump(FILE *of, file_s *f)
{
  if (!f) return;
  if (!of) of = stdout;

  fprintf(of, "FILE:\n"); fflush(stderr);
  fprintf(of, "  type: %d\n", f->type); fflush(stderr);
  fprintf(of, "  name: %s\n", f->name); fflush(stderr);
  fprintf(of, "  mode: %0o\n", f->mode); fflush(stderr);
  fprintf(of, "  length: %d\n", f->length); fflush(stderr);

  return;
}

files_s *files_new(void)
{
  files_s *fs;

  fs = (files_s *)malloc(sizeof(files_s));
  if (!fs) return NULL;

  memset(fs, 0, sizeof(files_s));

  return fs;
}

void files_free(files_s *fs)
{
  int i;

  if (!fs) return;

  for (i = 0; i < fs->n; i++)
    file_free(fs->files[i]);

  free(fs);

  return;
}

void files_add_file(files_s *fs, file_s *f)
{
  if (!fs || !f) return;

  if (!fs->n)
    fs->files = (file_s **)malloc(sizeof(file_s *));
  else
    fs->files = (file_s **)realloc(fs->files, sizeof(file_s *) * (fs->n + 1));

  fs->files[fs->n] = f;

  ++fs->n;

  return;
}

file_s *files_find_file(files_s *fs, char *name)
{
  int i;

  if (!fs || !name) return NULL;

  for (i = 0; i < fs->n; i++)
    if (!strcmp(fs->files[i]->name, name))
      return fs->files[i];

  return NULL;
}

int files_expand_directory(files_s *fs, char *dirname)
{
  DIR *dir;
  struct dirent *ent;
  char path[4096];
  file_s *f;
  struct stat st;

  if (!fs || !dirname) return -1;

  memset(path, 0, 4096);

  dir = opendir(dirname);
  if (!dir) return -1;

  while ((ent = readdir(dir)) != NULL)
  {
    strcpy(path, dirname);
    strcat(path, "/");
    strcat(path, ent->d_name);

    memset(&st, 0, sizeof(struct stat));
    stat(path, &st);

    if (S_ISDIR(st.st_mode))
    {
      if (!strcmp(ent->d_name, ".")) continue;
      if (!strcmp(ent->d_name, "..")) continue;
    }

    f = file_new();
    if (!f) return -1;

    file_fill(f, path);

    files_add_file(fs, f);

    switch (f->type)
    {
      case file_directory:
        files_expand_directory(fs, f->name);
        break;

      default:
        break;
    }

    memset(path, 0, 4096);
  }

  closedir(dir);

  return 0;
}

void files_dump(FILE *of, files_s *fs)
{
  int i;

  if (!fs) return;
  if (!of) of = stdout;

  fprintf(of, "FILES:\n"); fflush(stderr);

  for (i = 0; i < fs->n; i++)
    file_dump(of, fs->files[i]);

  return;
}

int build(int argc, char **argv)
{
  int c;
  char *installer_name = NULL;
  int i;
  int first_time = 1;
  files_s *fs = NULL;
  file_s *f;
  char *my_name = NULL;
  int r;
  global_s *global;
  sections_s *sections;
  FILE *mef;

  opterr = 0;

  my_name = argv[0];

  global = global_new();

  while ((c = getopt(argc, argv, ":o:e:cmxlh")) != EOF)
  {
    switch (c)
    {
      case 'm': // list markers in an installer and exit
        sections = sections_list(my_name);
        sections_dump(stdout, sections);
        return 0;

      case 'l': // list file names and exit
        sections = sections_list(my_name);
        sections_contents(stdout, sections);
        return 0;

      case 'x': // extract files, do not execute anything, and exit
        sections = sections_list(my_name);
#if defined(_WIN32) || defined(WIN32)
        mef = fopen(my_name, "rb");
#else
        mef = fopen(my_name, "r");
#endif
        extract(mef, sections);
        fclose(mef);
        return 0;

      case 'h': // show help
        usage();
        return 0;

      case 'o':
        installer_name = strdup(optarg);
        break;

      case 'e':
        global->exec = strdup(optarg);
        break;

      case 'c':
        global->cleanup = 1;
        break;

      case ':':
        fprintf(stderr, "Missing argument for '%c'\n", optopt); fflush(stderr);
        exit(1);
        break;

      case '?':
        fprintf(stderr, "Unknown option '%c'\n", optopt); fflush(stderr);
        exit(1);
        break;

      default:
        break;
    }
  }

  if (!installer_name) installer_name = strdup("install.exe");

  for (i = optind; i < argc; i++)
  {
    if (first_time)
    {
      first_time = 0;
      fs = files_new();
    }

    f = file_new();
    file_fill(f, argv[i]);

    if (!files_find_file(fs, f->name))
      files_add_file(fs, f);

    switch (f->type)
    {
      case file_directory:
        if (files_expand_directory(fs, f->name))
        {
          fprintf(stderr, "Error expanding directory %s\n", f->name);
          fflush(stderr);
        }
        break;

      default:
        break;
    }
  }

  r = create_installer(my_name, installer_name, global, fs);

  if (installer_name) free(installer_name);
  if (fs) files_free(fs);
  if (global) global_free(global);

  return r;
}

int create_installer(char *me,
                     char *installer_name,
                     global_s *global,
                     files_s *fs)
{
  FILE *mef;
  FILE *inf;
  int c;
  int i;
  header_s *header;

  if (!me || !installer_name || !global || !fs) return -1;

  me = which(me);
  if (!me) return -1;

#if defined(_WIN32) || defined(WIN32)
  mef = fopen(me, "rb");
#else
  mef = fopen(me, "r");
#endif
  if (!mef) return -1;

#if defined(_WIN32) || defined(WIN32)
  inf = fopen(installer_name, "wb");
#else
  inf = fopen(installer_name, "w");
#endif
  if (!inf)
  {
    fclose(mef);
    return -1;
  }

    // Copy ezn executable to installer
  while ((c = fgetc(mef)) != EOF)
    fputc(c, inf);
  fclose(mef);

    // Add EZN marker to installer
  emit_data(inf);

    // Add Global Section to installer
  emit_global(inf, global);

  for (i = 0; i < fs->n; i++)
  {
    header = header_new();
    header->name = strdup(fs->files[i]->name);
    header->type = fs->files[i]->type;
    header->mode = fs->files[i]->mode;
    header->length = fs->files[i]->length;

    emit_header(inf, header);
    emit_file(inf, header);

    header_free(header);
  }

  fclose(inf);

  chmod(installer_name, 0755);

  return 0;
}

void emit_data(FILE *f)
{
  if (!f) return;

    // The strange formatting is to ensure that no misleading markers
    // are compiled into the installer
  fprintf(f, "%s %s %s %s\n", "***", "EZN", "DATA", "***"); fflush(stderr);

  return;
}

void emit_global(FILE *f, global_s *global)
{
  if (!f || !global) return;

    // The strange formatting is to ensure that no misleading markers
    // are compiled into the installer
  fprintf(f, "%s %s %s %s\n", "***", "EZN", "GLOB", "***"); fflush(stderr);
  if (global->exec)
  {
    fprintf(f, "exec %s\n", global->exec);
    fflush(stderr);
  }
  if (global->cleanup)
  {
    fprintf(f, "cleanup\n");
    fflush(stderr);
  }
  fprintf(f, "%s %s %s %s\n", "***", "EZN", "END ", "***"); fflush(stderr);

  return;
}

void emit_header(FILE *f, header_s *header)
{
  if (!f || !header) return;

    // The strange formatting is to ensure that no misleading markers
    // are compiled into the installer
  fprintf(f, "%s %s %s %s\n", "***", "EZN", "HEAD", "***"); fflush(stderr);
  fprintf(f, "name %s\n", header->name); fflush(stderr);
  fprintf(f, "type %s\n", file_type_to_string(header->type)); fflush(stderr);
  fprintf(f, "mode %0o\n", header->mode); fflush(stderr);
  fprintf(f, "length %d\n", header->length); fflush(stderr);
  fprintf(f, "%s %s %s %s\n", "***", "EZN", "END ", "***"); fflush(stderr);

  return;
}

void emit_file(FILE *f, header_s *header)
{
  FILE *in;
  int c;

  if (!f || !header) return;

  switch (header->type)
  {
    case file_regular:
#if defined(_WIN32) || defined(WIN32)
      in = fopen(header->name, "rb");
#else
      in = fopen(header->name, "r");
#endif
      while ((c = fgetc(in)) != EOF)
        fputc(c, f);
      fclose(in);
      break;

    default:
      break;
  }

  return;
}

char *file_type_to_string(file_type type)
{
  static char ts[32];

  memset(ts, 0, 32);

  switch (type)
  {
    case file_regular:
      sprintf(ts, "REGULAR");
      break;

    case file_directory:
      sprintf(ts, "DIRECTORY");
      break;

    default:
      sprintf(ts, "<unknown>");
      break;
  }

  return ts;
}

file_type file_string_to_type(char *s)
{
  if (!s) return file_none;

  if (!strcmp(s, "REGULAR")) return file_regular;
  if (!strcmp(s, "DIRECTORY")) return file_directory;

  return file_none;
}

void strip(char *s)
{
  char c;
  int i;

  if (!s) return;

  while (((c = s[i = strlen(s)-1]) == '\n') || (c == '\r'))
    s[i] = 0;

  return;
}

void strip_slash(char *s)
{
  int i;

  if (!s) return;

  while ((s[i = strlen(s)-1]) == '/')
    s[i] = 0;

  return;
}

