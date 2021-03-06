#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "getopt.h"


long lseek ();

char *mktemp ();

extern int sys_nerr;
extern char *sys_errlist[];


/*I don't have flock do I??
struct flock {
    short    l_type;
    short    l_whence;
    off_t    l_start;
    off_t    l_len;
    pid_t    l_pid;
};


int    open  (const char *, int, ...)  ;
int    creat  (const char *, mode_t)  ;
int    fcntl  (int, int, ...)  ;

int    flock  (int, int)  ;
*/



#if !defined (SEEK_SET)
#  define SEEK_SET 0
#  define SEEK_CUR 1
#  define SEEK_END 2
#endif /* !SEEK_SET */




struct lineinfo
{
  char *text;
  union {
    char *text;
    long number;
  } key;
  long keylen;
};


struct keyfield
{
  int startwords;
  int startchars;
  int endwords;
  int endchars;
  char ignore_blanks;
  char fold_case;
  char reverse;
  char numeric;
  char positional;
  char braced;
};


/*Globals--we use a lot */
struct keyfield keyfields[3];
int num_keyfields = 3;
char **infiles;
char **outfiles;
int num_infiles;
char **linearray;
long nlines;
char *tempdir;
char *tempbase;
int tempcount;
int last_deleted_tempcount;
char *text_base;
int keep_tempfiles;
char *program_name;


