/*===========================================================================*/
/*                                                                           */
/* This file is part of a demonstration application for use with the         */
/* SYMPHONY Branch, Cut, and Price Library. This application is a solver for */
/* the Matching Problem.                                                     */
/*                                                                           */
/* (c) Copyright 2003 Michael Trick and Ted Ralphs. All Rights Reserved.     */
/*                                                                           */
/* This application was originally written by Michael Trick and was modified */
/* by Ted Ralphs (tkralphs@lehigh.edu).                                      */
/*                                                                           */
/* This software is licensed under the Common Public License. Please see     */
/* accompanying file for terms.                                              */
/*                                                                           */
/*===========================================================================*/

/* system include files */
#include <malloc.h>
#include <memory.h>
#include <stdio.h>

/* SYMPHONY include files */
#include "BB_constants.h"
#include "BB_macros.h"
#include "cg_u.h"

/* MATCH include files */
#include "user.h"

/*===========================================================================*/

/*===========================================================================*\
 * This file contains user-written functions used by the cut generator
 * process.
\*===========================================================================*/

/*===========================================================================*\
 * Here is where the user must receive all of the data sent from
 * user_send_cg_data() and set up data structures. Note that this function is
 * only called if one of COMPILE_IN_CG, COMPILE_IN_LP, or COMPILE_IN_TM is
 * FALSE. For sequential computation, nothing is needed here.
\*===========================================================================*/

int user_receive_cg_data(void **user, int dg_id)
{
   return(USER_DEFAULT);
}

/*===========================================================================*/

/*===========================================================================*\
 * If the user wants to fill in a customized routine for sending and receiving
 * the LP solution, it can be done here. For most cases, the default routines
 * are fine.
\*===========================================================================*/

int user_receive_lp_solution_cg(void *user)
{
   return(USER_DEFAULT);
}

/*===========================================================================*/

/*===========================================================================*\
 * Find cuts violated by a particular LP solution. This can be a fairly
 * involved function but the bottom line is that an LP solution comes in
 * and cuts go out. Remember, use the function cg_send_cut() to send cuts out
 * when they are found.
\*===========================================================================*/

int user_find_cuts(void *user, int varnum, int iter_num, int level,
		   int index, double objval, int *indices, double *values,
		   double ub, double etol, int *num_cuts, int *alloc_cuts, 
		   cut_data ***cuts)
{
   user_problem *prob = (user_problem *) user;
   double edge_val[200][200]; /* Matrix of edge values */
   int i, j, k;
   int *new_cuts;
   cut_data cut;
   int cutnum;
   
   cutnum = 0;

   new_cuts = (int *) malloc(prob->nnodes * ISIZE);

   /* Allocate the edge_val matrix to zero (we could also just calloc it) */
   memset((char *)edge_val, 0, 200*200*ISIZE);
   
   for (i = 0; i < varnum; i++) {
      edge_val[prob->node1[indices[i]]][prob->node2[indices[i]]] 
	 = values[i];
   }
   for (i = 0; i < prob->nnodes; i++){
      for (j = i+1; j < prob->nnodes; j++){
	 for (k = j+1; k < prob->nnodes; k++) {
	    if (edge_val[i][j]+edge_val[j][k]+edge_val[i][k] > 1.0 + etol) {
	       memset(new_cuts, 0, prob->nnodes * ISIZE);
	       new_cuts[i] = 1; 
	       new_cuts[j] = 1;
	       new_cuts[k] = 1;
	       cut.size = (prob->nnodes)*ISIZE;
	       cut.coef = (char *) new_cuts;
	       cut.rhs = 1.0;
	       cut.range = 0.0;
	       cut.type = TRIANGLE;
	       cut.sense = 'L';
	       cut.deletable = TRUE;
	       cut.branch = ALLOWED_TO_BRANCH_ON;
	       cg_add_user_cut(&cut, num_cuts, alloc_cuts, cuts);
	       cutnum++; 
	    }
	 }
      }
   }
   
   FREE(new_cuts);

   return(USER_SUCCESS);
}

/*===========================================================================*/

/*===========================================================================*\
 * Free the user data structure. If the default setup is used with sequential
 * computation, nothing needs to be filled in here.
\*===========================================================================*/

int user_free_cg(void **user)
{
   return(USER_DEFAULT);
}

/*===========================================================================*/

/*===========================================================================*\
 * This is an undocumented (for now) debugging feature which can allow the user
 * to identify the cut which cuts off a particular known feasible solution.
\*===========================================================================*/

#ifdef CHECK_CUT_VALIDITY
int user_check_validity_of_cut(void *user, cut_data *new_cut)
{
  return(USER_DEFAULT);
}
#endif
