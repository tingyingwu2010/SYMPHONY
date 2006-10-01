/*===========================================================================*/
/*                                                                           */
/* This file is part of the SYMPHONY Branch, Cut, and Price Library.         */
/*                                                                           */
/* SYMPHONY was jointly developed by Ted Ralphs (tkralphs@lehigh.edu) and    */
/* Laci Ladanyi (ladanyi@us.ibm.com).                                        */
/*                                                                           */
/* (c) Copyright 2000-2006 Ted Ralphs. All Rights Reserved.                  */
/*                                                                           */
/* This software is licensed under the Common Public License. Please see     */
/* accompanying file for terms.                                              */
/*                                                                           */
/* author: asm4                                                              */
/*===========================================================================*/

#ifndef _ROUNDING_H_
#define _ROUNDING_H_
/* rounding related */

typedef struct ROUNDING_PROB {
   int 		n;
   int 		m;
   int 		num_ints;
   int 		*int_vars;
   int          *is_int;
   double 	*rowActivity;
   const double	*rowLower;
   const double	*rowUpper;
   double 	*xval;
   double 	*rowMin;
   double 	*rowMax;
   const int	*rowStart;
   const int	*rowLength;
   const int	*rowIndices;
   const int    *colStart;
   const int    *colIndex;
   const int    *colLength;
   const double	*matrixByCol;
   double       *lb;
   double       *ub;
   double 	lpetol;
}rounding_problem;

int rnd_test();
int rnd_create_rnd_problem(lp_prob *p);
int rnd_find_row_bounds(rounding_problem *rp);
int rnd_initialize_rp(rounding_problem *rp, lp_prob *p);
int rnd_delete_rp(rounding_problem *rp);
int rnd_check_constr_feas(int n, int m, double *rowActivity, const double *rowLower, const double *rowUpper, double lpetol);
int rnd_check_integrality(int n, int *intvars, double *xval, double lpetol);
int rnd_lexicographic(rounding_problem *rp, lp_prob *p);
int rnd_find_feas_rounding(rounding_problem *rp, int grp_cnt, int *var_search_grp, int varnum);
int rnd_update_rp(rounding_problem *rp, int xind, double newval, double newlb, double newub);
int rnd_check_if_feas_impossible(int m, double *rowActivity, const double *rowLower, const double *rowUpper, double *rowMax, double *rowMin, double lpetol);
#endif