/*function prototypes, not quite ANSI, K&R functions with prototypes*/
int main (int argc, char *argv[]);
void usuage (void);
void decode_command (int argc, char *argv[]);
char *maketempname (int count);
void flush_tempfiles (int to_count);
char *tempcopy (int idesc);
int compare_full (char **line1, char **line2);
int compare_prepared (struct lineinfo *line1, struct lineinfo *line2);
int compare_general (char *str1, char *str2, long pos1, long pos2, int use_keyfi
char *find_field (struct keyfield *keyfield, char *str, long *lengthptr);
char *find_pos (char *str, int words, int chars, int ignore_blanks);
char *find_braced_pos (char *str, int words, int chars, int ignore_blanks);
char *find_braced_end (char *str);
long find_value (char *start, long length);
void init_char_order (void);
int compare_field (struct keyfield *keyfield, char *start1, long length1,
                     long pos1, char *start2, long length2, long pos2);
void initbuffer (struct linebuffer *linebuffer);
long readline (struct linebuffer *linebuffer, FILE *stream);
void sort_offline (char *infile, int nfiles, long total, char *outfile);
void sort_in_core (char *infile, long total, char *outfile);
char **parsefile (char *filename, char **nextline, char *data, long size);
void indexify (char *line, FILE *ostream);
void init_index (void);
void finish_index (FILE *ostream);
void writelines (char **linearray, int nlines, FILE *ostream);
int merge_files (char **infiles, int nfiles, char *outfile);
int merge_direct (char **infiles, int nfiles, char *outfile);
void fatal (char *s1, char *s2);
void error (char *s1, char *s2);
void perror_with_name (char *name);
void pfatal_with_name (char *name);
char *concat (char *s1, char *s2, char *s3);
void *xmalloc (int nbytes);
void *xrealloc (void *pointer, int nbytes);
void memory_error (char *callers_name, int bytes_wanted);






int
main (int argc, char *argv[])
{
  int i;

  tempcount = 0;
  last_deleted_tempcount = 0;
  program_name = argv[0];



  keyfields[0].braced = 1;
  keyfields[0].fold_case = 1;
  keyfields[0].endwords = -1;
  keyfields[0].endchars = -1;


  keyfields[1].braced = 1;
  keyfields[1].numeric = 1;
  keyfields[1].startwords = 1;
  keyfields[1].endwords = -1;
  keyfields[1].endchars = -1;



  keyfields[2].endwords = -1;
  keyfields[2].endchars = -1;

  decode_command (argc, argv);

  tempbase = mktemp (concat ("txiXXXXXX", "", ""));



  for (i = 0; i < num_infiles; i++)
    {
      int desc;
      long ptr;
      char *outfile;

      desc = open (infiles[i], 0x0000 , 0);
      if (desc < 0)
    pfatal_with_name (infiles[i]);
      lseek (desc, 0L, 2 );
      ptr = lseek (desc, 0L, 1 );

      close (desc);

      outfile = outfiles[i];
      if (!outfile)
    {
      outfile = concat (infiles[i], "s", "");
    }

      if (ptr < 500000 )

    sort_in_core (infiles[i], ptr, outfile);
      else
    sort_offline (infiles[i], ptr, outfile);
    }

  flush_tempfiles (tempcount);
  exit (0 );
}


void
usage (void)
{
  fprintf (stderr, "Usage: %s [-k] infile [-o outfile] ...\n", program_name);
  fprintf (stderr, "\tk -- keep temporary files.\n");
  fprintf (stderr, "\t%s modified for OS/2/ANSI C by Chris Inacio.\n");
  exit (1);
}




void
decode_command (int argc, char *argv[])
{
  int optc;
  char **ip;
  char **op;



  tempdir = getenv ("TMPDIR");

  if (tempdir == 0 )
    tempdir = "/tmp/";
  else
    tempdir = concat (tempdir, "/", "");


  keep_tempfiles = 0;



  infiles = (char **) xmalloc (argc * sizeof (char *));
  outfiles = (char **) xmalloc (argc * sizeof (char *));
  ip = infiles;
  op = outfiles;

  while ((optc = getopt (argc, argv, "-ko:")) != (-1) )
    {
      switch (optc)
    {
    case 1:
      *ip++ = optarg;
      *op++ = 0 ;
      break;

    case 'k':
      keep_tempfiles = 1;
      break;

    case 'o':
      if (op > outfiles)
        *(op - 1) = optarg;
      break;

    default:
      usage ();
    }
    }


  num_infiles = ip - infiles;
  *ip = 0;
  if (num_infiles == 0)
    usage ();
}



char *
maketempname (int count)
{
  char tempsuffix[10];
  sprintf (tempsuffix, "%d", count);
  return concat (tempdir, tempbase, tempsuffix);
}



void
flush_tempfiles (int to_count)
{
  if (keep_tempfiles)
    return;
  while (last_deleted_tempcount < to_count)
    unlink (maketempname (++last_deleted_tempcount));
}






char *
tempcopy (int idesc)
{
  char *outfile = maketempname (++tempcount);
  int odesc;
  char buffer[1024 ];

  odesc = open (outfile, 0x0001  | 0x0200 , 0666);

  if (odesc < 0)
    pfatal_with_name (outfile);

  while (1)
    {
      int nread = read (idesc, buffer, 1024 );
      write (odesc, buffer, nread);
      if (!nread)
    break;
    }

  close (odesc);

  return outfile;
}



int
compare_full (char **line1, char **line2)
{
  int i;





  for (i = 0; i < num_keyfields; i++)
    {
      long length1, length2;
      char *start1 = find_field (&keyfields[i], *line1, &length1);
      char *start2 = find_field (&keyfields[i], *line2, &length2);
      int tem = compare_field (&keyfields[i], start1, length1, *line1 - text_base,
                   start2, length2, *line2 - text_base);
      if (tem)
    {
      if (keyfields[i].reverse)
        return -tem;
      return tem;
    }
    }

  return 0;
}






int
compare_prepared (struct lineinfo *line1, struct lineinfo *line2)
{
  int i;
  int tem;
  char *text1, *text2;


  if (keyfields->positional)
    {
      if (line1->text - text_base > line2->text - text_base)
    tem = 1;
      else
    tem = -1;
    }
  else if (keyfields->numeric)
    tem = line1->key.number - line2->key.number;
  else
    tem = compare_field (keyfields, line1->key.text, line1->keylen, 0,
             line2->key.text, line2->keylen, 0);
  if (tem)
    {
      if (keyfields->reverse)
    return -tem;
      return tem;
    }

  text1 = line1->text;
  text2 = line2->text;





  for (i = 1; i < num_keyfields; i++)
    {
      long length1, length2;
      char *start1 = find_field (&keyfields[i], text1, &length1);
      char *start2 = find_field (&keyfields[i], text2, &length2);
      int tem = compare_field (&keyfields[i], start1, length1, text1 - text_base,
                   start2, length2, text2 - text_base);
      if (tem)
    {
      if (keyfields[i].reverse)
        return -tem;
      return tem;
    }
    }

  return 0;
}






int
compare_general (char *str1, char *str2, long pos1, long pos2, int use_keyfields)
{
  int i;





  for (i = 0; i < use_keyfields; i++)
    {
      long length1, length2;
      char *start1 = find_field (&keyfields[i], str1, &length1);
      char *start2 = find_field (&keyfields[i], str2, &length2);
      int tem = compare_field (&keyfields[i], start1, length1, pos1,
                   start2, length2, pos2);
      if (tem)
    {
      if (keyfields[i].reverse)
        return -tem;
      return tem;
    }
    }

  return 0;
}





char *
find_field (struct keyfield *keyfield, char *str, long *lengthptr)
{
  char *start;
  char *end;
  char *(*fun) ();

  if (keyfield->braced)
    fun = find_braced_pos;
  else
    fun = find_pos;

  start = (*fun) (str, keyfield->startwords, keyfield->startchars,
          keyfield->ignore_blanks);
  if (keyfield->endwords < 0)
    {
      if (keyfield->braced)
    end = find_braced_end (start);
      else
    {
      end = start;
      while (*end && *end != '\n')
        end++;
    }
    }
  else
    {
      end = (*fun) (str, keyfield->endwords, keyfield->endchars, 0);
      if (end - str < start - str)
    end = start;
    }
  *lengthptr = end - start;
  return start;
}






char *
find_pos (char *str, int words, int chars, int ignore_blanks)
{
  int i;
  char *p = str;

  for (i = 0; i < words; i++)
    {
      char c;

      while ((c = *p) == ' ' || c == '\t')
    p++;
      while ((c = *p) && c != '\n' && !(c == ' ' || c == '\t'))
    p++;
      if (!*p || *p == '\n')
    return p;
    }

  while (*p == ' ' || *p == '\t')
    p++;

  for (i = 0; i < chars; i++)
    {
      if (!*p || *p == '\n')
    break;
      p++;
    }
  return p;
}




char *
find_braced_pos (char *str, int words, int chars, int ignore_blanks)
{
  int i;
  int bracelevel;
  char *p = str;
  char c;

  for (i = 0; i < words; i++)
    {
      bracelevel = 1;
      while ((c = *p++) != '{' && c != '\n' && c)
      ;
      if (c != '{')
    return p - 1;
      while (bracelevel)
    {
      c = *p++;
      if (c == '{')
        bracelevel++;
      if (c == '}')
        bracelevel--;
      if (c == 0 || c == '\n')
        return p - 1;
    }
    }

  while ((c = *p++) != '{' && c != '\n' && c)
      ;

  if (c != '{')
    return p - 1;

  if (ignore_blanks)
    while ((c = *p) == ' ' || c == '\t')
      p++;

  for (i = 0; i < chars; i++)
    {
      if (!*p || *p == '\n')
    break;
      p++;
    }
  return p;
}




char *
find_braced_end (char *str)
{
  int bracelevel;
  char *p = str;
  char c;

  bracelevel = 1;
  while (bracelevel)
    {
      c = *p++;
      if (c == '{')
    bracelevel++;
      if (c == '}')
    bracelevel--;
      if (c == 0 || c == '\n')
    return p - 1;
    }
  return p - 1;
}

long
find_value (char *start, long length)
{
  while (length != 0L)
    {
      if (((_ctype_ + 1)[ *start ] & 0x04 ) )
    return atol (start);
      length--;
      start++;
    }
  return 0l;
}



/*yet another global variable*/
int char_order[256];

void
init_char_order (void)
{
  int i;
  for (i = 1; i < 256; i++)
    char_order[i] = i;

  for (i = '0'; i <= '9'; i++)
    char_order[i] += 512;

  for (i = 'a'; i <= 'z'; i++)
    {
      char_order[i] = 512 + i;
      char_order[i + 'A' - 'a'] = 512 + i;
    }
}





int
compare_field (struct keyfield *keyfield, char *start1, long length1,
                long pos1, char *start2, long length2, long pos2)
{
  if (keyfields->positional)
    {
      if (pos1 > pos2)
    return 1;
      else
    return -1;
    }
  if (keyfield->numeric)
    {
      long value = find_value (start1, length1) - find_value (start2, length2);
      if (value > 0)
    return 1;
      if (value < 0)
    return -1;
      return 0;
    }
  else
    {
      char *p1 = start1;
      char *p2 = start2;
      char *e1 = start1 + length1;
      char *e2 = start2 + length2;

      while (1)
    {
      int c1, c2;

      if (p1 == e1)
        c1 = 0;
      else
        c1 = *p1++;
      if (p2 == e2)
        c2 = 0;
      else
        c2 = *p2++;

      if (char_order[c1] != char_order[c2])
        return char_order[c1] - char_order[c2];
      if (!c1)
        break;
    }


      p1 = start1;
      p2 = start2;
      while (1)
    {
      int c1, c2;

      if (p1 == e1)
        c1 = 0;
      else
        c1 = *p1++;
      if (p2 == e2)
        c2 = 0;
      else
        c2 = *p2++;

      if (c1 != c2)

        return c2 - c1;
      if (!c1)
        break;
    }

      return 0;
    }
}




/*Weird place to put a structure definition */
struct linebuffer
{
  long size;
  char *buffer;
};



void
initbuffer (struct linebuffer *linebuffer)
{
  linebuffer->size = 200;
  linebuffer->buffer = (char *) xmalloc (200);
}




long
readline (struct linebuffer *linebuffer, FILE *stream)
{
  char *buffer = linebuffer->buffer;
  char *p = linebuffer->buffer;
  char *end = p + linebuffer->size;

  while (1)
    {
      int c = (--(  stream  )->_r < 0 ? __srget(  stream  ) : (int)(*(  stream  )->_p++))  ;
      if (p == end)
    {
      buffer = (char *) xrealloc (buffer, linebuffer->size *= 2);
      p += buffer - linebuffer->buffer;
      end += buffer - linebuffer->buffer;
      linebuffer->buffer = buffer;
    }
      if (c < 0 || c == '\n')
    {
      *p = 0;
      break;
    }
      *p++ = c;
    }

  return p - buffer;
}



void
sort_offline (char *infile, int nfiles, long total, char *outfile)
{

  int ntemps = 2 * (total + 500000  - 1) / 500000 ;
  char **tempfiles = (char **) xmalloc (ntemps * sizeof (char *));
  FILE *istream = fopen (infile, "r");
  int i;
  struct linebuffer lb;
  long linelength;
  int failure = 0;

  initbuffer (&lb);



  linelength = readline (&lb, istream);

  if (lb.buffer[0] != '\\' && lb.buffer[0] != '@')
    {
      error ("%s: not a texinfo index file", infile);
      return;
    }




  for (i = 0; i < ntemps; i++)
    {
      char *outname = maketempname (++tempcount);
      FILE *ostream = fopen (outname, "w");
      long tempsize = 0;

      if (!ostream)
    pfatal_with_name (outname);
      tempfiles[i] = outname;




      while (tempsize + linelength + 1 <= 500000 )
    {
      tempsize += linelength + 1;
      fputs (lb.buffer, ostream);
      __sputc( '\n' ,   ostream ) ;



      linelength = readline (&lb, istream);
      if (!linelength && (((  istream  )->_flags & 0x0020 ) != 0)  )
        break;

      if (lb.buffer[0] != '\\' && lb.buffer[0] != '@')
        {
          error ("%s: not a texinfo index file", infile);
          failure = 1;
          goto fail;
        }
    }
      fclose (ostream);
      if ((((  istream  )->_flags & 0x0020 ) != 0)  )
    break;
    }

  free (lb.buffer);

fail:


  ntemps = i;





  for (i = 0; i < ntemps; i++)
    {
      char *newtemp = maketempname (++tempcount);
      sort_in_core (&tempfiles[i], 500000 , newtemp);
      if (!keep_tempfiles)
    unlink (tempfiles[i]);
      tempfiles[i] = newtemp;
    }

  if (failure)
    return;



  merge_files (tempfiles, ntemps, outfile);
}





void
sort_in_core (infile, total, outfile)
{
  char **nextline;
  char *data = (char *) xmalloc (total + 1);
  char *file_data;
  long file_size;
  int i;
  FILE *ostream = (&__sF[1]) ;
  struct lineinfo *lineinfo;



  int desc = open (infile, 0x0000 , 0);

  if (desc < 0)
    fatal ("failure reopening %s", infile);
  for (file_size = 0;;)
    {
      i = read (desc, data + file_size, total - file_size);
      if (i <= 0)
    break;
      file_size += i;
    }
  file_data = data;
  data[file_size] = 0;

  close (desc);

  if (file_size > 0 && data[0] != '\\' && data[0] != '@')
    {
      error ("%s: not a texinfo index file", infile);
      return;
    }

  init_char_order ();



  text_base = data;




  nlines = total / 50;
  if (!nlines)
    nlines = 2;
  linearray = (char **) xmalloc (nlines * sizeof (char *));




  nextline = linearray;



  nextline = parsefile (infile, nextline, file_data, file_size);
  if (nextline == 0)
    {
      error ("%s: not a texinfo index file", infile);
      return;
    }







  lineinfo = (struct lineinfo *) malloc ((nextline - linearray) * sizeof (struct lineinfo));

  if (lineinfo)
    {
      struct lineinfo *lp;
      char **p;

      for (lp = lineinfo, p = linearray; p != nextline; lp++, p++)
    {
      lp->text = *p;
      lp->key.text = find_field (keyfields, *p, &lp->keylen);
      if (keyfields->numeric)
        lp->key.number = find_value (lp->key.text, lp->keylen);
    }

      qsort (lineinfo, nextline - linearray, sizeof (struct lineinfo), compare_prepared);

      for (lp = lineinfo, p = linearray; p != nextline; lp++, p++)
    *p = lp->text;

      free (lineinfo);
    }
  else
    qsort (linearray, nextline - linearray, sizeof (char *), compare_full);



  if (outfile)
    {
      ostream = fopen (outfile, "w");
      if (!ostream)
    pfatal_with_name (outfile);
    }

  writelines (linearray, nextline - linearray, ostream);
  if (outfile)
    fclose (ostream);

  free (linearray);
  free (data);
}


char **
parsefile (char *filename, char **nextline, char *data, long size)
{
  char *p, *end;
  char **line = nextline;

  p = data;
  end = p + size;
  *end = 0;

  while (p != end)
    {
      if (p[0] != '\\' && p[0] != '@')
    return 0;

      *line = p;
      while (*p && *p != '\n')
    p++;
      if (p != end)
    p++;

      line++;
      if (line == linearray + nlines)
    {
      char **old = linearray;
      linearray = (char **) xrealloc (linearray, sizeof (char *) * (nlines *= 4));
      line += linearray - old;
    }
    }

  return line;
}



/*Some more global variables*/
char *lastprimary;
int lastprimarylength;
char *lastsecondary;
int lastsecondarylength;
int pending;
char *lastinitial;
int lastinitiallength;
char lastinitial1[2];





void
init_index (void)
{
  pending = 0;
  lastinitial = lastinitial1;
  lastinitial1[0] = 0;
  lastinitial1[1] = 0;
  lastinitiallength = 0;
  lastprimarylength = 100;
  lastprimary = (char *) xmalloc (lastprimarylength + 1);
  memset(( lastprimary ), '\0', (  lastprimarylength + 1 )) ;
  lastsecondarylength = 100;
  lastsecondary = (char *) xmalloc (lastsecondarylength + 1);
  memset(( lastsecondary ), '\0', (  lastsecondarylength + 1 )) ;
}




void
indexify (char *line, FILE *ostream)
{
  char *primary, *secondary, *pagenumber;
  int primarylength, secondarylength = 0, pagelength;
  int nosecondary;
  int initiallength;
  char *initial;
  char initial1[2];
  register char *p;



  p = find_braced_pos (line, 0, 0, 0);
  if (*p == '{')
    {
      initial = p;


      initiallength = find_braced_end (p + 1) + 1 - p;
    }
  else
    {
      initial = initial1;
      initial1[0] = *p;
      initial1[1] = 0;
      initiallength = 1;

      if (initial1[0] >= 'a' && initial1[0] <= 'z')
    initial1[0] -= 040;
    }

  pagenumber = find_braced_pos (line, 1, 0, 0);
  pagelength = find_braced_end (pagenumber) - pagenumber;
  if (pagelength == 0)
    abort ();

  primary = find_braced_pos (line, 2, 0, 0);
  primarylength = find_braced_end (primary) - primary;

  secondary = find_braced_pos (line, 3, 0, 0);
  nosecondary = !*secondary;
  if (!nosecondary)
    secondarylength = find_braced_end (secondary) - secondary;


  if (strncmp (primary, lastprimary, primarylength))
    {

      if (pending)
    {
      fputs ("}\n", ostream);
      pending = 0;
    }



      if (initiallength != lastinitiallength ||
      strncmp (initial, lastinitial, initiallength))
    {
      fprintf (ostream, "\\initial {");
      fwrite (initial, 1, initiallength, ostream);
      fprintf (ostream, "}\n", initial);
      if (initial == initial1)
        {
          lastinitial = lastinitial1;
          *lastinitial1 = *initial1;
        }
      else
        {
          lastinitial = initial;
        }
      lastinitiallength = initiallength;
    }


      if (nosecondary)
    fputs ("\\entry {", ostream);
      else
    fputs ("\\primary {", ostream);
      fwrite (primary, primarylength, 1, ostream);
      if (nosecondary)
    {
      fputs ("}{", ostream);
      pending = 1;
    }
      else
    fputs ("}\n", ostream);


      if (lastprimarylength < primarylength)
    {
      lastprimarylength = primarylength + 100;
      lastprimary = (char *) xrealloc (lastprimary,
                       1 + lastprimarylength);
    }
      strncpy (lastprimary, primary, primarylength);
      lastprimary[primarylength] = 0;


      lastsecondary[0] = 0;
    }



  if (nosecondary && *lastsecondary)
    error ("entry %s follows an entry with a secondary name", line);


  if (!nosecondary && strncmp (secondary, lastsecondary, secondarylength))
    {
      if (pending)
    {
      fputs ("}\n", ostream);
      pending = 0;
    }


      fputs ("\\secondary {", ostream);
      fwrite (secondary, secondarylength, 1, ostream);
      fputs ("}{", ostream);
      pending = 1;


      if (lastsecondarylength < secondarylength)
    {
      lastsecondarylength = secondarylength + 100;
      lastsecondary = (char *) xrealloc (lastsecondary,
                         1 + lastsecondarylength);
    }
      strncpy (lastsecondary, secondary, secondarylength);
      lastsecondary[secondarylength] = 0;
    }


  if (pending++ != 1)
    fputs (", ", ostream);
  fwrite (pagenumber, pagelength, 1, ostream);
}



void
finish_index (FILE *ostream)
{
  if (pending)
    fputs ("}\n", ostream);
  free (lastprimary);
  free (lastsecondary);
}




void
writelines (char **linearray, int nlines, FILE *ostream)
{
  char **stop_line = linearray + nlines;
  char **next_line;

  init_index ();



  for (next_line = linearray; next_line != stop_line; next_line++)
    {

      if (next_line == linearray


      || compare_general (*(next_line - 1), *next_line, 0L, 0L, num_keyfields - 1))
    {
      char *p = *next_line;
      char c;

      while ((c = *p++) && c != '\n')
          ;
      *(p - 1) = 0;
      indexify (*next_line, ostream);
    }
    }

  finish_index (ostream);
}



int
merge_files (char **infiles, int nfiles, char *outfile)
{
  char **tempfiles;
  int ntemps;
  int i;
  int value = 0;
  int start_tempcount = tempcount;

  if (nfiles <= 10 )
    return merge_direct (infiles, nfiles, outfile);




  ntemps = (nfiles + 10  - 1) / 10 ;
  tempfiles = (char **) xmalloc (ntemps * sizeof (char *));
  for (i = 0; i < ntemps; i++)
    {
      int nf = 10 ;
      if (i + 1 == ntemps)
    nf = nfiles - i * 10 ;
      tempfiles[i] = maketempname (++tempcount);
      value |= merge_direct (&infiles[i * 10 ], nf, tempfiles[i]);
    }




  flush_tempfiles (start_tempcount);



  merge_files (tempfiles, ntemps, outfile);

  free (tempfiles);

  return value;
}



int
merge_direct (char **infiles, int nfiles, char *outfile)
{
  struct linebuffer *lb1, *lb2;
  struct linebuffer **thisline, **prevline;
  FILE **streams;
  int i;
  int nleft;
  int lossage = 0;
  int *file_lossage;
  struct linebuffer *prev_out = 0;
  FILE *ostream = (&__sF[1]) ;

  if (outfile)
    {
      ostream = fopen (outfile, "w");
    }
  if (!ostream)
    pfatal_with_name (outfile);

  init_index ();

  if (nfiles == 0)
    {
      if (outfile)
    fclose (ostream);
      return 0;
    }










  lb1 = (struct linebuffer *) xmalloc (nfiles * sizeof (struct linebuffer));
  lb2 = (struct linebuffer *) xmalloc (nfiles * sizeof (struct linebuffer));



  thisline = (struct linebuffer **)
    xmalloc (nfiles * sizeof (struct linebuffer *));



  prevline = (struct linebuffer **)
    xmalloc (nfiles * sizeof (struct linebuffer *));

  streams = (FILE **) xmalloc (nfiles * sizeof (FILE *));


  file_lossage = (int *) xmalloc (nfiles * sizeof (int));



  for (i = 0; i < nfiles; i++)
    {
      initbuffer (&lb1[i]);
      initbuffer (&lb2[i]);
      thisline[i] = &lb1[i];
      prevline[i] = &lb2[i];
      file_lossage[i] = 0;
      streams[i] = fopen (infiles[i], "r");
      if (!streams[i])
    pfatal_with_name (infiles[i]);

      readline (thisline[i], streams[i]);
    }


  nleft = nfiles;

  while (nleft)
    {
      struct linebuffer *best = 0;
      struct linebuffer *exch;
      int bestfile = -1;
      int i;



      for (i = 0; i < nfiles; i++)
    {
      if (thisline[i] &&
          (!best ||
           0 < compare_general (best->buffer, thisline[i]->buffer,
                 (long) bestfile, (long) i, num_keyfields)))
        {
          best = thisline[i];
          bestfile = i;
        }
    }




      if (!(prev_out &&
        !compare_general (prev_out->buffer,
                  best->buffer, 0L, 1L, num_keyfields - 1)))
    indexify (best->buffer, ostream);
      prev_out = best;




      exch = prevline[bestfile];
      prevline[bestfile] = thisline[bestfile];
      thisline[bestfile] = exch;

      while (1)
    {


      if ((((  streams[bestfile]  )->_flags & 0x0020 ) != 0)  )
        {
          thisline[bestfile] = 0;

          nleft--;
          break;
        }
      readline (thisline[bestfile], streams[bestfile]);
      if (thisline[bestfile]->buffer[0] || !(((  streams[bestfile]  )->_flags & 0x0020 ) != 0)  )
        break;
    }
    }

  finish_index (ostream);



  for (i = 0; i < nfiles; i++)
    {
      fclose (streams[i]);
      free (lb1[i].buffer);
      free (lb2[i].buffer);
    }
  free (file_lossage);
  free (lb1);
  free (lb2);
  free (thisline);
  free (prevline);
  free (streams);

  if (outfile)
    fclose (ostream);

  return lossage;
}



void
fatal (char *s1, char *s2)
{
  error (s1, s2);
  exit (1 );
}



void
error (char *s1, char *s2)
{
  printf ("%s: ", program_name);
  printf ("%s %s",s1, s2);
  printf ("\n");
}

void
perror_with_name (char *name)
{
  char *s;

  if (errno < sys_nerr)
    s = concat ("", sys_errlist[errno], " for %s");
  else
    s = "cannot open %s";
  error (s, name);
}

void
pfatal_with_name (char *name)
{
  char *s;

  if (errno < sys_nerr)
    s = concat ("", sys_errlist[errno], " for %s");
  else
    s = "cannot open %s";
  fatal (s, name);
}




char *
concat (char *s1, char *s2, char *s3)
{
  int len1 = strlen (s1), len2 = strlen (s2), len3 = strlen (s3);
  char *result = (char *) xmalloc (len1 + len2 + len3 + 1);

  strcpy (result, s1);
  strcpy (result + len1, s2);
  strcpy (result + len1 + len2, s3);
  *(result + len1 + len2 + len3) = 0;

  return result;
}


void *
xmalloc (int nbytes)
{
  void *temp = (void *) malloc (nbytes);

  if (nbytes && temp == (void *)0 )
    memory_error ("xmalloc", nbytes);

  return (temp);
}


void *
xrealloc (void *pointer, int nbytes)
{
  void *temp;

  if (!pointer)
    temp = (void *)xmalloc (nbytes);
  else
    temp = (void *)realloc (pointer, nbytes);

  if (nbytes && !temp)
    memory_error ("xrealloc", nbytes);

  return (temp);
}

void
memory_error (char *callers_name, int bytes_wanted)
{
  char printable_string[80];

  sprintf (printable_string,
       "Virtual memory exhausted in %s ()!  Needed %d bytes.",
       callers_name, bytes_wanted);

  error (printable_string, " ");
  abort ();
}


