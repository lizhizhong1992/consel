/*

  misc.c: miscellaneous functions

  Time-stamp: <2001-04-12 08:24:47 shimo>

  shimo@ism.ac.jp 
  Hidetoshi Shimodaira

  Meschach library:  v_sort, skipjunk
  Molphy: luinverse

*/


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <sys/time.h>
#include "misc.h"

static const char rcsid[] = "$Id$";

/*
  error message handling
*/

int debugmode=0;
void dprintf(int n, char *fmt, ...)
{
  va_list args;
  va_start(args,fmt);
  if(debugmode>=n) {
    vprintf(fmt, args);  
  }
  va_end(args);
}

void error(char *fmt, ...)
{
  va_list args;
  va_start(args,fmt);
  printf("\n#! Terminated: ");
  vprintf(fmt, args);
  printf(".\n");
  va_end(args);
  exit(1);
}

void warning(char *fmt, ...)
{
  va_list args;
  va_start(args,fmt);
  printf("\n#! Warning: ");
  vprintf(fmt, args);
  printf(".\n");
  va_end(args);
}

/*
  get time from the system clock
*/

double get_time(void)
{
  clock_t t;
  double x;
  static clock_t t0=0;

  struct tms {
    clock_t    tms_utime;
    clock_t    tms_stime;
    clock_t    tms_cutime;
    clock_t    tms_cstime;
  } tmrec;

  clock_t times();

  t = times(&tmrec);
  t = tmrec.tms_utime+tmrec.tms_stime;
  if(t0==0) t0=t;
  x = (double) (t-t0) / 100.0; /* seconds */

  return x;
}

/*
  memory allocation: free, alloc, realloc
*/
void myfree(void *ptr)
{
  free(ptr);
}

void *myalloc(size_t size)
{
    void *ptr;
    if(size==0) warning("zero size in malloc");
    ptr=calloc(size,1); /* use calloc to initialize for safty */
    if(ptr == NULL) error("cant allocate memory size=%d",size);
    return ptr;
}

void *myrealloc(void *old, size_t size)
{
    void *ptr;
    ptr=realloc(old,size);
    if(ptr == NULL) error("cant reallocate memory size=%d",size);
    return ptr;
}

/*
  matrix
*/
double **new_mat(int m, int n)
{
  double **base,*xp;
  int i;

  xp = new_vec(m*n);
  base = (double **)MALLOC(sizeof(double*) * m);
  for(i=0;i<m;i++) base[i] = xp + i*n;

  return base;
}

double **renew_mat(double **old, int m, int n)
{
  double *buf,**base;
  int i;

  if(old) buf=old[0]; else buf=NULL;
  buf=(double *)REALLOC(buf,sizeof(double)*(m*n));
  base=(double **)REALLOC(old,sizeof(double*) * m);
  for(i=0;i<m;i++) base[i] = buf + i*n;

  return base;
}

void free_mat(double **buf)
{
  FREE(buf[0]);
  FREE(buf);
}

/* skipjunk -- skips white spaces and strings of the form #....\n
   Here .... is a comment string 
   borrowed from from "Meschach" library
*/
int fskipjunk(FILE *fp)
{
  int c;
     
  for ( ; ; ) {       /* forever do... */
    /* skip blanks */
    do c = getc(fp);
    while ( isspace(c) );

    /* skip comments (if any) */
    if ( c == '#' )
      /* yes it is a comment (line) */
      while ( (c=getc(fp)) != '\n' );
    else {
      ungetc(c,fp);
      break;
    }
  }
  return 0;
}

/*
  string comparison returns 1 if they are identical otherwise 0
*/
int streq(char const *s1,char const *s2)
{
    int i=0;
    while(s1[i] == s2[i]) {
        if(!s1[i]) return 1;
        i++;
    }
    return 0;
}
/*
  catenate string with memory allocation
*/
char *mstrcat(char *str1, char *str2)
{
  int len1,len2;
  char *str;
  len1 = strlen(str1); len2 = strlen(str2);
  str = (char *)MALLOC(len1+len2+1);
  strcpy(str,str1);
  strcpy(str+len1,str2);
  return str;
}

/*
  ascii read/write
*/

int fread_i(FILE *fp)
{
  int x;
  fskipjunk(fp);
  if(fscanf(fp,"%d",&x)!=1) error("cant read int");
  return x;
}

