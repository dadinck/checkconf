#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
/* Temporary file name for checking */
#define TESTFILEBASE "testmex762r"
/* Output file name */
#define CONFIG_HEADER "config_check.h"
#ifdef __VMS
/* Default file to hold check definitions */
#  define CHECKFILENAME "checkconf.vms"
#  include <unixio.h>
#  define strdup(a) stringduplicate((a))
#  define unlink(a) remove((a))
#  define COMPILE "CC"
#  ifdef __ALPHA
#    define COMPILE_SUCCESS 0X5f60001
#  else
#    define COMPILE_SUCCESS 0xb90001
#  endif /* __ALPHA */
#  define LINK_SUCCESS    1
#  define LINK "LINK"
#  define TESTFILE TESTFILEBASE ".c"
#  define TESTOBJ  TESTFILEBASE ".obj"
#  define TESTEXE  TESTFILEBASE ".exe"
#endif /* defined VMS */
#ifdef _WIN32
/* Default file to hold check definitions */
#  define CHECKFILENAME "checkconf.win"
#  define COMPILE "cl /c /w /nologo"
#  define COMPILE_SUCCESS 0
#  define LINK_SUCCESS    0
#  define LINK "LINK /nologo"
#  define TESTFILE TESTFILEBASE ".c"
#  define TESTOBJ  TESTFILEBASE ".obj"
#  define TESTEXE  TESTFILEBASE ".exe"
#endif /* defined WIN32 */
#ifdef __unix__
/* Default file to hold check definitions */
#  define CHECKFILENAME "checkconf.unix"
#  define COMPILE "cc -c"
#  define COMPILE_SUCCESS 0
#  define LINK_SUCCESS    0
#  define TESTFILE TESTFILEBASE ".c"
#  define TESTOBJ  TESTFILEBASE ".o"
#  define TESTEXE  TESTFILEBASE
#  define LINK "cc -o " TESTEXE
#endif /* defined __unix__ */

/*  Need to check headers, functions, and defines */

/* to run: */
/* run checkconf */
/* checkconf [checkconf.lis [option.opt]] 
/* checkheader head1 sys/types.h */
/* compile cflags *file* */
/* link lflags *file* libs */
/* cflags c flags */
/* lflags link flags */
/* libs   libraries */
/* checktype   none deftype */
/* checktype   time time_t */
/* checkfunc   head1,head2 func(type a, type b, type c) */
/*                */

#ifdef KEEPTESTFILE
#undef unlink
#define unlink(a) /* a */
#endif

FILE * config;
char cflags[300];
char lflags[300];
char libs[300];
char headers[400];
char item[400];
char check[400];
char * maintop   = "#include <stdio.h>\n";
char * mainstart = "int\nmain() {\n";
char * mainmid   = "printf(\"hello\\n\");\n";
char * mainend   = "\n}\n";


char *
stringduplicate(char * s) /*strdup clone*/
{
   char * p = (char *) malloc(strlen(s)+1);
   if(p == NULL) return p;
   strcpy(p, s);
   return p;
}

void
chomp(char * f) /* remove trailing newline if present */
{
   char * p;
   for(p=f; *p!=0; ++p);
   if(p!=f && *(p-1) == '\n') *(p-1) = '\0';
}



void
itemize(char * it)
{
  int x, y;
  strcpy(item, "HAVE_");
  for(x=0, y=5; it[x] != 0 && it[x] != '('; ++x, ++y) {
	if(it[x] == '.' || it[x] == '/' || it[x] == ' ') {
	  item[y] = '_';
   	} else {
	  item[y] = toupper(it[x]);
	}
  }
  item[y] = 0;
}

void
includes(char * heads)
{
  char * aheader;
  headers[0] = '\0';
  if(strcmp(heads, "none") == 0) return;
  aheader = strtok(heads, ",");
  while(aheader) {
    sprintf(headers, "%s#include <%s.h>\n", headers, aheader);
    aheader = strtok(0, ",");
  }  
}

int
parseheader(char * line)
{
  char * myline = strdup(line);
  char * headnames, *theheader, * aheader;
  int retval = -1;
  check[0]='\0';
  if(myline == NULL) return -1;
  chomp(myline);
  if(strtok(myline, " ") == NULL) goto quitparseheader;
  headnames = strtok(0, " ");
  if(headnames == NULL) goto quitparseheader;
  theheader = strtok(0, " ");
  includes(headnames);
  if(theheader == NULL) goto quitparseheader;
  strcpy(check, theheader);
  itemize(check);
  sprintf(headers, "%s#include <%s>\n", headers, theheader);
  retval = 0;
  quitparseheader:
  free(myline);
  return(retval);
}

