#include <malloc.h>
#include <stdlib.h>

#include "ins_routines.h"
#include "timemeas.h"
#include "messages.h"
#include "proccomm.h"
#include "compute_cost.h"
#include "vrp_macros.h"
#include "vrp_const.h"
#include "heur_routines.h"

void main(void)
{
  heur_prob *p;
  int numroutes, cur_route, nearnode, *starter;
  int mytid, info, r_bufid, parent;
  int *intour;
  int last, cost;
  neighbor *nbtree;
  _node *tour;
  route_data *route_info;
  int start;
  best_tours *tours;
  double t;

  (void) used_time(&t);

  mytid = pvm_mytid();
	
  p = (heur_prob *) calloc ((int)1, sizeof(heur_prob));
  tours = p->cur_tour = (best_tours *) calloc (1, sizeof(best_tours));
	
  /*-----------------------------------------------------------------------*\
  |                     Receive the VRP data                                |
  \*-----------------------------------------------------------------------*/

  parent = receive(p);

  PVM_FUNC(r_bufid, pvm_recv(-1, VRP_DATA));

  PVM_FUNC(info, pvm_upkbyte((char *)tours, sizeof(best_tours), 1));
  tour = p->cur_tour->tour = (_node *) calloc (p->vertnum, sizeof(_node));
  PVM_FUNC(info, pvm_upkbyte((char *)tour, (p->vertnum)*sizeof(_node), 1));
  numroutes = p->cur_tour->numroutes;
  starter = (int *) calloc (numroutes, sizeof(int));
  route_info = p->cur_tour->route_info
             = (route_data *) calloc (numroutes+1, sizeof(route_data));
	
  PVM_FUNC(r_bufid, pvm_recv(-1, VRP_DATA));
  PVM_FUNC(info, pvm_upkint(&start, 1, 1));/*receive the start
							   rule*/

  if (start != FAR_INS) srand(start); /*if the start rule is random, then*\
	                              \*initialize the random number gen.*/
  starters(p, starter, route_info, start);/*generate the route starters for*\
                                          \*all the clusters.              */
  /*-----------------------------------------------------------------------*\
  |                     Allocate arrays                                     |
  \*-----------------------------------------------------------------------*/

  nbtree   = (neighbor *) malloc (p->vertnum * sizeof(neighbor));
  intour   = (int *)      calloc (p->vertnum, sizeof(int));
	
  /*-----------------------------------------------------------------------*\
  |               Find the nearest insertion tour from 'starters'           |
  \*-----------------------------------------------------------------------*/

  for (cur_route=1; cur_route<=numroutes; cur_route++){
    /*---------------------------------------------------------------------*\
    | The first part of this loop adds the starter and the nearest node to  |
    | it into the route to initialize it. Then a function is called         |
    | which inserts the rest of the nodes into the route in nearest         |
    | insert order.                                                         |
    \*---------------------------------------------------------------------*/
    if (route_info[cur_route].numcust <= 1) continue;
    cost = 0;
    last = 0;
    intour[0] = 0;
    intour[starter[cur_route-1]] = IN_TOUR;
    ni_insert_edges(p, starter[cur_route-1], nbtree, intour, &last, tour, cur_route);
    nearnode = closest(nbtree, intour, &last);
    intour[nearnode] = IN_TOUR;
    ni_insert_edges(p, nearnode, nbtree, intour, &last, tour, cur_route);
    tour[starter[cur_route-1]].next = nearnode;
    tour[nearnode].next = starter[cur_route-1];
    if (starter[cur_route - 1] == 0)
      route_info[cur_route].first = route_info[cur_route].last = nearnode;
    if (nearnode == 0)
      route_info[cur_route].first = route_info[cur_route].last
	                          = starter[cur_route-1];      

    cost = 2 * ICOST(&p->dist, starter[cur_route-1], nearnode);
    cost = nearest_ins_from_to(p, tour, cost, 2, route_info[cur_route].numcust+1,
			       starter[cur_route-1],
			       nbtree, intour, &last, route_info, cur_route);
    route_info[cur_route].cost = cost;
  }

  tour[0].next = route_info[1].first;

  /*-------------------------------------------------------------------------*\
  | This loop points the last node of each route to the first node of the next|
  | route. At the end of this procedure, the last node of each route is       |
  | pointing at the depot, which is not what we want.                         |
  \*-------------------------------------------------------------------------*/
  for (cur_route = 1; cur_route< numroutes; cur_route++)
    tour[route_info[cur_route].last].next = route_info[cur_route+1].first;

  cost = compute_tour_cost(&p->dist, tour);
	
  /*-----------------------------------------------------------------------*\
  |               Transmit the tour back to the parent                      |
  \*-----------------------------------------------------------------------*/
  
  send_tour(tour, cost, numroutes, tours->algorithm, used_time(&t), parent,
	    p->vertnum, 1, route_info);
	
  if ( nbtree ) free ((char *) nbtree);
  if ( intour ) free ((char *) intour);
  if ( starter) free ((char *) starter);

  free_heur_prob(p);
	
  PVM_FUNC(r_bufid, pvm_recv(parent, YOU_CAN_DIE));
  PVM_FUNC(info, pvm_freebuf(r_bufid));
  PVM_FUNC(info, pvm_exit());
	
}