double fread_d(FILE *fp)
{
  double x;
  fskipjunk(fp);
  if(fscanf(fp,"%lf",&x)!=1) error("cant read double");
  return x;
}

double *fread_vec(FILE *fp, int *mp)
{
  int i,m;
  double *A;

  m=fread_i(fp); /* number of items */
  if(*mp>0 && *mp != m) error("size mismatch in vec");
  A = new_vec(m);

  for(i=0;i<m;i++) A[i]=fread_d(fp);

  *mp = m;
  return A;
}

int *fread_ivec(FILE *fp, int *mp)
{
  int i,m;
  int *A;

  m=fread_i(fp); /* number of items */
  if(*mp>0 && *mp != m) error("size mismatch in ivec");
  A = new_ivec(m);

  for(i=0;i<m;i++) A[i]=fread_i(fp);

  *mp = m;
  return A;
}

double **fread_mat(FILE *fp, int *mp, int *np)
{
  int i,j,m,n;
  double **A;

  m=fread_i(fp); /* number of items (rows) */
  n=fread_i(fp); /* number of samples (columns) */
  if(*mp>0 && *mp != m) error("size of rows mismatch in mat");
  if(*np>0 && *np != n) error("size of columns mismatch in mat");

  A = new_mat(m,n);

  for(i=0;i<m;i++) for(j=0;j<n;j++) A[i][j]=fread_d(fp);

  *mp = m;
  *np = n;
  return A;
}

double **freread_mat(FILE *fp, int *mp, int *np, double **old)
{
  int i,j,m,n;
  double **A;

  m=fread_i(fp); /* number of items (rows) */
  n=fread_i(fp); /* number of samples (columns) */
  if(*mp>0 && *mp != m) error("size of rows mismatch in mat");
  if(*np>0 && *np != n) error("size of columns mismatch in mat");

  A = renew_mat(old,m,n);

  for(i=0;i<m;i++) for(j=0;j<n;j++) A[i][j]=fread_d(fp);

  *mp = m;
  *np = n;
  return A;
}

static int mcol=5;
static char    *format = "%14.9g ";

int fwrite_vec(FILE *fp, double *A, int m)
{
  int i,k;

  fprintf(fp,"#!VEC:\n%d\n",m);
  k=0;
  for(i=0;i<m;i++) {
    if(++k > mcol) {
      putc('\n',fp);
      k=1;
    }
    fprintf(fp,format, A[i]);
  }
  putc('\n',fp);
  return 0;
}

int fwrite_ivec(FILE *fp, int *A, int m)
{
  int i,k;

  fprintf(fp,"#!VEC:\n%d\n",m);
  k=0;
  for(i=0;i<m;i++) {
    if(++k > mcol) {
      putc('\n',fp);
      k=1;
    }
    fprintf(fp,"%10d ", A[i]);
  }
  putc('\n',fp);
  return 0;
}

int fwrite_mat(FILE *fp, double **A, int m, int n)
{
  int i,j,k;
  double *xp;

  fprintf(fp,"#!MAT:\n%d %d\n",m,n);

  for(i=0;i<m;i++) {
    fprintf(fp,"\n# row: %d\n",i);
    k=0;
    xp=A[i];
    for(j=0;j<n;j++) {
      if(++k > mcol) {
	putc('\n',fp);
	k=1;
      }
      fprintf(fp,format,xp[j]);
    }
    putc('\n',fp);
  }
  return 0;
}

/*
  binary read/write
*/

int fread_bi(FILE *fp)
{
  int x;
  if(fread(&x,sizeof(int),1,fp)!=1) error("cant read binary int");
  return x;
}

int fread_bd(FILE *fp)
{
  double x;
  if(fread(&x,sizeof(double),1,fp)!=1) error("cant read binary double");
  return x;
}

int *fread_bivec(FILE *fp, int *mp)
{
  int m;
  int *A;

  m=fread_bi(fp); /* number of itmes */
  if(*mp>0 && *mp != m) error("size mismatch in bivec");
  A = new_ivec(m);
  if(fread(A,sizeof(int),m,fp)!=m) error("cant read bivec");
  *mp = m;
  return A;
}

