#include <malloc.h>
#include <stdlib.h>

#include "BB_constants.h"
#include "BB_macros.h"
#include "tsp_ins_rout.h"
#include "s_path.h"
#include "heur_routines.h"
#include "timemeas.h"
#include "messages.h"
#include "proccomm.h"
#include "vrp_const.h"
#include "compute_cost.h"
#include "qsort.h"

static int compar(const void *elem1, const void *elem2)
{
   return(((neighbor *)elem1)->cost - ((neighbor *)elem2)->cost);
}

/*===========================================================================*/

void  main(void)
{
   heur_prob *p;
   int mytid, info, r_bufid, parent;
   int starter, farnode, v0, v1, cur_start;
   _node *tsp_tour, *tour, *opt_tour;
   int maxdist;
   int *intour;
   int last, cost;
   int i, j;
   neighbor *nbtree;
   int trials, interval;
   int farside, vertnum;
   best_tours *opt_tours, *tours;
   double t;

   (void) used_time(&t);
	
   mytid = pvm_mytid();
	
   p = (heur_prob *) calloc ((int)1, sizeof(heur_prob));
	
   /*-----------------------------------------------------------------------*\
   |                     Receive the VRP data                                |
   \*-----------------------------------------------------------------------*/

   parent = receive(p);

   PVM_FUNC(r_bufid, pvm_recv(-1, TSP_TRIALS));
   PVM_FUNC(info, pvm_upkint(&trials, 1, 1));

   PVM_FUNC(r_bufid, pvm_recv(parent, VRP_DATA));
   PVM_FUNC(info, pvm_upkint(&farside, 1, 1));
	
   /*-----------------------------------------------------------------------*\
   |                     Receive the starting point                          |
   \*-----------------------------------------------------------------------*/
   PVM_FUNC(r_bufid, pvm_recv(-1, HEUR_START_POINT));
   PVM_FUNC(info, pvm_upkint(&starter, 1, 1));
   vertnum = p->vertnum;
	
   if (starter == vertnum)
      for (starter=v0=1, maxdist=ICOST(&p->dist, 0,1); v0<vertnum-1; v0++)
	 for (v1=v0+1; v1<vertnum; v1++)
	    if (maxdist < ICOST(&p->dist, v0, v1)){
	       maxdist = ICOST(&p->dist, v0, v1);
	       starter = v0;
	    }
	
   /*-----------------------------------------------------------------------*\
   |                     Allocate arrays                                     |
   \*-----------------------------------------------------------------------*/
   tsp_tour   = (_node *)  malloc (vertnum * sizeof(_node));
   nbtree     = (neighbor *) malloc (vertnum * sizeof(neighbor));
   intour     = (int *)      calloc (vertnum, sizeof(int));
   tours      = p->cur_tour = (best_tours *) calloc (1, sizeof(best_tours));
   tour       = p->cur_tour->tour = (_node *) calloc (vertnum, sizeof(_node));
   opt_tours = (best_tours *) malloc (sizeof(best_tours));
   opt_tour  = (_node *) malloc (vertnum*sizeof(_node));

  /*------------------------------------------------------------------------*\
  | This heuristic is a so-called route-first, cluster-second heuristic.     |
  | We first construct a TSP route by farnear insert and then partition it   |
  | into feasible routes by finding a shortest cycle of a graph with edge    |
  | costs bewtween nodes being defined to be the cost of a route from one    |
  | endpoint of the edge to the other.                                       |
  \*------------------------------------------------------------------------*/
	
   /*-----------------------------------------------------------------------*\
   |   Find the first 'farside node with farthest insertion from 'starter'   |
   \*-----------------------------------------------------------------------*/
   last = 0;
   intour[0] = IN_TOUR;
   intour[starter] = IN_TOUR;
   fi_insert_edges(p, starter, nbtree, intour, &last);
   farnode = farthest(nbtree, intour, &last);
   intour[farnode] = IN_TOUR;
   fi_insert_edges(p, farnode, nbtree, intour, &last);
   tsp_tour[starter].next = farnode;
   tsp_tour[farnode].next = starter;
	
   cost = 2 * ICOST(&p->dist, starter, farnode);
   farside = MAX(farside, 2);
   cost = farthest_ins_from_to(p, tsp_tour, cost, 
			       2, farside, starter, nbtree, intour, &last);
	
   /*------------------------------------------------------------------------*\
   |  Order the elements in nbtree (and fix the intour values) so after that  |
   |  nbtree is suitable to continue with nearest insertion.                  |
   \*------------------------------------------------------------------------*/

   qsort((char *)(nbtree+1), last, sizeof(neighbor), compar);
   for (i=1; i<=last; i++)
      intour[nbtree[i].nbor] = i;
	
   /*-----------------------------------------------------------------------*\
   |               Continue with nearest insertion                           |
   \*-----------------------------------------------------------------------*/
   cost = nearest_ins_from_to(p, tsp_tour, cost, 
			      farside, vertnum-1, starter, nbtree, intour, 
			      &last);

  /*------------------------------------------------------------------------*\
  | We must arbitrarily choose a node to be the first node on the first      |
  | route in order to start the partitioning algorithm. The trials variable  |
  | tells us how many starting points to try. Its value is contained in      |
  | p->par.tsp.numstarts.                                                    |
  \*------------------------------------------------------------------------*/

   if (trials > vertnum-1) trials = vertnum-1;
   interval = (vertnum-1)/trials;
   opt_tours->cost = MAXINT;

  /*------------------------------------------------------------------------*\
  | Try various partitionings and take the solution that has the least cost  |
  \*------------------------------------------------------------------------*/

   for (i=0, cur_start = starter; i<trials; i++){
     make_routes(p, tsp_tour, cur_start, tours);
     if (tours->cost < opt_tours->cost){
       (void) memcpy ((char *)opt_tours, (char *)tours, sizeof(best_tours));
       (void) memcpy ((char *)opt_tour, (char *)tour, vertnum*sizeof(_node));
     }
     for (j=0; j<interval; j++)
       cur_start = tsp_tour[cur_start].next;
   }
     
	
   /*-----------------------------------------------------------------------*\
   |              Transmit the tour back to the parent                       |
   \*-----------------------------------------------------------------------*/

   send_tour(opt_tour, opt_tours->cost, opt_tours->numroutes, TSP_FINI,
	     used_time(&t), parent, vertnum, 0, NULL);
	
   if ( nbtree ) free ((char *) nbtree);
   if ( intour ) free ((char *) intour);
   if ( opt_tours ) free ((char *) opt_tours);
   if ( opt_tour ) free ((char *) opt_tour);
   if ( tsp_tour ) free ((char *) tsp_tour);
     
   free_heur_prob(p);
	
   PVM_FUNC(r_bufid, pvm_recv(parent, YOU_CAN_DIE));
   PVM_FUNC(info, pvm_freebuf(r_bufid));
   PVM_FUNC(info, pvm_exit());
}

