/*===========================================================================*/
/*                                                                           */
/* This file is part of a demonstration application for use with the         */
/* SYMPHONY Branch, Cut, and Price Library. This application is a solver for */
/* Capacitated Network Routing Problems.                                     */
/*                                                                           */
/* (c) Copyright 2000-2003 Ted Ralphs. All Rights Reserved.                  */
/*                                                                           */
/* This application was developed by Ted Ralphs (tkralphs@lehigh.edu)        */
/*                                                                           */
/* This software is licensed under the Common Public License. Please see     */
/* accompanying file for terms.                                              */
/*                                                                           */
/*===========================================================================*/

/* system include files */
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

/* SYMPHONY include files */
#include "BB_macros.h"
#include "BB_constants.h"
#include "proccomm.h"
#include "qsortucb.h"
#include "cg_u.h"

/* CNRP include files */
#include "cnrp_cg.h"
#include "cnrp_macros.h"
#include "cnrp_const.h"

/*===========================================================================*/

/*===========================================================================*\
 * This file contains user-written functions used by the cut generator
 * process.
\*===========================================================================*/

/*===========================================================================*\
 * Here is where the user must receive all of the data sent from
 * user_send_cg_data() and set up data structures. Note that this function is
 * only called if one of COMPILE_IN_CG, COMPILE_IN_LP, or COMPILE_IN_TM is
 * FALSE.
\*===========================================================================*/

int user_receive_cg_data(void **user, int dg_id)
{
   int i, j, k;
   /* This is the user-defined data structure, a pointer to which will
      be passed to each user function. It must contain all the
      problem-specific data needed for computations within the CG */
   cg_vrp_spec *vrp = (cg_vrp_spec *) malloc(sizeof(cg_vrp_spec));
   int edgenum;

   *user = vrp;

   vrp->n = NULL;

   /*------------------------------------------------------------------------*\
    * Receive the data
   \*------------------------------------------------------------------------*/
   
   receive_char_array((char *)(&vrp->par), sizeof(cg_user_params));
   
   receive_int_array(&vrp->dg_id, 1);
   receive_int_array(&vrp->numroutes, 1);
   receive_int_array(&vrp->vertnum, 1);
   vrp->demand = (int *) calloc(vrp->vertnum, sizeof(int));
   receive_int_array(vrp->demand, vrp->vertnum);
   receive_int_array(&vrp->capacity, 1);
   edgenum = vrp->vertnum*(vrp->vertnum-1)/2;
#ifdef CHECK_CUT_VALIDITY
   receive_int_array(&vrp->feas_sol_size, 1);
   if (vrp->feas_sol_size){
      vrp->feas_sol = (int *) calloc(vrp->feas_sol_size, sizeof(int));
      receive_int_array(vrp->feas_sol, vrp->feas_sol_size);
   }
#endif

   /*------------------------------------------------------------------------*\
    * Set up some data structures
   \*------------------------------------------------------------------------*/

   vrp->in_set = (char *) calloc(vrp->vertnum, sizeof(char));
   vrp->ref = (int *) malloc(vrp->vertnum*sizeof(int));
   vrp->new_demand = (int *) malloc(vrp->vertnum*sizeof(int));
   vrp->cut_val = (double *) calloc(vrp->vertnum, sizeof(double));
   vrp->cut_list = (char *) malloc(((vrp->vertnum >> DELETE_POWER)+1)*
				   (vrp->par.max_num_cuts_in_shrink + 1)*
				   sizeof(char));
   
   vrp->edges = (int *) calloc(2*edgenum, sizeof(int));

   /*create the edge list (we assume a complete graph)*/
   for (i = 1, k = 0; i < vrp->vertnum; i++){
      for (j = 0; j < i; j++){
	 vrp->edges[k << 1] = j;
	 vrp->edges[(k << 1) + 1] = i;
	 k++;
      }
   }

   vrp->dg_id = dg_id;

   return(USER_SUCCESS);
}

/*===========================================================================*/

int user_receive_lp_solution_cg(void *user)
{
   /* Leave this job to SYMPHONY. We don't need anything special */
   return(USER_SUCCESS);
}

/*===========================================================================*/

/*===========================================================================*\
 * Free the user data structure
\*===========================================================================*/

int user_free_cg(void **user)
{
   cg_vrp_spec *vrp = (cg_vrp_spec *)(*user);

#if defined(CHECK_CUT_VALIDITY) && !defined(COMPILE_IN_TM)
   if (vrp->feas_sol_size)
      FREE(vrp->feas_sol);
#endif
#pragma omp master
   FREE(vrp->demand);
   FREE(vrp->edges);
   FREE(vrp->in_set);
   FREE(vrp->ref);
   FREE(vrp->new_demand);
   FREE(vrp->cut_val);
   FREE(vrp->cut_list);
   
   FREE(*user);

   return(USER_SUCCESS);
}

/*===========================================================================*/

#define SEND_DIR_SUBTOUR_CONSTRAINT(num_nodes, total_demand)                 \
new_cut->type = (num_nodes < vertnum/2 ?                                     \
		 SUBTOUR_ELIM_SIDE:SUBTOUR_ELIM_ACROSS);                     \
new_cut->rhs = (new_cut->type == SUBTOUR_ELIM_SIDE ?                         \
		RHS(num_nodes, total_demand, capacity) :                     \
		mult*BINS(total_demand, capacity));                          \
		num_cuts += cg_send_cut(new_cut);                            \

#define SEND_SUBTOUR_CONSTRAINT(num_nodes, total_demand)                     \
if (mult - 1 && num_nodes > 2){                                              \
   new_cut->type = (num_nodes < vertnum/2 ?                                  \
		    SUBTOUR_ELIM_SIDE:SUBTOUR_ELIM_ACROSS);                  \
   new_cut->rhs = (new_cut->type == SUBTOUR_ELIM_SIDE ?                      \
		   RHS(num_nodes, total_demand, capacity) :                  \
		   mult*BINS(total_demand, capacity));                       \
   num_cuts += cg_send_cut(new_cut);                                         \
}else{                                                                       \
   new_cut->type = SUBTOUR_ELIM_ACROSS;                                      \
   new_cut->rhs = mult*BINS(total_demand, capacity);                         \
   num_cuts += cg_send_cut(new_cut);                                         \
}                                                                            \

/*===========================================================================*/

/*===========================================================================*\
 * Find cuts violated by a particular LP solution. This is a fairly
 * involved function but the bottom line is that an LP solution comes in
 * and cuts go out.
\*===========================================================================*/

