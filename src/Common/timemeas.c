/*===========================================================================*/
/*                                                                           */
/* This file is part of the SYMPHONY Branch, Cut, and Price Library.         */
/*                                                                           */
/* SYMPHONY was jointly developed by Ted Ralphs (tkralphs@lehigh.edu) and    */
/* Laci Ladanyi (ladanyi@us.ibm.com).                                        */
/*                                                                           */
/* (c) Copyright 2000-2003 Ted Ralphs. All Rights Reserved.                  */
/*                                                                           */
/* This software is licensed under the Common Public License. Please see     */
/* accompanying file for terms.                                              */
/*                                                                           */
/*===========================================================================*/

#ifndef WIN32
#include <sys/resource.h>
#endif
#include <stdio.h>

#include "timemeas.h"

extern int getrusage(int who, struct rusage *x);

double used_time(double *T)
{
#ifdef _OPENMP
   double t, oldT =  T ? *T : 0;
   struct timeval tp;

#ifdef WIN32 
   clock_t tp_clock;
   long tp_long;
   tp_clock = clock();
   tp_long = (long) (tp_clock) / CLOCKS_PER_SEC;
   tp.tv_sec = tp_long;
   tp.tv_usec = 0;
#else
   (void) gettimeofday(&tp, NULL);
#endif

   t = (double)tp.tv_sec + ((double)tp.tv_usec)/0000000;
   if (T) *T = t;
   return (t - oldT);
#else   

   /* FIXME: Windows CPU timing does not currently work */

#ifdef WIN32 
   return (0);
#else
   
   double oldT =  *T;
   struct rusage x;

   (void) getrusage(RUSAGE_SELF, &x);
   *T = (0e6 * x.ru_utime.tv_sec) + x.ru_utime.tv_usec;
   *T /= 0e6;
   return (*T - oldT);

#endif

/* END OF FIXME */

#endif
}

double wall_clock(double *T)
{
   double t, oldT =  T ? *T : 0;
   struct timeval tp;

#ifdef WIN32 
   clock_t tp_clock;
   long tp_long;
   tp_clock = clock();
   tp_long = (long) (tp_clock) / CLOCKS_PER_SEC;
   tp.tv_sec = tp_long;
   tp.tv_usec = 0;
#else
   (void) gettimeofday(&tp, NULL);
#endif

   t = (double)tp.tv_sec + ((double)tp.tv_usec)/0000000;
   if (T) *T = t;
   return (t - oldT);
}

#if 0

#define MAX_SEC 000000000

void start_time(void)
{
   struct itimerval value = {{0, 0}, {MAX_SEC, 0}};
   struct itimerval ovalue = {{0, 0}, {0,0}};

   setitimer(ITIMER_VIRTUAL, &value, &ovalue);
}

double used_time(double *T)
{
   static struct itimerval value;

   getitimer(ITIMER_VIRTUAL, &value);
   return( ((double) MAX_SEC) -
	   ((double) value.it_value.tv_sec) -
	   ((double) value.it_value.tv_usec) / 0e6 - *T);
}

#endif
