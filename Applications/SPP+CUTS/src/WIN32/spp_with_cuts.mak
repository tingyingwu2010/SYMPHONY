##############################################################################
##############################################################################
#                                                                            #
# This file is part of the SYMPHONY Branch, Cut, and Price Library.          #
#                                                                            #
# SYMPHONY was jointly developed by Ted Ralphs (tkralphs@lehigh.edu) and     #
# Laci Ladanyi (ladanyi@us.ibm.com).                                         #
#                                                                            #
# (c) Copyright 2003  Ted Ralphs. All Rights Reserved.                       #
#                                                                            #
# This software is licensed under the Common Public License. Please see      #
# accompanying file for terms.                                               #
#                                                                            #
##############################################################################
##############################################################################

##############################################################################
##############################################################################
#
# This makefile is for Microsoft Visual C++ usage only! In order to compile 
# this application, simply type the following command:
#
# nmake /f spp_with_cuts_with_cuts.mak 
#
# The executable "symphony.exe" for this application will be created in 
# .\Debug directory. By default, SYMPHONY is set up to use the CLP
# optimization solver via COIN-OSI's CLP interface and to use the COIN-CGL cuts. 
# However, you are free to  specify your own settings for the executable via 
# the following variables.
##############################################################################

##############################################################################
# The SYMPHONYROOT environment variable specifies the root directory for the 
# source code. If this file is not in the SYMPHONY root directory, change this
# variable to the correct path.
# (you can download nmake.exe from  "http://download.microsoft.com/download/
# vc15/Patch/1.52/W95/EN-US/Nmake15.exe" if you need that.)
##############################################################################

SYMPHONYROOT=..\..

##############################################################################
# COINROOT is the path to the root directory of the COIN libraries. Many of
# the new features of COIN require the COIN libraries to be installed.
##############################################################################

COINROOT = C:\COIN

##############################################################################
# OUTDIR variable specifies where to create the executable file, 
# "symphony.exe", the corresponding objects and the dependencies.  
##############################################################################

OUTDIR=.\Debug

##############################################################################
##############################################################################
# LP solver dependent definitions
##############################################################################
##############################################################################

##############################################################################
##############################################################################
#You must define an LP solver in order to use the software. By default, this 
# option is set to OsI_CLP. See the corresponding "LPINCDIR" and "LPLIB" 
# variables used to put the lp solver include files and the libraries on path
# and make the necessary changes if you require.
##############################################################################
##############################################################################

##############################################################################
# CPLEX definitions
##############################################################################

# Uncomment the line below if you want to use CPLEX and specify the 
# corresponding paths to the solver files and libraries. 

#LP_SOLVER = CPLEX
!IF "$(LP_SOLVER)" == "CPLEX"
LPINCDIR = "C:\ILOG\cplex81\include\ilcplex"
LPLIB = "C:\ILOG\cplex81\lib\msvc6\stat_sta\cplex81.lib"
!ENDIF

##############################################################################
# OSL definitions
##############################################################################

# Uncomment the line below if you want to use OSL and specify the 
# corresponding paths to the solver files and libraries. 

#LP_SOLVER = OSL
!IF "$(LP_SOLVER)" == "OSL"
LPINCDIR = "C:\Program Files\IbmOslV3Lib\osllib\include"
LPLIB = "C:\Program Files\IbmOslV3Lib\osllib\lib\oslmd6030.lib"
!ENDIF

##############################################################################
# OSI definitions
##############################################################################

# Uncomment the line below if you want to use OSI interface and specify the 
# corresponding paths to the solver files and libraries. 

LP_SOLVER = OSI
OSI_INTERFACE = CLP

!IF "$(LP_SOLVER)" == "OSI"
LPINCDIR = \
	"$(COINROOT)\Coin\include" /I\
	"$(COINROOT)\Osi\include"
LPLIB = \
	"$(COINROOT)\Win\coinLib\Debug\coinLib.lib" \
	"$(COINROOT)\Win\osiLib\Debug\osiLib.lib"
