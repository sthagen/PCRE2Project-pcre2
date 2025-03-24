/*************************************************
*           PCRE2 DEMONSTRATION PROGRAM          *
*************************************************/

/* This is a demonstration program to illustrate a straightforward way of
using the PCRE2 regular expression library from a C program. See the
pcre2sample documentation for a short discussion ("man pcre2sample" if you have
the PCRE2 man pages installed). PCRE2 is a revised API for the library, and is
incompatible with the original PCRE API.

There are actually three libraries, each supporting a different code unit
width. This demonstration program uses the 8-bit library. The default is to
process each code unit as a separate character, but if the pattern begins with
"(*UTF)", both it and the subject are treated as UTF-8 strings, where
characters may occupy multiple code units.

In Unix-like environments, if PCRE2 is installed in your standard system
libraries, you should be able to compile this program using this command:

cc -Wall pcre2demo.c -lpcre2-8 -o pcre2demo

If PCRE2 is not installed in a standard place, it is likely to be installed
with support for the pkg-config mechanism. If you have pkg-config, you can
compile this program using this command:

cc -Wall pcre2demo.c `pkg-config --cflags --libs libpcre2-8` -o pcre2demo

If you do not have pkg-config, you may have to use something like this:

cc -Wall pcre2demo.c -I/usr/local/include -L/usr/local/lib \
  -R/usr/local/lib -lpcre2-8 -o pcre2demo

Replace "/usr/local/include" and "/usr/local/lib" with wherever the include and
library files for PCRE2 are installed on your system. Only some operating
systems (Solaris is one) use the -R option.

Building under Windows:

If you want to statically link this program against a non-dll .a file, you must
define PCRE2_STATIC before including pcre2.h, so in this environment, uncomment
the following line. */

/* #define PCRE2_STATIC */

/* The PCRE2_CODE_UNIT_WIDTH macro must be defined before including pcre2.h.
For a program that uses only one code unit width, setting it to 8, 16, or 32
makes it possible to use generic function names such as pcre2_compile(). Note
that just changing 8 to 16 (for example) is not sufficient to convert this
program to process 16-bit characters. Even in a fully 16-bit environment, where
string-handling functions such as strcmp() and printf() work with 16-bit
characters, the code for handling the table of named substrings will still need
to be modified. */

#define PCRE2_CODE_UNIT_WIDTH 8

#include <stdio.h>
#include <string.h>
#include <pcre2.h>


/**************************************************************************
* Here is the program. The API includes the concept of "contexts" for     *
* setting up unusual interface requirements for compiling and matching,   *
* such as custom memory managers and non-standard newline definitions.    *
* This program does not do any of this, so it makes no use of contexts,   *
* always passing NULL where a context could be given.                     *
**************************************************************************/