void
checkforheader(char * what)
{
  FILE * testfile = fopen(TESTFILE, "w");
  int status;
  char cmdstr[400];
  if(testfile == NULL) {
	fprintf(config, "/* could not open %s for %s */\n", TESTFILE, what);
	return;
  }
  if(parseheader(what) != 0) {
    printf("parse error for %s", what);
    fclose(testfile);
    return;
  }
  printf("checking for %s\n", check);
  fprintf(testfile, "%s%s", maintop, headers);
  fprintf(testfile, "%s%s%s", mainstart, mainmid, mainend);
  fclose(testfile);
  sprintf(cmdstr, COMPILE " %s %s", cflags, TESTFILE);
  status = system(cmdstr);
  unlink(TESTFILE);
  unlink(TESTOBJ);
  fprintf(config, "/* define %s if you have %s */\n", item, check);
  if(status == COMPILE_SUCCESS) {
	printf("ok\n");
	fprintf(config, "#define %s 1\n\n", item);
  } else {
        printf("not found %x\n",status);
	fprintf(config, "/* #undef %s */\n\n", item);
  }
}

int
parsetype(char * thistype)
{
  int retval = -1;
  char * myline = strdup(thistype);
  char * headnames, *thetype;
  check[0] = '\0';
  if(myline == NULL) return(retval);
  chomp(myline);
  if(strtok(myline, " ") == NULL) goto quitparsetype;
  headnames = strtok(0, " ");
  if(headnames == NULL) goto quitparsetype;
  thetype = strtok(0, "\n");
  includes(headnames);
  if(thetype == NULL) goto quitparsetype;
  strcpy(check, thetype);
  itemize(check);
  retval = 0;
 quitparsetype:
  free(myline);
  return(retval);
}

void
checktype(char * what)
{
  FILE * testfile = fopen(TESTFILE, "w");
  int status;
  char cmdstr[400];
  if(testfile == NULL) {
	fprintf(config, "/* could not open %s for %s */\n", TESTFILE, what);
	return;
  }
  if(parsetype(what) != 0) {
    printf("parse error for %s", what);
    fclose(testfile);
    return;
  }
  printf("checking for %s\n", check);
  fprintf(testfile, "%s%s", maintop, headers);
  fprintf(testfile, "%s%s foo;\n%s%s", mainstart, check, mainmid, mainend);
  fclose(testfile);
  sprintf(cmdstr, COMPILE " %s %s", cflags, TESTFILE);
  status = system(cmdstr);
  unlink(TESTFILE);
  unlink(TESTOBJ);
  fprintf(config, "/* define %s if you have %s */\n", item, check);
  if(status != COMPILE_SUCCESS) {
        printf("not found %x\n",status);
	fprintf(config, "/* #undef %s */\n\n", item);
  } else {
	printf("ok\n");
	fprintf(config, "#define %s 1\n\n", item);
  }

  return;
}

int
parsefunc(char * tryit, char * fargs)
{
  int retval = -1, idx;
  char * myline = strdup(tryit);
  char * eoline, *funct;
  char * argstart;
  char * headnames, *thetype;
  check[0] = '\0';
  if(myline == NULL) return(retval);
  chomp(myline);
  eoline = myline + strlen(myline);
  if(strtok(myline, " ") == NULL) goto quitparsefunc;
  headnames = strtok(0, " ");
  if(headnames == NULL) goto quitparsefunc;
  /* find function */
  for (funct = headnames; *funct != '\0'; ++ funct);
  for (++funct; funct < eoline && *funct == ' '; ++ funct);
  strcpy(check, funct);
  itemize(check);
  includes(headnames);
  for (argstart=funct;*argstart!=0 && *argstart!= '(';++argstart);
  if(*argstart=='(') {
    strcpy(fargs, argstart+1);
    for (idx=0; fargs[idx]!=0 && fargs[idx]!=')'; ++idx) {
      if(fargs[idx] == ',') {
	fargs[idx] = ';';
      }
    } 
    if(fargs[idx] == ')') {
      fargs[idx] = ';';
      fargs[idx+1] = '\0';
    }
  } else {
    fargs[0] = '\0';
  }
  retval = 0;
 quitparsefunc:
  free(myline);
  return(retval);
}