!ENDIF


!IF "$(OSI_INTERFACE)" == "CPLEX"
LPINCDIR = $(LPINCDIR) /I\
	"C:\ILOG\cplex81\include\ilcplex" /I\
	"$(COINROOT)\Osi\OsiCpx\include"
LPLIB = $(LPLIB) \
	"C:\ILOG\cplex81\lib\msvc6\stat_sta\cplex81.lib" \
	"$(COINROOT)\Win\osiCpxLib\Debug\osiCpxLib.lib"
!ENDIF


!IF "$(OSI_INTERFACE)" == "OSL"
LPINCDIR = $(LPINCDIR) /I\
	"C:\Program Files\IbmOslV3Lib\osllib\include" /I\
	"$(COINROOT)\Osi\OsiOsl\include"
LPLIB = $(LPLIB) \
	"C:\Program Files\IbmOslV3Lib\osllib\lib\oslmd6030.lib" \
        "$(COINROOT)\Win\osiOslLib\Debug\osiOslLib.lib"
!ENDIF


!IF "$(OSI_INTERFACE)" == "CLP"
LPINCDIR = $(LPINCDIR) /I\
	"$(COINROOT)\Clp\include" /I\
	"$(COINROOT)\Osi\OsiClp\include"
LPLIB = $(LPLIB) \
	"$(COINROOT)\Win\clpLib\Debug\clpLib.lib" \
	"$(COINROOT)\Win\osiClpLib\Debug\osiClpLib.lib"
!ENDIF

!IF "$(OSI_INTERFACE)" == "XPRESS"
LPINCDIR = $(LPINCDIR) /I\
	"C:\" /I\
	"$(COINROOT)\Osi\OsiXpr\include"
LPLIB = $(LPLIB) \
	"C:\" \
	"$(COINROOT)\Win\osiXprLib\Debug\osiXprLib.lib"
!ENDIF

!IF "$(OSI_INTERFACE)" == "SOPLEX"
LPINCDIR = $(LPINCDIR) /I\
	"C:\" /I\
	"$(COINROOT)\Osi\OsiSpx\include"
LPLIB = $(LPLIB) \
	"C:\" \
	"$(COINROOT)\Win\osiSpxLib\Debug\osiSpxLib.lib"
!ENDIF

!IF "$(OSI_INTERFACE)" == "DYLP"
LPINCDIR = $(LPINCDIR) /I\
	"C:\" /I\
	"$(COINROOT)\Osi\OsiDylp\include"
LPLIB = $(LPLIB) \
	"C:\" \
	"$(COINROOT)\Win\osiDylpLib\Debug\osiDylpLib.lib"
!ENDIF


!IF "$(OSI_INTERFACE)" == "GLPK"
LPINCDIR = $(LPINCDIR) /I\
	"C:\GLPK\glpk-4.0\include" /I\
	"$(COINROOT)\Osi\OsiGlpk\include"
LPLIB = $(LPLIB) \
	"C:\GLPK\glpk-4.0\glpk.lib" \
	"$(COINROOT)\Win\osiGlpkLib\Debug\osiGlpkLib.lib"
!ENDIF


##############################################################################
# Besides the above variables, you have to set your environment path to solver
# specific dynamic libraries if there exists any. For instance, you have to 
# set your path to where "cplex81.dll" is, something like: 
#
#              "set path = %path%;C:\ILOG\cplex81\bin\msvc6 " 
#
# if you are using CPLEX 8.1 and Visual C++ 6. 
##############################################################################

##############################################################################
# SOLVER definition for SYMPHONY
##############################################################################

!IF "$(LP_SOLVER)" == "OSI"
DEFINITIONS ="__$(LP_SOLVER)_$(OSI_INTERFACE)__"
!ELSE
DEFINITIONS ="__$(LP_SOLVER)__"
!ENDIF

##############################################################################
# GLPMPL definitions. The user should set "USE_GLPMPL" variable to "TRUE" and 
# specify the paths for "glpk" files if she wants to read in glpmpl files.  
##############################################################################