double *fread_bvec(FILE *fp, int *mp)
{
  int m;
  double *A;

  m=fread_bi(fp); /* number of itmes */
  if(*mp>0 && *mp != m) error("size mismatch in bvec");
  A = new_vec(m);
  if(fread(A,sizeof(double),m,fp)!=m) error("cant read bvec");
  *mp = m;
  return A;
}

double **fread_bmat(FILE *fp, int *mp, int *np)
{
  int m,n;
  double **A;

  m=fread_bi(fp); /* number of items (rows) */
  n=fread_bi(fp); /* number of samples (columns) */
  if(*mp>0 && *mp != m) error("size of rows mismatch in bmat");
  if(*np>0 && *np != n) error("size of columns mismatch in bmat");
  A = new_mat(m,n);
  if(fread(A[0],sizeof(double),m*n,fp)!=(m*n)) error("cant read bmat");
  *mp = m; *np = n;
  return A;
}

double **freread_bmat(FILE *fp, int *mp, int *np, double **old)
{
  int m,n;
  double **A;

  m=fread_bi(fp); /* number of items (rows) */
  n=fread_bi(fp); /* number of samples (columns) */
  if(*mp>0 && *mp != m) error("size of rows mismatch in bmat");
  if(*np>0 && *np != n) error("size of columns mismatch in bmat");
  A = renew_mat(old,m,n);
  if(fread(A[0],sizeof(double),m*n,fp)!=(m*n)) error("cant read bmat");
  *mp = m; *np = n;
  return A;
}

int fwrite_bi(FILE *fp, int x)
{
  if(fwrite(&x,sizeof(int),1,fp)!=1) error("cant write binary int");
  return 0;
}

int fwrite_bd(FILE *fp, double x)
{
  if(fwrite(&x,sizeof(double),1,fp)!=1) error("cant write binary double");
  return 0;
}

int fwrite_bivec(FILE *fp, int *A, int m)
{
  fwrite_bi(fp,m);
  if(fwrite(A,sizeof(int),m,fp)!=m) error("cant write bivec");
  return 0;
}

int fwrite_bvec(FILE *fp, double *A, int m)
{
  fwrite_bi(fp,m);
  if(fwrite(A,sizeof(double),m,fp)!=m) error("cant write bvec");
  return 0;
}

int fwrite_bmat(FILE *fp, double **A, int m, int n)
{
  fwrite_bi(fp,m);
  fwrite_bi(fp,n);
  if(fwrite(A[0],sizeof(double),m*n,fp)!=(m*n)) error("cant write bmat");
  return 0;
}

/*
  symmetrize a matrix
*/
double sym_mat(double **mat, int m)
{
  double x,sum;
  int i,j;

  sum=0.0;
  for(i=0;i<m;i++)
    for(j=0;j<i;j++) {
      sum += fabs(mat[i][j]-mat[j][i]);
      x = mat[i][j]+mat[j][i];
      mat[i][j]=mat[j][i]=0.5*x;
    }
  return sum;
}

