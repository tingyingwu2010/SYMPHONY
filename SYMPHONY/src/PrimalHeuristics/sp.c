/*===========================================================================*/
/*                                                                           */
/* This file is part of the SYMPHONY Branch, Cut, and Price Library.         */
/*                                                                           */
/* SYMPHONY was jointly developed by Ted Ralphs (tkralphs@lehigh.edu) and    */
/* Laci Ladanyi (ladanyi@us.ibm.com).                                        */
/*                                                                           */
/* (c) Copyright 2000-2007 Ted Ralphs. All Rights Reserved.                  */
/*                                                                           */
/*                                                                           */
/*===========================================================================*/

#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <string.h>

#include "sym_constants.h"
#include "sym_primal_heuristics.h"
#include "sym_macros.h"

/* Functions related to solution pool */

/*===========================================================================*/
/*===========================================================================*/
int sp_add_solution (lp_prob *p, int cnt, int *indices, double *values, 
      double obj_value, int bc_index)
{
   sp_desc *sp = p->tm->sp;
   int i;
   sp_solution *sol;
   
   if (sp->num_solutions == sp->max_solutions) {
      /* delete first solution and move everything up by 1 */
      sp_delete_solution(sp,0);
      /*
      for (i=0;i<(sp->max_solutions-1);i++) {
         sp->solutions[i] = sp->solutions[i+1];
      }
      */
   }
   sol = sp->solutions[sp->num_solutions];
   sol->objval = obj_value;
   sol->xlength = cnt;
   sol->xind = (int *) malloc(ISIZE*cnt);
   memcpy(sol->xind,indices,ISIZE*cnt);
   sol->xval = (double *) malloc(DSIZE*cnt);
   memcpy(sol->xval,values,DSIZE*cnt);
   sol->node_index = bc_index;
   sp->num_solutions++;
   PRINT(p->par.verbosity,-1,("sp: solution pool size = %d \n", 
            sp->num_solutions));
   return 0;
}

/*===========================================================================*/
/*===========================================================================*/
int sp_delete_solution (sp_desc *sp, int position)
{
   int i;
   if (position>=sp->num_solutions) {
      return 0;
   }

   FREE(sp->solutions[position]->xind);
   FREE(sp->solutions[position]->xval);
   for (i=position; i<sp->num_solutions-1; i++) {
      sp->solutions[i]->xind=sp->solutions[i+1]->xind;
      sp->solutions[i]->xval=sp->solutions[i+1]->xval;
      sp->solutions[i]->objval = sp->solutions[i+1]->objval;
      sp->solutions[i]->xlength = sp->solutions[i+1]->xlength;
      sp->solutions[i]->node_index = sp->solutions[i+1]->node_index;
   }
   sp->solutions[sp->num_solutions-1]->xlength = 0;
   sp->num_solutions--;
   return 0;
}

/*===========================================================================*/
/*===========================================================================*/
int sp_is_solution_in_sp (lp_prob *p, int cnt, int *indices, double *values, 
      double obj_value)
{
   /* not implemented yet */
   return 0;
}

/*===========================================================================*/
/*===========================================================================*/
int sp_initialize(tm_prob *tm)
{
   int i;
   tm->sp = (sp_desc*)malloc(sizeof(sp_desc));
   sp_desc *sp = tm->sp;
   sp->max_solutions = 10;
   sp->num_solutions = 0;
   sp->solutions = (sp_solution **) malloc(sp->max_solutions*sizeof(sp_solution*));
   for (i=0;i<sp->max_solutions;i++) {
      sp->solutions[i] = (sp_solution *) malloc(sizeof(sp_solution));
   }

   /* TODO: put the above as parameters */

   return 0;
}

/*===========================================================================*/
/*===========================================================================*/
int sp_free_sp(sp_desc *sp)
{
   int i;
   for (i=sp->num_solutions-1; i>=0; i--) {
      sp_delete_solution(sp,i);
   }
   for (i=sp->max_solutions-1; i>-1; i--) {
      FREE(sp->solutions[i]);
   }
   FREE(sp->solutions);
   return 0;
}
/*===========================================================================*/
/*===========================================================================*/