USE_GLPMPL = FALSE

!IF "$(USE_GLPMPL)" == "TRUE"
LPINCDIR = $LPINCDIR) C:\GLPK\glpk-4.0\include
LPLIB = $(LPLIB) C:\GLPK\glpk-4.0\glpk.lib
DEFINITIONs = $(DEFINITIONS) /D "USE_GLPMPL"
!ENDIF

##############################################################################
##############################################################################
# Generate generic cutting planes. If you are using the OSI interface, you 
# can now add generic cutting planes from the CGL by setting the flag below.
# Which cutting planes are added can be controlled by SYMPHONY parameters (see
# the user's manual
##############################################################################
##############################################################################

USE_CGL_CUTS = TRUE

!IF "$(USE_CGL_CUTS)" == "TRUE"
LPINCDIR = $(LPINCDIR) /I "$(COINROOT)\Cgl\include"
LPLIB = $(LPLIB) "$(COINROOT)\Win\cglLib\Debug\cglLib.lib"
DEFINITIONS= $(DEFINITIONS) /D "USE_CGL_CUTS"
!ENDIF

##############################################################################
# If you wish to compile and use the SYMPHONY callable library through the 
# SYMPHONY OSI interface, set USE_OSI_INTERFACE to TRUE below. Note that
# you must have COIN installed to use this capability. See above to set the 
# path to the COIN directories. 
##############################################################################

USE_OSI_INTERFACE = FALSE

!IF "$(USE_OSI_INTERFACE)" == "TRUE"
ALL_INCDIR = $(LPINCDIR) /I "$(COINROOT)\Osi\OsiSym\include"
ALL_LIB = $(LPLIB) "$(COINROOT)\Win\osiSymLib\Debug\osiSymLib.lib"
!ELSE
ALL_INCDIR = $(LPINCDIR)
ALL_LIB = $(LPLIB)
!ENDIF


##############################################################################
##############################################################################
#
# Compiling and Linking...
#
##############################################################################
##############################################################################

DEFINITIONS = $(DEFINITIONS) /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" \
	/D "COMPILE_IN_CG" /D "COMPILE_IN_CP" /D "COMPILE_IN_LP" \
	/D "COMPILE_IN_TM" /D "USE_SYM_APPLICATION" 

ALL_INCDIR =$(ALL_INCDIR) /I "$(SYMPHONYROOT)\include" /I "..\include"


.SILENT:

CPP=cl.exe
CPPFLAGS= /nologo /MLd /W2 /GR /Gm /YX /GX /ZI /Od \
	/I $(ALL_INCDIR) /D $(DEFINITIONS) \
	/Fp"$(OUTDIR)\spp_with_cuts.pch" /Fo"$(OUTDIR)\\" \
	/Fd"$(OUTDIR)\spp_with_cuts.pdb" /FD /GZ /c /Tp

.c.obj: 
	$(CPP) $(CPPFLAGS) "$*.c"

ALL : "$(OUTDIR)" "LIB_MESSAGE" sym_lib "APPL_MESSAGE" "APPL_OBJECTS" \
	spp_with_cuts_exe 

CLEAN:
	del /Q $(OUTDIR)\*.obj
        del /Q $(OUTDIR)\spp_with_cuts.exe 
        del /Q $(OUTDIR)\symphonyLib.lib
        del /Q $(OUTDIR)\spp_with_cuts.idb
        del /Q $(OUTDIR)\spp_with_cuts.pdb
	del /Q $(OUTDIR)\spp_with_cuts.pch

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"


APPL_MESSAGE: 
	echo Compiling application files...

LIB_MESSAGE:
	echo Creating SYMPHONY library...

APPL_OBJECTS : \
	..\Common\spp_common.obj \
	..\CutGen\spp_cg.obj \
	..\CutGen\spp_cg_clique.obj \
	..\CutGen\spp_cg_functions.obj \
	..\CutPool\spp_cp.obj \
	..\DrawGraph\spp_dg.obj \
	..\LP\spp_lp.obj \
	..\LP\spp_lp_branch.obj \
	..\LP\spp_lp_functions.obj \
	..\Master\spp_master.obj \
	..\Master\spp_master_functions.obj \
	..\Master\spp_main.obj
	echo Application files compiled successfully...
	echo ...