int user_find_cuts(void *user, int varnum, int iter_num, int level,
		   int index, double objval, int *indices, double *values,
		   double ub, double etol, int *cutnum)
{
   cg_vrp_spec *vrp = (cg_vrp_spec *)user;
   int vertnum = vrp->vertnum;
   network *n;
   vertex *verts = NULL;
   int *compdemands = NULL, *compnodes = NULL, *compnodes_copy = NULL;
   int *compmembers = NULL, comp_num = 0;
   double node_cut, max_node_cut, *compcuts = NULL;
   int rcnt, cur_bins = 0;
   char **coef_list;
   int i, k, max_node;
   int num_cuts = 0;
   double cur_slack = 0.0;
   int capacity = vrp->capacity;
   int cut_size = (vertnum >> DELETE_POWER) + 1;
   cut_data *new_cut = (cut_data *) calloc(1, sizeof(cut_data));
   elist *cur_edge = NULL;
   int which_connected_routine = vrp->par.which_connected_routine;
   int *ref = vrp->ref;
   double *cut_val = vrp->cut_val;
   char *in_set = vrp->in_set;
   char *cut_list = vrp->cut_list;

   elist *cur_edge1 = NULL, *cur_edge2 = NULL;
   int node1 = 0, node2 = 0;
   int *demand = vrp->demand;
   int *new_demand = vrp->new_demand;
   int total_demand = demand[0]; 
   int num_routes = vrp->numroutes, num_trials;
   int triangle_cuts = 0;
   char *coef;
   int mult;
   char prob_type = vrp->par.prob_type;
#ifdef ADD_FLOW_VARS
   int total_edgenum = vertnum*(vertnum - 1)/2, l, real_demand;
   double flow_value;
#ifndef DIRECTED_X_VARS
   int j;
   double flow_cap = capacity/2;
#endif
#endif
   edge *edge1;
#if defined(DIRECTED_X_VARS) && defined(ADD_FLOW_VARS)
   int h;
   char d_x_vars = TRUE;
#elif defined(ADD_FLOW_VARS)
   char d_x_vars = FALSE;
#endif
   
   if (iter_num == 1) srandom(1);

   if (prob_type == TSP || prob_type == VRP || prob_type == BPP){
      mult = 2;
   }else{
      mult = 1;
   }
      
   /* This creates a fractional graph representing the LP solution */
#ifdef ADD_FLOW_VARS
   for (i = 0; i < varnum && indices[i] < (1+d_x_vars)*total_edgenum; i++);
   n = create_flow_net(indices, values, varnum, etol, vrp->edges, demand,
		       vertnum);
#else
   n = create_net(indices, values, varnum, etol, vrp->edges, demand, vertnum);
#endif
   
   if (n->is_integral){
      /* if the network is integral, check for connectivity */
#ifdef ADD_FLOW_VARS      
      num_cuts = check_flow_connectivity(n, etol, capacity, num_routes, mult);
      if (!vrp->par.tau || num_cuts){
	 free_net(n);
	 *cutnum = num_cuts;
	 return(USER_SUCCESS);
      }
#else
      num_cuts = check_connectivity(n, etol, capacity, num_routes, mult);
      free_net(n);
      *cutnum = num_cuts;
      return(USER_SUCCESS);
#endif
   }

   verts = n->verts;

   /*First look for violated flow capacity constraints We are checking
     to see if there are any nonzero flow variables whose
     corresponding edge variable is zero. Recall, 'i' already equals the
     index of the first flow var*/
#if 1
#ifdef DIRECTED_X_VARS
   if (vrp->par.generate_x_cuts){
      new_cut->coef  = (char *) malloc(ISIZE);
      new_cut->name  = CUT__DO_NOT_SEND_TO_CP;
      for (i = 0, edge1 = n->edges; i < n->edgenum; i++, edge1++){
	 if (edge1->weight > 1 + etol){
	    new_cut->type  = X_CUT;
	    new_cut->size  = ISIZE;
	    new_cut->rhs   = 1.0;
	    ((int *)new_cut->coef)[0] = INDEX(edge1->v0, edge1->v1);
	    num_cuts += cg_send_cut(new_cut);
	 }
      }
      FREE(new_cut->coef);
   }      
#endif
   
#if defined(ADD_FLOW_VARS) && defined(DIRECTED_X_VARS) 
   if (vrp->par.generate_cap_cuts){
      new_cut->coef  = (char *) malloc(ISIZE);
      new_cut->name  = CUT__DO_NOT_SEND_TO_CP;
      for (i = 0, edge1 = n->edges; i < n->edgenum; i++, edge1++){
	 if ((flow_value = edge1->flow1) > etol){
	    real_demand = edge1->v0 ? demand[edge1->v0] : 0;
	    if ((capacity - real_demand)*edge1->weight1 < edge1->flow1 - etol){
	       new_cut->type  = FLOW_CAP;
	       new_cut->size  = ISIZE;
	       new_cut->rhs   = 0.0;
	       ((int *)new_cut->coef)[0] = INDEX(edge1->v0, edge1->v1);
	       num_cuts += cg_send_cut(new_cut);
	    }
	 }
	 if ((flow_value = edge1->flow2) > etol){
	    if ((capacity-demand[edge1->v1])*edge1->weight2<edge1->flow2-etol){
	       new_cut->type  = FLOW_CAP;
	       new_cut->size  = ISIZE;
	       new_cut->rhs   = 0.0;
	       ((int *)new_cut->coef)[0] = INDEX(edge1->v0, edge1->v1) +
		  total_edgenum;
	       num_cuts += cg_send_cut(new_cut);
	    }
	 }
      }
      FREE(new_cut->coef);
   }
#elif defined(ADD_FLOW_VARS)
   if (vrp->par.generate_cap_cuts){
      new_cut->coef  = (char *) malloc(ISIZE);
      new_cut->name  = CUT__DO_NOT_SEND_TO_CP;
      for (i = 0, edge1 = n->edges; i < n->edgenum; i++, edge1++){
	 if (flow_cap*edge1->weight < edge1->flow1 + edge1->flow2 - etol){
	    new_cut->type  = FLOW_CAP;
	    new_cut->size  = ISIZE;
	    new_cut->rhs   = 0.0;
	    ((int *)new_cut->coef)[0] = INDEX(edge1->v0, edge1->v1);
	    num_cuts += cg_send_cut(new_cut);
	 }
      }
      FREE(new_cut->coef);
   }
#endif   

   if (num_cuts){
      free_net(n);
      FREE(new_cut);
      *cutnum = num_cuts;
      return(USER_SUCCESS);
   }
   
#if defined(ADD_FLOW_VARS) && defined(DIRECTED_X_VARS) 
   if (vrp->par.generate_tight_cap_cuts){
      new_cut->coef  = (char *) malloc(ISIZE);
      new_cut->name  = CUT__DO_NOT_SEND_TO_CP;
      for (i = 0, edge1 = n->edges; i < n->edgenum; i++, edge1++){
	 if ((flow_value = edge1->flow1) > etol){
	    for (cur_edge = verts[edge1->v1].first; cur_edge;
		 cur_edge = cur_edge->next_edge){
	       if (cur_edge->other_end > edge1->v1){
		  flow_value -= cur_edge->data->flow1;
	       }else{
		  flow_value -= cur_edge->data->flow2;
	       }
	       if (flow_value < edge1->weight1*demand[edge1->v1]){
		  break;
	       }
	    }
	    if (flow_value > edge1->weight1*demand[edge1->v1] + etol){
	       new_cut->type  = TIGHT_FLOW;
	       new_cut->size  = ISIZE;
	       new_cut->rhs   = 0.0;
	       ((int *)new_cut->coef)[0] = INDEX(edge1->v0, edge1->v1);
	       num_cuts += cg_send_cut(new_cut);
	    }
	 }
	 if ((flow_value = edge1->flow2) > etol){
	    real_demand = edge1->v0 ? demand[edge1->v0] : 0;
	    for (cur_edge = verts[edge1->v0].first; cur_edge;
		 cur_edge = cur_edge->next_edge){
	       if (cur_edge->other_end > edge1->v0){
		  flow_value -= cur_edge->data->flow1;
	       }else{
		  flow_value -= cur_edge->data->flow2;
	       }
	       if (flow_value < edge1->weight2*real_demand){
		  break;
	       }
	    }
	    if (flow_value > edge1->weight2*real_demand + etol){
	       new_cut->type  = TIGHT_FLOW;
	       new_cut->size  = ISIZE;
	       new_cut->rhs   = 0.0;
	       ((int *)new_cut->coef)[0] = INDEX(edge1->v0, edge1->v1) +
		  total_edgenum;
	       num_cuts += cg_send_cut(new_cut);
	    }
	 }
      }
      FREE(new_cut->coef);
   }
#elif defined(ADD_FLOW_VARS)
   if (vrp->par.generate_tight_cap_cuts){
      new_cut->coef  = (char *)malloc(ISIZE);
      new_cut->name  = CUT__DO_NOT_SEND_TO_CP;
      for (i = 0, edge1 = n->edges; i < n->edgenum; i++, edge1++){
	 for (cur_edge = verts[edge1->v1].first; cur_edge;
	      cur_edge = cur_edge->next_edge){
	    if (cur_edge->other_end > edge1->v1){
	       flow_value -= cur_edge->data->flow1;
	    }else{
	       flow_value -= cur_edge->data->flow2;
	    }
	    if (flow_value < edge1->weight*demand[edge1->v1]){
	       break;
	    }
	 }
	 if (flow_value > edge1->weight*demand[edge1->v1] + etol){
	    new_cut->type  = TIGHT_FLOW;
	    new_cut->size  = ISIZE;
	    new_cut->rhs   = 0.0;
	    ((int *)new_cut->coef)[0] = INDEX(edge1->v0, edge1->v1);
	    num_cuts += cg_send_cut(new_cut);
	 }
	 real_demand = edge1->v0 ? demand[edge1->v0] : 0;
	 for (cur_edge = verts[edge1->v0].first; cur_edge;
	      cur_edge = cur_edge->next_edge){
	    if (cur_edge->other_end > edge1->v0){
	       flow_value -= cur_edge->data->flow1;
	    }else{
	       flow_value -= cur_edge->data->flow2;
	    }
	    if (flow_value < edge1->weight*real_demand){
	       break;
	    }
	 }
	 if (flow_value > edge1->weight*real_demand + etol){
	    new_cut->type  = TIGHT_FLOW;
	    new_cut->size  = ISIZE;
	    new_cut->rhs   = 0.0;
	    ((int *)new_cut->coef)[0] = INDEX(edge1->v0, edge1->v1) +
	       total_edgenum;
	    num_cuts += cg_send_cut(new_cut);
	 }
      }
      FREE(new_cut->coef);
   }
#endif   

   if (num_cuts){
      free_net(n);
      FREE(new_cut);
      *cutnum = num_cuts;
      return(USER_SUCCESS);
   }
#endif

#ifdef DO_TSP_CUTS
   if (vrp->par.which_tsp_cuts && prob_type == TSP){
      num_cuts += tsp_cuts(n, vrp->par.verbosity, TRUE,
			   vrp->par.which_tsp_cuts);
      free_net(n);
      *cutnum = num_cuts;
      FREE(new_cut);
      return(USER_SUCCESS);
   }      
#endif
   
   if (which_connected_routine == BOTH)
      which_connected_routine = CONNECTED;
   
   new_cut->size = cut_size;
   compnodes_copy = (int *) malloc(vertnum * sizeof(int));
   compmembers = (int *) malloc(vertnum * sizeof(int));
   
   do{
      compnodes = (int *) calloc(vertnum, sizeof(int));
      compdemands = (int *) calloc(vertnum, sizeof(int));
      compcuts = (double *) calloc(vertnum, sizeof(double));
      
      /*------------------------------------------------------------------*\
       * Get the connected components of the solution graph
      \*------------------------------------------------------------------*/
      rcnt = (which_connected_routine == BICONNECTED ?
	      biconnected(n, compnodes, compdemands, compcuts) :
	      connected(n, compnodes, compdemands, compmembers,
			compcuts, NULL));
      
      /* copy the arrays as they will be needed later */
      if (!which_connected_routine &&
	  vrp->par.do_greedy){
	 compnodes_copy = (int *) memcpy((char *)compnodes_copy, (char*)compnodes,
					 vertnum*sizeof(int));
	 n->compnodes = compnodes_copy;
	 comp_num = rcnt;
      }
      
      /*---------------------------------------------------------------*\
       * Check each component to see if it violates a capacity constraint
      \*---------------------------------------------------------------*/
      
      coef_list = (char **) calloc(rcnt, sizeof(char *));
      coef_list[0] = (char *) calloc(rcnt*cut_size, sizeof(char));
      for(i = 1; i<rcnt; i++)
	 coef_list[i] = coef_list[0]+i*cut_size;
      
      for(i = 1; i < vertnum; i++)
	 (coef_list[(verts[i].comp)-1][i >> DELETE_POWER]) |=
	    (1 << (i & DELETE_AND));
      
      for (i = 0; i < rcnt; i++){
	 if (compnodes[i+1] < 2) continue;
	 /*check ith component to see if it violates a constraint*/
	 if (vrp->par.which_connected_routine == BOTH &&
	     which_connected_routine == BICONNECTED && compcuts[i+1]==0)
	    continue;
	 if (compcuts[i+1] < mult*BINS(compdemands[i+1], capacity)-etol){
	    /*the constraint is violated so impose it*/
	    new_cut->coef = coef_list[i];
#if 0
	    new_cut->type = SUBTOUR_ELIM_SIDE;
	    new_cut->rhs = RHS(compnodes[i+1],compdemands[i+1], capacity);
	    num_cuts += cg_send_cut(new_cut);
#endif
#ifdef DIRECTED_X_VARS
	    SEND_DIR_SUBTOUR_CONSTRAINT(compnodes[i+1], compdemands[i+1]);
#else
	    SEND_SUBTOUR_CONSTRAINT(compnodes[i+1], compdemands[i+1]);
#endif
	 }else{/*if the constraint is not violated, then try generating a
		violated constraint by deleting customers that don't
		change the number of trucks required by the customers in
		the component but decrease the value of the cut*/
	    cur_bins = BINS(compdemands[i+1], capacity);/*the current
							  number of trucks
							  required*/
	    cur_slack = compcuts[i+1] - mult*cur_bins;/*current slack in the
							constraint*/
	    while (compnodes[i+1]){/*while there are still nodes in the
				     component*/
	       for (max_node = 0, max_node_cut = 0, k = 1;
		    k < vertnum; k++){
		  if (verts[k].comp == i+1){
		     if (BINS(compdemands[i+1]-verts[k].demand, capacity)
			 == cur_bins){
			/*if the number of trucks doesn't decrease upon
			  deleting this customer*/
			for (node_cut = 0, cur_edge = verts[k].first;
			     cur_edge; cur_edge = cur_edge->next_edge){
			   node_cut += (cur_edge->other_end ?
					-cur_edge->data->weight :
					cur_edge->data->weight);
			}
			if (node_cut > max_node_cut){/*check whether the
						       value of the cut
						       decrease is the best
						       seen so far*/
			   max_node = k;
			   max_node_cut = node_cut;
			}
		     }
		  }
	       }
	       if (!max_node){
		  break;
	       }
	       /*delete the customer that exhibited the greatest
		 decrease in cut value*/
	       compnodes[i+1]--;
	       compdemands[i+1] -= verts[max_node].demand;
	       compcuts[i+1] -= max_node_cut;
	       cur_slack -= max_node_cut;
	       verts[max_node].comp = 0;
	       coef_list[i][max_node >> DELETE_POWER] ^=
		  (1 << (max_node & DELETE_AND));
	       if (cur_slack < 0){/*if the cut is now violated, impose
				    it*/
		  new_cut->coef = coef_list[i];
#if 0
		  new_cut->type = SUBTOUR_ELIM_SIDE;
		  new_cut->rhs = RHS(compnodes[i+1],compdemands[i+1],capacity);
		  new_cut->size = cut_size;
		  num_cuts += cg_send_cut(new_cut);
#endif
#ifdef DIRECTED_X_VARS
		  SEND_DIR_SUBTOUR_CONSTRAINT(compnodes[i+1],compdemands[i+1]);
#else
		  SEND_SUBTOUR_CONSTRAINT(compnodes[i+1], compdemands[i+1]);
#endif
		  break;
	       }
	    }
	 }
      }
      FREE(coef_list[0]);
      FREE(coef_list);
      which_connected_routine++;
      FREE(compnodes);
      FREE(compdemands);
      FREE(compcuts);
   }while((!num_cuts || vrp->par.which_connected_routine == BOTH)
	  && which_connected_routine < 2);

#if 0
   if (num_cuts < 10 && vrp->par.do_mincut){
      num_cuts += min_cut(vrp, n, etol, mult);
      free_net(n);
   }
   
   if (!vrp->par.do_greedy || num_cuts >= 10){
      *cutnum = num_cuts;
      FREE(new_cut);
      return(USER_SUCCESS);
   }
   
#ifdef ADD_FLOW_VARS
   for (i = 0; i < varnum && indices[i] < (1+d_x_vars)*total_edgenum; i++);
   n = create_flow_net(indices, values, varnum, etol, vrp->edges, demand,
		       vertnum);
#else
   n = create_net(indices, values, varnum, etol, vrp->edges, demand, vertnum);
#endif
#endif

   if (num_cuts < 10 && vrp->par.do_greedy && (prob_type == VRP ||
			prob_type == TSP || prob_type == BPP)){
      coef = (char *) malloc(cut_size * sizeof(char)); 
      for (cur_edge=verts[0].first; cur_edge; cur_edge=cur_edge->next_edge){
	 for (cur_edge1 = cur_edge->other->first; cur_edge1;
	      cur_edge1 = cur_edge1->next_edge){
	    if (cur_edge1->data->weight + cur_edge->data->weight < 1 - etol)
	       continue; 
	    node1 = cur_edge->other_end; 
	    node2 = cur_edge1->other_end;
	    for (cur_edge2 = verts[node2].first; cur_edge2;
		 cur_edge2 = cur_edge2->next_edge){
	       if (!(cur_edge2->other_end) && node2){
		  if ((BINS(total_demand - demand[node1] - demand[node2],
			    capacity) > num_routes -1) &&
		      (cur_edge1->data->weight + cur_edge->data->weight +
		       cur_edge2->data->weight>2+etol)){
		     new_cut->type = SUBTOUR_ELIM_ACROSS;
		     new_cut->size =cut_size;
		     new_cut->rhs =mult*BINS(total_demand - demand[node1] -
					demand[node2],capacity);
		     memset(coef, 0, cut_size);
		     for (i = 1; i <vertnum ; i++)
			if ((i != node1) && (i != node2))
			   (coef[i >> DELETE_POWER]) |= (1 << (i&DELETE_AND));
		     new_cut->coef =coef;
		     triangle_cuts += cg_send_cut(new_cut);
		  }
		  break; 
	       }
	    }
	 }
      }
      FREE(coef);
      if (vrp->par.verbosity > 2)
	 printf("Found %d triangle cuts\n",triangle_cuts);
      num_cuts += triangle_cuts;
   }
   
   if (num_cuts < 10 && vrp->par.do_greedy){
      memcpy((char *)new_demand, (char *)demand, vertnum*ISIZE);
#ifndef DIRECTED_X_VARS
      if (mult == 2){
	 /*We can only do this for VRP and TSP problems without directed X
	   vars*/
#else
      {
#endif
	 num_cuts +=
	    reduce_graph(n, etol, new_demand, vrp->capacity, mult, new_cut);
      }
      if (comp_num > 1){
	 num_cuts += greedy_shrinking1(n, capacity, etol,
				       vrp->par.max_num_cuts_in_shrink,
				       new_cut, compnodes_copy, compmembers,
				       comp_num, in_set, cut_val,
				       ref, cut_list, new_demand, mult);
      }else{
	 num_cuts += greedy_shrinking1_one(n, capacity, etol,
					   vrp->par.max_num_cuts_in_shrink,
					   new_cut, in_set, cut_val, cut_list,
					   num_routes, new_demand, mult);
      }
      if (num_cuts && vrp->par.verbosity >2 )
	 printf("Shrink1 found %d cuts \n", num_cuts);
   }
   
   if (num_cuts < 10 && vrp->par.do_greedy){
      if (vrp->par.do_extra_in_root)
	 num_trials = level ? vrp->par.greedy_num_trials :
	 2 * vrp->par.greedy_num_trials;
      else
	 num_trials = vrp->par.greedy_num_trials;
      if (comp_num){
	 num_cuts += greedy_shrinking6(n, capacity, etol, new_cut,
				       compnodes_copy, compmembers, comp_num,
				       in_set, cut_val, ref, cut_list,
				       vrp->par.max_num_cuts_in_shrink,
				       new_demand, num_cuts ? num_trials :
				       2 * num_trials, 10.5, mult);
      }else{
	 num_cuts += greedy_shrinking6_one(n, capacity, etol, new_cut, in_set,
					   cut_val, num_routes, cut_list,
					   vrp->par.max_num_cuts_in_shrink,
					   new_demand, num_cuts ? num_trials :
					   2 * num_trials, 10.5, mult); 
      }
      if (num_cuts && vrp->par.verbosity >2)
	 printf("Shrink6 found %d cuts \n", num_cuts);
   }
   
#ifdef DO_TSP_CUTS
   if (!num_cuts && vrp->par.which_tsp_cuts){
      num_cuts += tsp_cuts(n, vrp->par.verbosity, FALSE,
			   vrp->par.which_tsp_cuts);
   }
#endif

   FREE(compmembers);
   FREE(compnodes_copy);

   /*Now look for violated flow capacity constraints We are checking
     to see if there are any nonzero flow variables whose
     corresponding edge variable is zero. Recall, 'i' already equals the
     index of the first flow var*/
   
#if 0
#ifdef DIRECTED_X_VARS
   if (vrp->par.generate_x_cuts){
      new_cut->coef  = (char *) malloc(ISIZE);
      new_cut->name  = CUT__DO_NOT_SEND_TO_CP;
      for (i = 0, edge1 = n->edges; i < n->edgenum; i++, edge1++){
	 if (edge1->weight > 1 + etol){
	    new_cut->type  = X_CUT;
	    new_cut->size  = ISIZE;
	    new_cut->rhs   = 1.0;
	    ((int *)new_cut->coef)[0] = INDEX(edge1->v0, edge1->v1);
	    num_cuts += cg_send_cut(new_cut);
	 }
      }
      FREE(new_cut->coef);
   }      
#endif
   
#if defined(ADD_FLOW_VARS) && defined(DIRECTED_X_VARS) 
   if (vrp->par.generate_cap_cuts){
      new_cut->coef  = (char *) malloc(ISIZE);
      new_cut->name  = CUT__DO_NOT_SEND_TO_CP;
      for (i = 0, edge1 = n->edges; i < n->edgenum; i++, edge1++){
	 if ((flow_value = edge1->flow1) > etol){
	    real_demand = edge1->v0 ? demand[edge1->v0] : 0;
	    if ((capacity - real_demand)*edge1->weight1 < edge1->flow1 - etol){
	       new_cut->type  = FLOW_CAP;
	       new_cut->size  = ISIZE;
	       new_cut->rhs   = 0.0;
	       ((int *)new_cut->coef)[0] = INDEX(edge1->v0, edge1->v1);
	       num_cuts += cg_send_cut(new_cut);
	    }
	 }
	 if ((flow_value = edge1->flow2) > etol){
	    if ((capacity-demand[edge1->v1])*edge1->weight2<edge1->flow2-etol){
	       new_cut->type  = FLOW_CAP;
	       new_cut->size  = ISIZE;
	       new_cut->rhs   = 0.0;
	       ((int *)new_cut->coef)[0] = INDEX(edge1->v0, edge1->v1) +
		  total_edgenum;
	       num_cuts += cg_send_cut(new_cut);
	    }
	 }
      }
      FREE(new_cut->coef);
   }
#elif defined(ADD_FLOW_VARS)
   if (vrp->par.generate_cap_cuts){
      new_cut->coef  = (char *) malloc(ISIZE);
      new_cut->name  = CUT__DO_NOT_SEND_TO_CP;
      for (i = 0, edge1 = n->edges; i < n->edgenum; i++, edge1++){
	 if (flow_cap*edge1->weight < edge1->flow1 + edge1->flow2 - etol){
	    new_cut->type  = FLOW_CAP;
	    new_cut->size  = ISIZE;
	    new_cut->rhs   = 0.0;
	    ((int *)new_cut->coef)[0] = INDEX(edge1->v0, edge1->v1);
	    num_cuts += cg_send_cut(new_cut);
	 }
      }
      FREE(new_cut->coef);
   }
#endif   

   if (num_cuts){
      free_net(n);
      FREE(new_cut);
      *cutnum = num_cuts;
      return(USER_SUCCESS);
   }
   
#if defined(ADD_FLOW_VARS) && defined(DIRECTED_X_VARS) 
   if (vrp->par.generate_tight_cap_cuts){
      new_cut->coef  = (char *) malloc(ISIZE);
      new_cut->name  = CUT__DO_NOT_SEND_TO_CP;
      for (i = 0, edge1 = n->edges; i < n->edgenum; i++, edge1++){
	 if ((flow_value = edge1->flow1) > etol){
	    for (cur_edge = verts[edge1->v1].first; cur_edge;
		 cur_edge = cur_edge->next_edge){
	       if (cur_edge->other_end > edge1->v1){
		  flow_value -= cur_edge->data->flow1;
	       }else{
		  flow_value -= cur_edge->data->flow2;
	       }
	       if (flow_value < edge1->weight1*demand[edge1->v1]){
		  break;
	       }
	    }
	    if (flow_value > edge1->weight1*demand[edge1->v1] + etol){
	       new_cut->type  = TIGHT_FLOW;
	       new_cut->size  = ISIZE;
	       new_cut->rhs   = 0.0;
	       ((int *)new_cut->coef)[0] = INDEX(edge1->v0, edge1->v1);
	       num_cuts += cg_send_cut(new_cut);
	    }
	 }
	 if ((flow_value = edge1->flow2) > etol){
	    real_demand = edge1->v0 ? demand[edge1->v0] : 0;
	    for (cur_edge = verts[edge1->v0].first; cur_edge;
		 cur_edge = cur_edge->next_edge){
	       if (cur_edge->other_end > edge1->v0){
		  flow_value -= cur_edge->data->flow1;
	       }else{
		  flow_value -= cur_edge->data->flow2;
	       }
	       if (flow_value < edge1->weight2*real_demand){
		  break;
	       }
	    }
	    if (flow_value > edge1->weight2*real_demand + etol){
	       new_cut->type  = TIGHT_FLOW;
	       new_cut->size  = ISIZE;
	       new_cut->rhs   = 0.0;
	       ((int *)new_cut->coef)[0] = INDEX(edge1->v0, edge1->v1) +
		  total_edgenum;
	       num_cuts += cg_send_cut(new_cut);
	    }
	 }
      }
      FREE(new_cut->coef);
   }
#elif defined(ADD_FLOW_VARS)
   if (vrp->par.generate_tight_cap_cuts){
      new_cut->coef  = (char *) malloc(ISIZE);
      new_cut->name  = CUT__DO_NOT_SEND_TO_CP;
      for (i = 0, edge1 = n->edges; i < n->edgenum; i++, edge1++){
	 for (cur_edge = verts[edge1->v1].first; cur_edge;
	      cur_edge = cur_edge->next_edge){
	    if (cur_edge->other_end > edge1->v1){
	       flow_value -= cur_edge->data->flow1;
	    }else{
	       flow_value -= cur_edge->data->flow2;
	    }
	    if (flow_value < edge1->weight*demand[edge1->v1]){
	       break;
	    }
	 }
	 if (flow_value > edge1->weight*demand[edge1->v1] + etol){
	    new_cut->type  = TIGHT_FLOW;
	    new_cut->size  = ISIZE;
	    new_cut->rhs   = 0.0;
	    ((int *)new_cut->coef)[0] = INDEX(edge1->v0, edge1->v1);
	    num_cuts += cg_send_cut(new_cut);
	 }
	 real_demand = edge1->v0 ? demand[edge1->v0] : 0;
	 for (cur_edge = verts[edge1->v0].first; cur_edge;
	      cur_edge = cur_edge->next_edge){
	    if (cur_edge->other_end > edge1->v0){
	       flow_value -= cur_edge->data->flow1;
	    }else{
	       flow_value -= cur_edge->data->flow2;
	    }
	    if (flow_value < edge1->weight*real_demand){
	       break;
	    }
	 }
	 if (flow_value > edge1->weight*real_demand + etol){
	    new_cut->type  = TIGHT_FLOW;
	    new_cut->size  = ISIZE;
	    new_cut->rhs   = 0.0;
	    ((int *)new_cut->coef)[0] = INDEX(edge1->v0, edge1->v1) +
	       total_edgenum;
	    num_cuts += cg_send_cut(new_cut);
	 }
      }
      FREE(new_cut->coef);
   }
#endif   
#endif
   
   FREE(new_cut);
   free_net(n);
   *cutnum = num_cuts;
   return(USER_SUCCESS);
}

/*===========================================================================*/

/*===========================================================================*\
 * This routine takes a solution which is integral and checkes whether it is
 * feasible by first checking if it is connected and then checking to make
 * sure each route obeys the capacity constraints.
\*===========================================================================*/

int check_connectivity(network *n, double etol, int capacity, int numroutes,
		       char mult)
{
  vertex *verts;
  elist *cur_route_start;
  int weight = 0, reduced_weight, *compdemands, *route;
  edge *edge_data;
  int cur_vert = 0, prev_vert, cust_num = 0, cur_route, rcnt, *compnodes;
  cut_data *new_cut;
  char **coef_list, *coef;
  int num_cuts = 0, i, reduced_cust_num;
  int vertnum = n->vertnum, vert1, vert2;
  int cut_size = (vertnum >> DELETE_POWER) +1;
  double *compcuts;
  
  if (!n->is_integral) return(NOT_INTEGRAL);

  verts = n->verts;
  compnodes = (int *) calloc(vertnum + 1, sizeof(int));
  compdemands = (int *) calloc(vertnum + 1, sizeof(int));
  compcuts = (double *) calloc(vertnum + 1, sizeof(double));
  /*get the components of the solution graph without the depot to check if the
    graph is connected or not*/
  rcnt = connected(n, compnodes, compdemands, NULL, compcuts, NULL);
  coef_list = (char **) calloc(rcnt, sizeof(char *));
  coef_list[0] = (char *) calloc(rcnt*cut_size, sizeof(char));
  for(i = 1; i<rcnt; i++)
     coef_list[i] = coef_list[0]+i*cut_size;

  for(i = 1; i < vertnum; i++)
    (coef_list[(verts[i].comp)-1][i >> DELETE_POWER]) |=
      (1 << (i & DELETE_AND));
  
  /*-------------------------------------------------------------------------*\
  | For each component check to see if the cut it induces is nonzero -- each  |
  | component's cut value must be either 0, 1 or 2 since we have integrality  |
  \*-------------------------------------------------------------------------*/
  
  new_cut = (cut_data *) calloc(1, sizeof(cut_data));
  new_cut->size = cut_size;
  for (i = 0; i<rcnt; i++){
    if (compcuts[i+1] < etol ||
	(mult == 1 && compdemands[i+1] > capacity)){
       /*if the cut value is zero, the graph is
	 disconnected and we have a violated cut*/
      new_cut->coef = coef_list[i];
#if 0
      new_cut->type = SUBTOUR_ELIM_SIDE;
      new_cut->rhs = RHS(compnodes[i+1], compdemands[i+1], capacity);
      num_cuts += cg_send_cut(new_cut);
#endif
#ifdef DIRECTED_X_VARS
      SEND_DIR_SUBTOUR_CONSTRAINT(compnodes[i+1], compdemands[i+1]);
#else
      SEND_SUBTOUR_CONSTRAINT(compnodes[i+1], compdemands[i+1]);
#endif
    }
  }

  FREE(coef_list[0]);
  FREE(coef_list);
  FREE(compnodes);
  FREE(compdemands);
  FREE(compcuts);

  if (mult == 1)
     /*This is a tree problem*/
     return(num_cuts);
  
  /*-------------------------------------------------------------------------*\
  | if the graph is connected, check each route to see if it obeys the        |
  | capacity constraints                                                      |
  \*-------------------------------------------------------------------------*/

  route = (int *) malloc(vertnum*ISIZE);
  for (cur_route_start = verts[0].first, cur_route = 0,
       edge_data = cur_route_start->data; cur_route < numroutes;
       cur_route++){
    edge_data = cur_route_start->data;
    edge_data->scanned = TRUE;
    cur_vert = edge_data->v1;
    prev_vert = weight = cust_num = 0;

    coef = new_cut->coef = (char *) calloc(cut_size, sizeof(char));

    route[0] = cur_vert;
    while (cur_vert){
                    /*keep tracing around the route and whenever the addition
		       of the next customer causes a violation, impose the
		       constraint induced
		       by the set of customers seen so far on the route*/
      coef[cur_vert >> DELETE_POWER]|=(1 << (cur_vert & DELETE_AND));
      cust_num++;
      if ((weight += verts[cur_vert].demand) > capacity){
#if 0
	new_cut->type = SUBTOUR_ELIM_SIDE;
	new_cut->rhs = RHS(cust_num, weight, capacity);
	num_cuts += cg_send_cut(new_cut);
#endif
#ifdef DIRECTED_X_VARS
	SEND_DIR_SUBTOUR_CONSTRAINT(cust_num, weight);
#else
	SEND_SUBTOUR_CONSTRAINT(cust_num, weight);
#endif
	vert1 = route[0];
	reduced_weight = weight;
	reduced_cust_num = cust_num;
	while (TRUE){
	  if ((reduced_weight -= verts[vert1].demand) > capacity){
	     reduced_cust_num--;
	     coef[vert1 >> DELETE_POWER] &= ~(1 << (vert1 & DELETE_AND));
#if 0
	     new_cut->type = SUBTOUR_ELIM_SIDE;
	     new_cut->rhs = RHS(reduced_cust_num, reduced_weight, capacity);
	     num_cuts += cg_send_cut(new_cut);
#endif
#ifdef DIRECTED_X_VARS
	     SEND_DIR_SUBTOUR_CONSTRAINT(reduced_cust_num, reduced_weight);
#else
	     SEND_SUBTOUR_CONSTRAINT(reduced_cust_num, reduced_weight);
#endif
	     vert1 = route[vert1];
	  }else{
	     break;
	  }
	}
	vert2 = route[0];
	while (vert2 != vert1){
	  coef[vert2 >> DELETE_POWER] |= (1 << (vert2 & DELETE_AND));
	  vert2 = route[vert2];
	}
      }
      if (verts[cur_vert].first->other_end != prev_vert){
	prev_vert = cur_vert;
	edge_data = verts[cur_vert].first->data;
	cur_vert = verts[cur_vert].first->other_end;
      }
      else{
	prev_vert = cur_vert;
	edge_data = verts[cur_vert].last->data; /*This statement could
						  possibly be taken out to
						  speed things up a bit*/
	cur_vert = verts[cur_vert].last->other_end;
      }
      route[prev_vert] = cur_vert;
    }
    edge_data->scanned = TRUE;

    FREE(coef);
    
    while (cur_route_start->data->scanned){/*find the next edge leading out of
					     the depot which has not yet been
					     traversed to start the next
					     route*/
      if (!(cur_route_start = cur_route_start->next_edge)) break;
    }
  }
  FREE(route);
  FREE(new_cut);
  
  for (cur_route_start = verts[0].first; cur_route_start;
       cur_route_start = cur_route_start->next_edge)
    cur_route_start->data->scanned = FALSE;
  
  return(num_cuts);
}

/*===========================================================================*/

int check_flow_connectivity(network *n, double etol, int capacity,
			    int numroutes, char mult)
{
  vertex *verts;
  elist *cur_route_start;
  int weight = 0, reduced_weight, *compdemands, *route;
  edge *edge_data;
  int cur_vert = 0, prev_vert, cust_num = 0, cur_route, rcnt, *compnodes;
  cut_data *new_cut;
  char **coef_list, *coef;
  int num_cuts = 0, i, reduced_cust_num;
  int vertnum = n->vertnum, vert1, vert2;
  int cut_size = (vertnum >> DELETE_POWER) +1;
  double *compcuts;
  
  if (!n->is_integral) return(NOT_INTEGRAL);

  verts = n->verts;
  compnodes = (int *) calloc(vertnum + 1, sizeof(int));
  compdemands = (int *) calloc(vertnum + 1, sizeof(int));
  compcuts = (double *) calloc(vertnum + 1, sizeof(double));
  /*get the components of the solution graph without the depot to check if the
    graph is connected or not*/
  rcnt = flow_connected(n, compnodes, compdemands, NULL, compcuts, NULL, etol);
  coef_list = (char **) calloc(rcnt, sizeof(char *));
  coef_list[0] = (char *) calloc(rcnt*cut_size, sizeof(char));
  for(i = 1; i<rcnt; i++)
     coef_list[i] = coef_list[0]+i*cut_size;

  for(i = 1; i < vertnum; i++)
    (coef_list[(verts[i].comp)-1][i >> DELETE_POWER]) |=
      (1 << (i & DELETE_AND));
  
  /*-------------------------------------------------------------------------*\
  | For each component check to see if the cut it induces is nonzero -- each  |
  | component's cut value must be either 0, 1 or 2 since we have integrality  |
  \*-------------------------------------------------------------------------*/
  
  new_cut = (cut_data *) calloc(1, sizeof(cut_data));
  new_cut->size = cut_size;
  for (i = 0; i<rcnt; i++){
    if (compcuts[i+1] < etol ||
	(mult == 1 && compdemands[i+1] > capacity)){
       /*if the cut value is zero, the graph is
	 disconnected and we have a violated cut*/
      new_cut->coef = coef_list[i];
#if 0
      if (compnodes[i+1] > 2){
	 new_cut->type = SUBTOUR_ELIM_SIDE;
	 new_cut->rhs = RHS(compnodes[i+1], compdemands[i+1], capacity);
      }else{
	 new_cut->type = SUBTOUR_ELIM_ACROSS;
	 new_cut->rhs = mult * BINS(compdemands[i+1], capacity);
      }
      num_cuts += cg_send_cut(new_cut);
#endif
#ifdef DIRECTED_X_VARS
      SEND_DIR_SUBTOUR_CONSTRAINT(compnodes[i+1], compdemands[i+1]);
#else
      SEND_SUBTOUR_CONSTRAINT(compnodes[i+1], compdemands[i+1]);
#endif
    }
  }

  FREE(coef_list[0]);
  FREE(coef_list);
  FREE(compnodes);
  FREE(compdemands);
  FREE(compcuts);

  if (mult == 1)
     return(num_cuts);
  
  /*-------------------------------------------------------------------------*\
  | if the graph is connected, check each route to see if it obeys the        |
  | capacity constraints                                                      |
  \*-------------------------------------------------------------------------*/

  route = (int *) malloc(vertnum*ISIZE);
  for (cur_route_start = verts[0].first, cur_route = 0,
       edge_data = cur_route_start->data; cur_route < numroutes;
       cur_route++){
    edge_data = cur_route_start->data;
    edge_data->scanned = TRUE;
    cur_vert = edge_data->v1;
    prev_vert = weight = cust_num = 0;

    coef = new_cut->coef = (char *) calloc(cut_size, sizeof(char));

    route[0] = cur_vert;
    while (cur_vert){
                    /*keep tracing around the route and whenever the addition
		       of the next customer causes a violation, impose the
		       constraint induced
		       by the set of customers seen so far on the route*/
      coef[cur_vert >> DELETE_POWER]|=(1 << (cur_vert & DELETE_AND));
      cust_num++;
      if ((weight += verts[cur_vert].demand) > capacity){
#if 0
	new_cut->type = SUBTOUR_ELIM_SIDE;
	new_cut->rhs = RHS(cust_num, weight, capacity);
	num_cuts += cg_send_cut(new_cut);
#endif
#ifdef DIRECTED_X_VARS
	SEND_DIR_SUBTOUR_CONSTRAINT(cust_num, weight);
#else
	SEND_SUBTOUR_CONSTRAINT(cust_num, weight);
#endif
	vert1 = route[0];
	reduced_weight = weight;
	reduced_cust_num = cust_num;
	while (TRUE){
	  if ((reduced_weight -= verts[vert1].demand) > capacity){
	     reduced_cust_num--;
	     coef[vert1 >> DELETE_POWER] &= ~(1 << (vert1 & DELETE_AND));
#if 0
	     new_cut->type = SUBTOUR_ELIM_SIDE;
	     new_cut->rhs = RHS(reduced_cust_num, reduced_weight, capacity);
	     num_cuts += cg_send_cut(new_cut);
#endif
#ifdef DIRECTED_X_VARS
	     SEND_DIR_SUBTOUR_CONSTRAINT(reduced_cust_num, reduced_weight);
#else
	     SEND_SUBTOUR_CONSTRAINT(reduced_cust_num, reduced_weight);
#endif
	     vert1 = route[vert1];
	  }else{
	     break;
	  }
	}
	vert2 = route[0];
	while (vert2 != vert1){
	  coef[vert2 >> DELETE_POWER] |= (1 << (vert2 & DELETE_AND));
	  vert2 = route[vert2];
	}
      }
      if (verts[cur_vert].first->other_end != prev_vert ||
	  verts[cur_vert].first->data->weight > 1.0){
	prev_vert = cur_vert;
	edge_data = verts[cur_vert].first->data;
	cur_vert = verts[cur_vert].first->other_end;
      }else{
	prev_vert = cur_vert;
	edge_data = verts[cur_vert].first->next_edge->data; /*This statement
							      could
						  possibly be taken out to
						  speed things up a bit*/
	cur_vert = verts[cur_vert].first->next_edge->other_end;
      }
      route[prev_vert] = cur_vert;
    }
    edge_data->scanned = TRUE;

    FREE(new_cut->coef);
    
    while (cur_route_start->data->scanned &&
	   cur_route_start->data->weight >= 1.0){/*find the next edge
						   leading out of the
						   depot which has not
						   yet been traversed
						   to start the next
						   route*/
      if (!(cur_route_start = cur_route_start->next_edge))
	 break; 
    }
  }
  FREE(route);
  FREE(new_cut);
  
  for (cur_route_start = verts[0].first; cur_route_start;
       cur_route_start = cur_route_start->next_edge)
    cur_route_start->data->scanned = FALSE;
  
  return(num_cuts);
}

/*===========================================================================*/

/*===========================================================================*\
 * This is an undocumented (for now) debugging feature which can allow the user
 * to identify the cut which cuts off a particular known feasible solution.
\*===========================================================================*/

#ifdef CHECK_CUT_VALIDITY

int user_check_validity_of_cut(void *user, cut_data *new_cut)
{
   cg_vrp_spec *vrp = (cg_vrp_spec *)user;
   int *edges = vrp->edges;
   int *feas_sol = vrp->feas_sol;
   double lhs = 0;
   char *coef;
   int v0, v1;
   int i;
   
   
   if (vrp->feas_sol_size){
      switch (new_cut->type){
	 
	 /*------------------------------------------------------------------*\
	  * The subtour elimination constraints are stored as a vector of bits
	  * indicating which side of the cut each customer is on.
	  \*-----------------------------------------------------------------*/
	 
       case SUBTOUR_ELIM_SIDE:
	 /*Here, I could just allocate enough memory up front and then
	   reallocate at the end istead of counting the number of entries in
	   the row first*/
	 coef = new_cut->coef;
	 for (i = 0; i<vrp->feas_sol_size; i++){
	    v0 = edges[feas_sol[i] << 1];
	    v1 = edges[(feas_sol[i] << 1) + 1];
	    if ((coef[v0 >> DELETE_POWER] & (1 << (v0 & DELETE_AND))) &&
		(coef[v1 >> DELETE_POWER] & (1 << (v1 & DELETE_AND)))){
	       lhs += 1;
	    }
	 }
	 new_cut->sense = 'L';
	 break;
	 
       case SUBTOUR_ELIM_ACROSS:
	 /*I could just allocate enough memory up front and then reallocate
	   at the end instead of counting the number of entries first*/
	 coef = new_cut->coef;
	 for (i = 0; i < vrp->feas_sol_size; i++){
	    v0 = edges[feas_sol[i] << 1];
	    v1 = edges[(feas_sol[i] << 1) + 1];
	    if ((coef[v0 >> DELETE_POWER] >> (v0 & DELETE_AND) & 1) ^
		(coef[v1 >> DELETE_POWER] >> (v1 & DELETE_AND) & 1)){
	       lhs += 1;
	    }
	 }
	 new_cut->sense = 'G';
	 break;
	 
       default:
	 printf("Unrecognized cut type!\n");
      }
      
      /*check to see if the cut is actually violated by the current solution --
	otherwise don't add it -- also check to see if its a duplicate*/
      if (new_cut->sense == 'G' ? lhs < new_cut->rhs : lhs > new_cut->rhs){
	 printf("CG: ERROR -- cut is violated by feasible solution!!!\n");
#if 0
	 printf("Solution number: %i level: %i iter_num: %i\n",
		cg->cur_sol.xindex, cg->cur_sol.xlevel, cg->cur_sol.xiter_num);
#endif
	 sleep(600);
	 exit(1);
      }
   }
   
   return(USER_SUCCESS);
}
#endif