/* remove declarations in argument list
   eg. strcpy(char * a, char * b) becomes strcpy(a, b) */
void
rmdecl(char * callfunc)
{
  char * src, * dest, * at;
  /* find to beginning of paren */
  for(src=dest=callfunc; *src != '\0' && *src != '('; ++src, ++dest);
  if(*src == '(') {++src; ++dest;} else {return;}
  while(*src != /* ( balance parens */ ')' && *src != '\0') {
    /* find to first comma or end paren */
    for(;*src != '\0' && *src != ',' && *src != ')'; ++src);
    at = src;
    /* go back to space or * */
    for(--src;src > dest && *src != ' ' && *src != '*';--src);
    /* copy */
    for(++src;src<=at;++src,++dest) {*dest = *src;}
    *dest = *src;
  }
  dest[1] = '\0';
}

void
checkfunc(char * what, char * optfile)
{
  int ccstat, linkstat;
  char argdefs[400], callargs[400];
  FILE * testfile = fopen(TESTFILE, "w");
  char cmdstr[400];
  if(testfile == NULL) {
	fprintf(config, "/* could not open %s for %s */\n", TESTFILE, what);
	return;
  }
  
  if(parsefunc(what, argdefs) != 0) {
    printf("parse error for %s", what);
    fclose(testfile);
    return;
  }
  printf("checking for %s\n", check);
  strcpy(callargs, check);
  rmdecl(callargs);
  fprintf(testfile, "%s%s%s", maintop, headers, mainstart);
  fprintf(testfile, "%s;\n%s;\n%s%s", argdefs, callargs, mainmid, mainend);
  fclose(testfile);
  sprintf(cmdstr, COMPILE " %s %s", cflags, TESTFILE);
  ccstat = system(cmdstr);
  sprintf(cmdstr, LINK " %s %s %s", lflags, TESTOBJ, libs);
  /* printf("%s\n", cmdstr); */
  linkstat = system(cmdstr);
  unlink(TESTFILE);
  unlink(TESTOBJ);
  unlink(TESTEXE);
  fprintf(config, "/* define %s if you have %s */\n", item, check);
  if(ccstat != COMPILE_SUCCESS || linkstat != LINK_SUCCESS) {
    printf("not found cc %x link %x\n", ccstat, linkstat);
	fprintf(config, "/* #undef %s */\n\n", item);
  } else {
    printf("ok\n");
	fprintf(config, "#define %s 1\n\n", item);
  }

  return;
}

int
main(int argc, char * argv[])
{
  char * checkthis, checkwhat[200];
  FILE * checks;
  char * optfile;
  cflags[0] = '\0';
  lflags[0] = '\0';
  libs[0]   = '\0';
  if(argc < 2) {
     checkthis = CHECKFILENAME;
  } else {
     checkthis = argv[1];
  }
  if(argc > 2) {
    optfile = argv[2];
  } else {
    optfile = NULL;
  }
  checks = fopen(checkthis, "r");
  if(checks == NULL) {
	printf("File %s not found\n", checkthis);
        exit(1);
  }
  config = fopen(CONFIG_HEADER, "w");
  if(config == NULL) {
	printf("could not open output file\n");
	exit(1);
  }
  while(fgets(checkwhat, 199, checks)) {
    if(strncmp(checkwhat, "checkheader", 11) == 0) {
	checkforheader(checkwhat);
    } else if(strncmp(checkwhat, "checktype", 9) == 0) {
	checktype(checkwhat);
    } else if(strncmp(checkwhat, "checkfunc", 9) == 0) {
	checkfunc(checkwhat, optfile);
    } else if(strncmp(checkwhat, "cflags", 6) == 0) { /* set cflags */
        strcpy(cflags, checkwhat+7);
	chomp(cflags);
    } else if(strncmp(checkwhat, "lflags", 6) == 0) { /* set linker options */
      strcpy(lflags, checkwhat+7); chomp(lflags);
    } else if(strncmp(checkwhat, "libs", 4) == 0) {/* set libraries to include in link */
      strcpy(libs, checkwhat + 5); chomp(libs);
    } else {
        printf("don't know what to check for %s", checkwhat);
    }
  }
}