/*
  INVERSION OF MATRIX ON LU DECOMPOSITION
*/
void luinverse(double **omtrx, double **imtrx, int size) /* From Molphy */
{
double eps = 1.0e-20; /* ! */
	int i, j, k, l, maxi, idx, ix, jx;
	double sum, tmp, maxb, aw;
	int *index;
	double *wk;

	index = (int *) MALLOC((unsigned)size * sizeof(int));
	wk = (double *) MALLOC((unsigned)size * sizeof(double));

	aw = 1.0;
	for (i = 0; i < size; i++) {
		maxb = 0.0;
		for (j = 0; j < size; j++) {
			if (fabs(omtrx[i][j]) > maxb)
				maxb = fabs(omtrx[i][j]);
		}
		if (maxb == 0.0) {
		  error("luinverse: singular matrix");
		}
		wk[i] = 1.0 / maxb;
	}
	for (j = 0; j < size; j++) {
		for (i = 0; i < j; i++) {
			sum = omtrx[i][j];
			for (k = 0; k < i; k++)
				sum -= omtrx[i][k] * omtrx[k][j];
			omtrx[i][j] = sum;
		}
		maxb = 0.0;
		maxi=0;
		for (i = j; i < size; i++) {
			sum = omtrx[i][j];
			for (k = 0; k < j; k++)
				sum -= omtrx[i][k] * omtrx[k][j];
			omtrx[i][j] = sum;
			tmp = wk[i] * fabs(sum);
			if (tmp >= maxb) {
				maxb = tmp;
				maxi = i;
			}
		}
		if (j != maxi) {
			for (k = 0; k < size; k++) {
				tmp = omtrx[maxi][k];
				omtrx[maxi][k] = omtrx[j][k];
				omtrx[j][k] = tmp;
			}
			aw = -aw;
			wk[maxi] = wk[j];
		}
		index[j] = maxi;
		if (omtrx[j][j] == 0.0)
			omtrx[j][j] = eps;
		if (j != size - 1) {
			tmp = 1.0 / omtrx[j][j];
			for (i = j + 1; i < size; i++)
				omtrx[i][j] *= tmp;
		}
	}
	for (jx = 0; jx < size; jx++) {
		for (ix = 0; ix < size; ix++)
			wk[ix] = 0.0;
		wk[jx] = 1.0;
		l = -1;
		for (i = 0; i < size; i++) {
			idx = index[i];
			sum = wk[idx];
			wk[idx] = wk[i];
			if (l != -1) {
				for (j = l; j < i; j++)
					sum -= omtrx[i][j] * wk[j];
			} else if (sum != 0.0)
				l = i;
			wk[i] = sum;
		}
		for (i = size - 1; i >= 0; i--) {
			sum = wk[i];
			for (j = i + 1; j < size; j++)
				sum -= omtrx[i][j] * wk[j];
			wk[i] = sum / omtrx[i][i];
		}
		for (ix = 0; ix < size; ix++)
			imtrx[ix][jx] = wk[ix];
	}
	FREE(wk);
	FREE(index);
} /*_ luinverse */



/* borrowed from mesch */

#define	MAX_STACK	60

/* v_sort -- sorts vector x, and generates permutation that gives the order
	of the components; x = [1.3, 3.7, 0.5] -> [0.5, 1.3, 3.7] and
	the permutation is order = [2, 0, 1].
	-- if order is NULL on entry then it is ignored
	-- the sorted vector x is returned */
void sort(double *xve, int *order, int dim)
{
    double tmp, v;
    int	i, j, l, r, tmp_i;
    int	stack[MAX_STACK], sp;

    if(order != NULL) for(i=0;i<dim;i++) order[i]=i;

    if ( dim <= 1 )
	return;

    /* using quicksort algorithm in Sedgewick,
       "Algorithms in C", Ch. 9, pp. 118--122 (1990) */
    sp = 0;
    l = 0;	r = dim-1;	v = xve[0];
    for ( ; ; )
    {
	while ( r > l )
	{
	    /* "i = partition(x_ve,l,r);" */
	    v = xve[r];
	    i = l-1;
	    j = r;
	    for ( ; ; )
	    {
		while ( xve[++i] < v )
		    ;
		while ( xve[--j] > v )
		    ;
		if ( i >= j )	break;
		
		tmp = xve[i];
		xve[i] = xve[j];
		xve[j] = tmp;
		if(order != NULL) {
		  tmp_i = order[i];
		  order[i] = order[j];
		  order[j] = tmp_i;
		}
	    }

	    tmp = xve[i];
	    xve[i] = xve[r];
	    xve[r] = tmp;
	    if(order != NULL) {
	      tmp_i = order[i];
	      order[i] = order[r];
	      order[r] = tmp_i;
	    }

	    if ( i-l > r-i )
	    {   stack[sp++] = l;   stack[sp++] = i-1;   l = i+1;   }
	    else
	    {   stack[sp++] = i+1;   stack[sp++] = r;   r = i-1;   }
	}

	/* recursion elimination */
	if ( sp == 0 )
	    break;
	r = stack[--sp];
	l = stack[--sp];
    }

    return;
}

int *perm_ivec(int *px, int *iv, int n) 
{
  /*
    iv[i] := iv[px[i]]
   */

  int s,i,j;
  int x0;

  for(s=0;s<n;s++) {
    if(px[s]>=n) continue;

    x0=iv[s];
    i=s;   

    while( j = px[i], px[i] += n, j != s ) {
      iv[i] = iv[j];
      i = j;
    }

    iv[i] = x0;
  }
  for(s=0;s<n;s++) px[s] -= n;

  return iv;
}