int main(int argc, char **argv)
{
pcre2_code *re;
PCRE2_SPTR pattern;     /* PCRE2_SPTR is a pointer to unsigned code units of */
PCRE2_SPTR subject;     /* the appropriate width (in this case, 8 bits). */
PCRE2_SPTR name_table;

int errornumber;
int find_all, caseless_match;
int i;
int rc;

uint32_t namecount;
uint32_t name_entry_size;

PCRE2_SIZE erroroffset;
PCRE2_SIZE *ovector;
PCRE2_SIZE ovector_last[2];
PCRE2_SIZE subject_length;

pcre2_match_data *match_data;


/**************************************************************************
* First, sort out the command line. Options:                              *
* - "-g" to request repeated matching to find all occurrences,            *
*   like Perl's /g option. We set the variable find_all to a non-zero     *
*   value if the -g option is present.                                    *
* - "-i" to request caseless matching, like Perl's /i option.  We set the *
*   variable caseless_match to PCRE2_CASELESS if the -i option is         *
*   present.                                                              *
**************************************************************************/

find_all = 0;
caseless_match = 0;
for (i = 1; i < argc; i++)
  {
  if (strcmp(argv[i], "-g") == 0) find_all = 1;
  else if (strcmp(argv[i], "-i") == 0) caseless_match = PCRE2_CASELESS;
  else if (argv[i][0] == '-')
    {
    printf("Unrecognised option %s\n", argv[i]);
    return 1;
    }
  else break;
  }

/* After the options, we require exactly two arguments, which are the pattern,
and the subject string. */

if (argc - i != 2)
  {
  printf("Exactly two arguments required: a regex and a subject string\n");
  return 1;
  }

/* Pattern and subject are char arguments, so they can be straightforwardly
cast to PCRE2_SPTR because we are working in 8-bit code units. The subject
length is cast to PCRE2_SIZE for completeness, though PCRE2_SIZE is in fact
defined to be size_t. */

pattern = (PCRE2_SPTR)argv[i];
subject = (PCRE2_SPTR)argv[i+1];
subject_length = (PCRE2_SIZE)strlen((char *)subject);


/*************************************************************************
* Now we are going to compile the regular expression pattern, and handle *
* any errors that are detected.                                          *
*************************************************************************/

re = pcre2_compile(
  pattern,               /* the pattern */
  PCRE2_ZERO_TERMINATED, /* indicates pattern is zero-terminated */
  caseless_match,        /* possibly enable caseless */
  &errornumber,          /* for error number */
  &erroroffset,          /* for error offset */
  NULL);                 /* use default compile context */

/* Compilation failed: print the error message and exit. */

if (re == NULL)
  {
  PCRE2_UCHAR buffer[256];
  pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
  printf("PCRE2 compilation failed at offset %d: %s\n", (int)erroroffset,
    buffer);
  return 1;
  }


/*************************************************************************
* If the compilation succeeded, we call PCRE2 again, in order to do a    *
* pattern match against the subject string. This does just ONE match. If *
* further matching is needed, it will be done below. Before running the  *
* match we must set up a match_data block for holding the result. Using  *
* pcre2_match_data_create_from_pattern() ensures that the block is       *
* exactly the right size for the number of capturing parentheses in the  *
* pattern. If you need to know the actual size of a match_data block as  *
* a number of bytes, you can find it like this:                          *
*                                                                        *
* PCRE2_SIZE match_data_size = pcre2_get_match_data_size(match_data);    *
*************************************************************************/

match_data = pcre2_match_data_create_from_pattern(re, NULL);

/* Now run the match. */

rc = pcre2_match(
  re,                   /* the compiled pattern */
  subject,              /* the subject string */
  subject_length,       /* the length of the subject */
  0,                    /* start at offset 0 in the subject */
  0,                    /* default options */
  match_data,           /* block for storing the result */
  NULL);                /* use default match context */

/* Matching failed: handle error cases */

if (rc < 0)
  {
  switch(rc)
    {
    case PCRE2_ERROR_NOMATCH: printf("No match\n"); break;
    /*
    Handle other special cases if you like
    */
    default: printf("Matching error %d\n", rc); break;
    }
  pcre2_match_data_free(match_data);   /* Release memory used for the match */
  pcre2_code_free(re);                 /*   data and the compiled pattern. */
  return 1;
  }

/* Match succeeded. Get a pointer to the output vector, where string offsets
are stored. */

ovector = pcre2_get_ovector_pointer(match_data);
printf("Match succeeded at offset %d\n", (int)ovector[0]);


/*************************************************************************
* We have found the first match within the subject string. If the output *
* vector wasn't big enough, say so. Then output any substrings that were *
* captured.                                                              *
*************************************************************************/

/* The output vector wasn't big enough. This should not happen, because we used
pcre2_match_data_create_from_pattern() above. */

if (rc == 0)
  printf("ovector was not big enough for all the captured substrings\n");

/* Since release 10.38 PCRE2 has locked out the use of \K in lookaround
assertions. This is the recommended behaviour. However, the option
PCRE2_EXTRA_ALLOW_LOOKAROUND_BSK allows applications to re-enable the old
behaviour. If that is set, it is possible to run patterns such as /(?=.\K)/ that
use \K in an assertion to set the start of a match later than its end. In this
demonstration program, we show how to detect this case, although it cannot arise
because the option is never set. */

if (ovector[0] > ovector[1])
  {
  printf("\\K was used in an assertion to set the match start after its end.\n"
    "From end to start the match was: %.*s\n", (int)(ovector[0] - ovector[1]),
      (char *)(subject + ovector[1]));
  printf("Run abandoned\n");
  pcre2_match_data_free(match_data);
  pcre2_code_free(re);
  return 1;
  }

/* Show substrings stored in the output vector by number. Obviously, in a real
application you might want to do things other than print them. */

for (i = 0; i < rc; i++)
  {
  PCRE2_SPTR substring_start = subject + ovector[2*i];
  PCRE2_SIZE substring_length = ovector[2*i+1] - ovector[2*i];
  printf("%2d: %.*s\n", i, (int)substring_length, (char *)substring_start);
  }


/**************************************************************************
* That concludes the basic part of this demonstration program. We have    *
* compiled a pattern, and performed a single match. The code that follows *
* shows first how to access named substrings, and then how to code for    *
* repeated matches on the same subject.                                   *
**************************************************************************/

/* See if there are any named substrings, and if so, show them by name. First
we have to extract the count of named parentheses from the pattern. */

(void)pcre2_pattern_info(
  re,                   /* the compiled pattern */
  PCRE2_INFO_NAMECOUNT, /* get the number of named substrings */
  &namecount);          /* where to put the answer */

if (namecount == 0)
  printf("No named substrings\n");
else
  {
  PCRE2_SPTR tabptr;
  printf("Named substrings\n");

  /* Before we can access the substrings, we must extract the table for
  translating names to numbers, and the size of each entry in the table. */

  (void)pcre2_pattern_info(
    re,                       /* the compiled pattern */
    PCRE2_INFO_NAMETABLE,     /* address of the table */
    &name_table);             /* where to put the answer */

  (void)pcre2_pattern_info(
    re,                       /* the compiled pattern */
    PCRE2_INFO_NAMEENTRYSIZE, /* size of each entry in the table */
    &name_entry_size);        /* where to put the answer */

  /* Now we can scan the table and, for each entry, print the number, the name,
  and the substring itself. In the 8-bit library the number is held in two
  bytes, most significant first. */

  tabptr = name_table;
  for (i = 0; i < namecount; i++)
    {
    int n = (tabptr[0] << 8) | tabptr[1];
    printf("(%d) %*s: %.*s\n", n, name_entry_size - 3, tabptr + 2,
      (int)(ovector[2*n+1] - ovector[2*n]), subject + ovector[2*n]);
    tabptr += name_entry_size;
    }
  }


/*************************************************************************
* If the "-g" option was given on the command line, we want to continue  *
* to search for additional matches in the subject string, in a similar   *
* way to the /g option in Perl. This turns out to be trickier than you   *
* might think because of the possibility of matching an empty string.    *
*                                                                        *
* To help with this task, PCRE2 provides the pcre2_next_match() helper.  *
*************************************************************************/

if (!find_all)     /* Check for -g */
  {
  pcre2_match_data_free(match_data);  /* Release the memory that was used */
  pcre2_code_free(re);                /* for the match data and the pattern. */
  return 0;                           /* Exit the program. */
  }

/* Loop for second and subsequent matches */

ovector_last[0] = ovector[0];
ovector_last[1] = ovector[1];

for (;;)
  {
  PCRE2_SIZE start_offset;
  uint32_t options;

  /* After each successful match, we use pcre2_next_match() to obtain the match
  parameters for subsequent match attempts. */

  if (!pcre2_next_match(match_data, &start_offset, &options))
    break;

  /* Run the next matching operation */

  rc = pcre2_match(
    re,                   /* the compiled pattern */
    subject,              /* the subject string */
    subject_length,       /* the length of the subject */
    start_offset,         /* starting offset in the subject */
    options,              /* options */
    match_data,           /* block for storing the result */
    NULL);                /* use default match context */

  /* If this match attempt fails, exit the loop for subsequent matches. */

  if (rc == PCRE2_ERROR_NOMATCH)
    break;

  /* Other matching errors are not recoverable. */

  if (rc < 0)
    {
    printf("Matching error %d\n", rc);
    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
    return 1;
    }

  /* This demonstration program depends on pcre2_next_match() to ensure that the
  loop for second and subsequent matches does not run forever. However, it would
  be robust practice for a production application to verify this. The following
  block of code shows how to do this. This error case is not reachable unless
  there is a bug in PCRE2.

  Because this program does not set the PCRE2_EXTRA_ALLOW_LOOKAROUND_BSK option,
  the logic is simple. We verify that either ovector[1] has advanced, or that we
  have an empty match touching the end of a previous non-empty match. See the
  API documentation for guidance if your application uses
  PCRE2_EXTRA_ALLOW_LOOKAROUND_BSK and searches for multiple matches. */

  if (!(ovector[1] > ovector_last[1] ||
        (ovector[1] == ovector[0] && ovector_last[1] > ovector_last[0] &&
         ovector[1] == ovector_last[1])))
    {
    printf("\\K was used in an assertion to yield non-advancing matches.\n");
    printf("Run abandoned\n");
    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
    return 1;
    }

  ovector_last[0] = ovector[0];
  ovector_last[1] = ovector[1];

  /* Match succeeded. */

  printf("\nMatch succeeded again at offset %d\n", (int)ovector[0]);

  /* The match succeeded, but the output vector wasn't big enough. This
  should not happen. */

  if (rc == 0)
    printf("ovector was not big enough for all the captured substrings\n");

  /* We guard against patterns such as /(?=.\K)/ that use \K in an assertion to
  set the start of a match later than its end. As explained above, this case
  should not occur because this demonstration program does not set the
  PCRE2_EXTRA_ALLOW_LOOKAROUND_BSK option, however, we do include code showing
  how to detect it. */

  if (ovector[0] > ovector[1])
    {
    printf("\\K was used in an assertion to set the match start after its end.\n"
      "From end to start the match was: %.*s\n", (int)(ovector[0] - ovector[1]),
        (char *)(subject + ovector[1]));
    printf("Run abandoned\n");
    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
    return 1;
    }

  /* As before, show substrings stored in the output vector by number, and then
  also any named substrings. */

  for (i = 0; i < rc; i++)
    {
    PCRE2_SPTR substring_start = subject + ovector[2*i];
    size_t substring_length = ovector[2*i+1] - ovector[2*i];
    printf("%2d: %.*s\n", i, (int)substring_length, (char *)substring_start);
    }

  if (namecount == 0)
    printf("No named substrings\n");
  else
    {
    PCRE2_SPTR tabptr = name_table;
    printf("Named substrings\n");
    for (i = 0; i < namecount; i++)
      {
      int n = (tabptr[0] << 8) | tabptr[1];
      printf("(%d) %*s: %.*s\n", n, name_entry_size - 3, tabptr + 2,
        (int)(ovector[2*n+1] - ovector[2*n]), subject + ovector[2*n]);
      tabptr += name_entry_size;
      }
    }
  }      /* End of loop to find second and subsequent matches */

printf("\n");

pcre2_match_data_free(match_data);
pcre2_code_free(re);
return 0;
}

/* End of pcre2demo.c */