LINK_OBJECTS= \
	$(OUTDIR)\spp_common.obj \
	$(OUTDIR)\spp_cg.obj \
	$(OUTDIR)\spp_cg_clique.obj \
	$(OUTDIR)\spp_cg_functions.obj \
	$(OUTDIR)\spp_cp.obj \
	$(OUTDIR)\spp_dg.obj \
	$(OUTDIR)\spp_lp.obj \
	$(OUTDIR)\spp_lp_branch.obj \
	$(OUTDIR)\spp_lp_functions.obj \
	$(OUTDIR)\spp_master.obj \
	$(OUTDIR)\spp_master_functions.obj \
	$(OUTDIR)\spp_main.obj

sym_lib : \
	$(SYMPHONYROOT)\Common\pack_array.obj \
	$(SYMPHONYROOT)\Common\pack_cut.obj \
	$(SYMPHONYROOT)\Common\proccomm.obj \
	$(SYMPHONYROOT)\Common\qsortucb.obj \
	$(SYMPHONYROOT)\Common\qsortucb_di.obj \
	$(SYMPHONYROOT)\Common\qsortucb_i.obj \
	$(SYMPHONYROOT)\Common\qsortucb_ic.obj \
	$(SYMPHONYROOT)\Common\qsortucb_id.obj \
	$(SYMPHONYROOT)\Common\qsortucb_ii.obj \
	$(SYMPHONYROOT)\Common\timemeas.obj \
	$(SYMPHONYROOT)\CutGen\cg_func.obj \
	$(SYMPHONYROOT)\CutGen\cg_proccomm.obj \
	$(SYMPHONYROOT)\CutGen\cg_wrapper.obj \
	$(SYMPHONYROOT)\CutGen\cut_gen.obj \
	$(SYMPHONYROOT)\CutPool\cp_func.obj \
	$(SYMPHONYROOT)\CutPool\cp_proccomm.obj \
	$(SYMPHONYROOT)\CutPool\cp_wrapper.obj \
	$(SYMPHONYROOT)\CutPool\cut_pool.obj \
	$(SYMPHONYROOT)\LP\lp.obj \
	$(SYMPHONYROOT)\LP\lp_branch.obj \
	$(SYMPHONYROOT)\LP\lp_free.obj \
	$(SYMPHONYROOT)\LP\lp_genfunc.obj \
	$(SYMPHONYROOT)\LP\lp_proccomm.obj \
	$(SYMPHONYROOT)\LP\lp_rowfunc.obj \
	$(SYMPHONYROOT)\LP\lp_solver.obj \
	$(SYMPHONYROOT)\LP\lp_varfunc.obj \
	$(SYMPHONYROOT)\LP\lp_wrapper.obj \
	$(SYMPHONYROOT)\Master\master.obj \
	$(SYMPHONYROOT)\Master\master_func.obj \
	$(SYMPHONYROOT)\Master\master_io.obj \
	$(SYMPHONYROOT)\Master\master_wrapper.obj \
	$(SYMPHONYROOT)\TreeManager\tm_func.obj \
	$(SYMPHONYROOT)\TreeManager\tm_proccomm.obj \
	$(SYMPHONYROOT)\TreeManager\treemanager.obj
	lib.exe /nologo /out:$(OUTDIR)\symphonyLib.lib $(OUTDIR)\*.obj
	echo "symphonyLib.lib" created successfully...
	echo ...
               	          
spp_with_cuts_exe : $(LINK_OBJECTS) $(OUTDIR)\symphonyLib.lib
	echo Linking...
	$(CPP) /nologo /W3 /Fe"$(OUTDIR)\spp_with_cuts.exe" $(ALL_LIB) $**
	echo "spp_with_cuts.exe" created successfully...
