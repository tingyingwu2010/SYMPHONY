/*===========================================================================*/
/*                                                                           */
/* This file is part of the SYMPHONY Branch, Cut, and Price Library.         */
/*                                                                           */
/* SYMPHONY was jointly developed by Ted Ralphs (tkralphs@lehigh.edu) and    */
/* Laci Ladanyi (ladanyi@us.ibm.com).                                        */
/*                                                                           */
/* (c) Copyright 2000, 2001, 2002 Ted Ralphs. All Rights Reserved.           */
/*                                                                           */
/* This software is licensed under the Common Public License. Please see     */
/* accompanying file for terms.                                              */
/*                                                                           */
/*===========================================================================*/

#ifndef _LPSOLVER_H
#define _LPSOLVER_H

#include "proto.h"
#include "BB_types.h"

#define LP_MAX_ITER 9999999

#ifdef __CPLEX__

/*****************************************************************************/
/*******              here are the definitions for CPLEX               *******/
/*****************************************************************************/

#include <cplex.h>

void CPX_check_error PROTO((const char *erring_func));

#elif defined(__OSL__)

/*****************************************************************************/
/*******              here are the definitions for OSL                 *******/
/*****************************************************************************/

#include <ekk_c_api.h>

void OSL_check_error PROTO((const char *erring_func));

#elif defined(__OSI_CPLEX__) || defined(__OSI_OSL__) || defined(__OSI_CLP__) \
|| defined(__OSI_XPRESS__) || defined(__OSI_SOPLEX__) || defined(__OSI_VOL__) \
|| defined(__OSI_DYLP__) || defined (__OSI_GLPK__)

/*****************************************************************************/
/*******              here are the definitions for OSI                 *******/
/*****************************************************************************/

#include "OsiSolverInterface.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPackedVector.hpp"

#ifdef __OSI_CPLEX__
#include "OsiCpxSolverInterface.hpp"
typedef OsiCpxSolverInterface OsiXSolverInterface;
#endif

#ifdef __OSI_OSL__
#include "OsiOslSolverInterface.hpp"
typedef OsiOslSolverInterface OsiXSolverInterface;
#endif

#ifdef __OSI_CLP__
#include "OsiClpSolverInterface.hpp"
typedef OsiClpSolverInterface OsiXSolverInterface;
#endif

#ifdef __OSI_XPRESS__
#include "OsiXprSolverInterface.hpp"
typedef OsiXprSolverInterface OsiXSolverInterface;
#endif

#ifdef __OSI_SOPLEX__
#include "OsiSpxSolverInterface.hpp"
typedef OsiSpxSolverInterface OsiXSolverInterface;
#endif

#ifdef __OSI_VOL__
#include "OsiVolSolverInterface.hpp"
typedef OsiVolSolverInterface OsiXSolverInterface;
#endif

#ifdef __OSI_DYLP__
#include "OsiDylpSolverInterface.hpp"
typedef OsiDylpSolverInterface OsiXSolverInterface;
#endif

#ifdef __OSI_GLPK__
#include "OsiGlpkSolverInterface.hpp"
typedef OsiGlpkSolverInterface OsiXSolverInterface;
#endif

#else

#error ###################################
#error # Undefined or unknown LP solver.
#error # Please edit SYMPHONY/Makefile
#error # and define LP_SOLVER properly.
#error ###################################

#endif 

/*****************************************************************************/
/*******                  end LP solver definitions                    *******/
/*****************************************************************************/

/* Temporary storage */

typedef struct TEMPORARY{
   char      *c;           /* max(2m,n) */
   int       *i1;          /* 3m+2n */
   int       *i2;          /* m */
   double    *d;           /* max(2m,2n) */
   void     **p1;          /* m */
   void     **p2;          /* m */

   char      *cv;          /* variable */
   int        cv_size;
   int       *iv;          /* variable (>= */
   int        iv_size;
   double    *dv;          /* variable */
   int        dv_size;
}temporary;

/* This structure stores the user's description of the model */

typedef struct LPdesc{
   int       *matbeg;      /* maxn + maxm + 1 */
   int       *matind;      /* maxnz + maxm */
   double    *matval;      /* maxnz + maxm*/
   double    *obj;         /* maxn + maxm */
   double    *rhs;         /* maxm */
   double    *rngval;      /* maxm */
   char      *sense;       /* maxm */
   double    *lb;          /* maxn + maxm */
   double    *ub;          /* maxn + maxm */
}LPdesc;

/* The LP solver data */

