/*===========================================================================*/
/*                                                                           */
/* This file is part of the SYMPHONY Branch, Cut, and Price Callable         */
/* Library.                                                                  */
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

#ifndef OsiSymSolverParameters_hpp
#define OsiSymSolverParameters_hpp

enum OsiSymIntParam {
   /** This controls the level of output */
   OsiSymVerbosity,
   OsiSymWarmStart,
   OsiSymNodeLimit,
   OsiSymFindFirstFeasible
};

enum OsiSymDblParam {
   /** The granularity is the actual minimum difference in objective function
       value for two solutions that actually have do different objective
       function values. For integer programs with integral objective function
       coefficients, this would be 1, for instance. */ 
   OsiSymGranularity,
   OsiSymTimeLimit,
   OsiSymGapLimit
};

enum OsiSymStrParam {
};

#endif
