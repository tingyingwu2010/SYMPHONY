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

#ifndef MASTER_U_H
#define MASTER_U_H

#include "proto.h"

/*===========================================================================*/
/*======================= User supplied functions ===========================*/
/*===========================================================================*/

void user_usage PROTO((void));
int user_initialize PROTO((void **user));
int user_free_master PROTO((void **user));
int user_readparams PROTO((void *user, char *filename, int argc, char **argv));
int user_io PROTO((void *user));
int user_init_draw_graph PROTO((void *user, int dg_id));
int user_start_heurs PROTO((void *user, double *ub, double *ub_estimate));
int user_set_base PROTO((void *user, int *basevarnum, int **basevars,
			 double **lb, double **ub, int *basecutnum,
			 int *colgen_strat));
int user_create_root PROTO((void *user, int *extravarnum, int **extravars));
int user_receive_feasible_solution PROTO((void *user, int msgtag, double cost,
					  int numvars, int *indices,
					  double *values));
int user_send_lp_data PROTO((void *user, void **user_lp));
int user_send_cg_data PROTO((void *user, void **user_cg));
int user_send_cp_data PROTO((void *user, void **user_cp));
/*__BEGIN_EXPERIMENTAL_SECTION__*/
int user_send_sp_data PROTO((void *user));
/*___END_EXPERIMENTAL_SECTION___*/
int user_display_solution PROTO((void *user, int length, int *xind,
				 double *xval));
int user_process_own_messages PROTO((void *user, int msgtag));
int user_send_feas_sol PROTO((void *user, int *feas_sol_size, int **feas_sol));

#endif