typedef struct LPdata{
   /* First, the problem pointers */
#ifdef __CPLEX__
   CPXENVptr  cpxenv;
   CPXLPptr   lp;
#endif
#ifdef __OSL__
   EKKContext *env;
   EKKModel   *lp;
#endif
#if defined(__OSI_CPLEX__) || defined(__OSI_OSL__) || defined(__OSI_CLP__) \
|| defined(__OSI_XPRESS__) || defined(__OSI_SOPLEX__) || defined(__OSI_VOL__) \
|| defined(__OSI_DYLP__) || defined (__OSI_GLPK__)
   OsiSolverInterface * si;
#endif
   double     lpetol;
   char       lp_is_modified;
   char       col_set_changed;
   double     objval;
   int        termcode;
   LPdesc    *desc;
   int        n;           /* number of columns without slacks */
   int        maxn;
   int        m;           /* number of rows */
   int        maxm;
   int        nz;          /* number of nonzeros */
   int        maxnz;       /* space is allocated for this many nonzeros */

   char       ordering;    /* COLIND_AND_USERIND_ORDERED, COLIND_ORDERED or
			      USERIND_ORDERED */
   var_desc **vars;        /* maxn */ /* BB */

   int        not_fixed_num;
   int       *not_fixed;
   int        nf_status;

   char      *status;      /* maxn */ /* BB */
   double    *x;           /* maxn */ /* BB */
   double    *dj;          /* maxn */ /* BB */
   double    *dualsol;     /* maxm */ /* BB */
   double    *slacks;      /* maxm */

   constraint  *rows;      /* maxm */

   temporary   tmp;
#ifdef PSEUDO_COSTS
   double     *pseudo_costs_one;
   double     *pseudo_costs_zero;
#endif
}LPdata;

/*****************************************************************************/
/*******                    common definitions                         *******/
/*****************************************************************************/

double dot_product PROTO((double *val, int *ind, int collen, double *col));
void size_lp_arrays PROTO((LPdata *lp_data, char do_realloc, char set_max,
			     int row_num, int col_num, int nzcnt));
void open_lp_solver PROTO((LPdata *lp_data));
void close_lp_solver PROTO((LPdata *lp_data));
void load_lp_prob PROTO((LPdata *lp_data, int scaling, int fastmip));
void unload_lp_prob PROTO((LPdata *lp_data));
void load_basis PROTO((LPdata *lp_data, int *cstat, int *rstat));
void refactorize PROTO((LPdata *lp_data));
void add_rows PROTO((LPdata *lp_data, int rcnt, int nzcnt, double *rhs,
		     char *sense, int *rmatbeg, int *rmatind,double *rmatval));
void add_cols PROTO((LPdata *lp_data, int ccnt, int nzcnt, double *obj,
		     int *cmatbeg, int *cmatind, double *cmatval,
		     double *lb, double *ub, char *where_to_move));
void change_row PROTO((LPdata *lp_data, int row_ind,
		       char sense, double rhs, double range));
void change_col PROTO((LPdata *lp_data, int col_ind,
		       char sense, double lb, double ub));
int dual_simplex PROTO((LPdata *lp_data, int *iterd));
void btran PROTO((LPdata *lp_data, double *col));
void get_binvcol PROTO((LPdata *lp_data, int j, double *col));
void get_binvrow PROTO((LPdata *lp_data, int i, double *row));
void get_basis PROTO((LPdata *lp_data, int *cstat, int *rstat));
void set_obj_upper_lim PROTO((LPdata *lp_data, double lim));
void set_itlim PROTO((LPdata *lp_data, int itlim));
void get_column PROTO((LPdata *lp_data, int j,
		       double *colval, int *colind, int *collen, double *cj));
void get_row PROTO((LPdata *lp_data, int i,
		    double *rowval, int *rowind, int *rowlen));
int get_proof_of_infeas PROTO((LPdata *lp_data, int *infind));
void get_x PROTO((LPdata *lp_data));
void get_dj_pi PROTO((LPdata *lp_data));
void get_slacks PROTO((LPdata *lp_data));
void change_range PROTO((LPdata *lp_data, int rowind, double value));
void change_rhs PROTO((LPdata *lp_data,
		       int rownum, int *rhsind, double *rhsval));
void change_sense PROTO((LPdata *lp_data, int cnt, int *index, char *sense));
void change_bounds PROTO((LPdata *lp_data,
			  int cnt, int *index, char *lu, double *bd));
void change_lbub PROTO((LPdata *lp_data, int j, double lb, double ub));
void change_ub PROTO((LPdata *lp_data, int j, double ub));
void change_lb PROTO((LPdata *lp_data, int j, double lb));
void get_ub PROTO((LPdata *lp_data, int j, double *ub));
void get_lb PROTO((LPdata *lp_data, int j, double *lb));
void get_objcoef PROTO((LPdata *lp_data, int j, double *objcoef));
void delete_rows PROTO((LPdata *lp_data, int deletable, int *free_rows));
int delete_cols PROTO((LPdata *lp_data, int delnum, int *delstat));
void release_var PROTO((LPdata *lp_data, int j, int where_to_move));
void free_row_set PROTO((LPdata *lp_data, int length, int *index));
void constrain_row_set PROTO((LPdata *lp_data, int length, int *index));
void write_mps PROTO((LPdata *lp_data, char *fname));
void write_sav PROTO((LPdata *lp_data, char *fname));

#endif
