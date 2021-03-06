
import os, sys, re, time, math
import numpy as np
import scipy.interpolate as spi



class GetOutOfLoop( Exception ):
    pass

  
def save_remove(fn):
  # otherwise remove will be called twice in parallel runs!
  if os.path.exists(fn): 
    try:
      os.remove(fn)
    except:
      print "Failed to remove", fn


class ScratchDirComm(object):
  def __init__(self, basepath):
    self.scratchdir=os.path.join(basepath, "scratch")
    print "Using scratchdir: ", self.scratchdir, ", waiting for directory to occur."

    # wait for scratchdir to occur
    while not os.path.exists(self.scratchdir):    
	time.sleep(0.5)

    self.stopfn=os.path.join(self.scratchdir, "STOP")
	
  def waitForCFDResults(self, pairs):
    # wait for all signal files to occur
    present=[False]*len(pairs)
    while False in present:
	if os.path.exists(self.stopfn): 
	    save_remove(self.stopfn)
	    raise GetOutOfLoop()

	time.sleep(0.5)
	
	for i,p in enumerate(pairs):
	    startfn=os.path.join(self.scratchdir, "complete.%s"%p.cfdname)
	    if os.path.exists(startfn): 
		present[i]=True;

  def releaseFEMResults(self, pairs):
    for p in pairs:
      save_remove(os.path.join(self.scratchdir, "complete.%s"%p.cfdname))
      time.sleep(0.5)



class CFDFEMPair(object):
  
    allpairs={}
    pf={}
    
    def __init__(self, scratchdircomm, model, label, pairinfo):
	from Cata.cata import FORMULE
	from Accas import _F
	
        cfdname, femname = pairinfo
        self.label=label
        CFDFEMPair.allpairs[self.label]=self # register for later use in FORMULE
        self.cfdname=cfdname
        self.femname=femname
        self.scratchdircomm=scratchdircomm
        self.debug=True
        
        pf=CFDFEMPair.pf
	pf[self.label]=FORMULE(
			NOM_PARA=("X", "Y", "Z"),
			VALE="ppf([X, Y, Z], '"+self.label+"')"
			)
        
    def pressureRB(self):
	from Accas import _F
	return _F(
		    GROUP_MA=self.femname,
		    PRES=self.pf[self.label]
		  );
      
    def reread(self):
	pass
        
    def __call__(self, x):
	"""
	Return pressure at location x
	Here, a fixed pressure of 1 MPa is returned for debug purposes
	"""
        return 1.0
    
    def writeDisplacements(self, sol, instant):
      pass




def ppf(x, label):
  #print label, x
  return CFDFEMPair.allpairs[label](x)



class IpolCFDFEMPair(CFDFEMPair):

    def __init__(self, scratchdircomm, model, label, pairinfo):
      super(IpolCFDFEMPair, self).__init__(scratchdircomm, model, label, pairinfo)
      self.pts=np.loadtxt(os.path.join(self.scratchdircomm.scratchdir, "locations."+self.cfdname), delimiter=";")
      #self.tree=Invdisttree(self.pts)

    def reread(self):
        cfd_p=np.loadtxt(os.path.join(self.scratchdircomm.scratchdir, "pressure_values."+self.cfdname), delimiter=";")
        #self.tree.setData(cfd_p)
        self.pinterp=spi.NearestNDInterpolator(self.pts, cfd_p)
        
    def __call__(self, p):
	"""
	Interpolate pressure at location x in FEM model
	"""
        r=self.pinterp(p[0], p[1], p[2])
        return r

    def writeDisplacements(self, sol, instant):
	from Cata.cata import POST_RELEVE_T, DETRUIRE
	from Accas import _F
	
	force=POST_RELEVE_T(ACTION=(_F(OPERATION='EXTRACTION',
                                INTITULE='force',
                                RESULTAT=sol,
                                NOM_CHAM='FORC_NODA',
                                INST=(instant),
                                GROUP_NO=self.femname,
                                RESULTANTE=('DX', 'DY', 'DZ'),
                                #TOUT_CMP='OUI'
                            ),
                         ),);
	ft=force.EXTR_TABLE().values()
	print "applied force=", ft['DX'][0], ft['DY'][0], ft['DZ'][0]
	DETRUIRE(CONCEPT=( _F(NOM=(force) ) ), INFO=1);

    	# extract displacements of coupled boundaries        
	utab=POST_RELEVE_T(ACTION=(_F(OPERATION='EXTRACTION',
					INTITULE='disp',
					RESULTAT=sol,
					NOM_CHAM='DEPL',
					INST=(instant),
					#GROUP_NO=('v_2', 'v_3'),
					GROUP_NO=self.femname,
					TOUT_CMP='OUI'
				    )
				));
	u=utab.EXTR_TABLE().values();
	uinterp_pts=np.asarray(zip(u['COOR_X'], u['COOR_Y'], u['COOR_Z']))
	disp=np.asarray(zip(u['DX'], u['DY'], u['DZ']))

	if self.debug:
	  f=open(os.path.join(self.scratchdircomm.scratchdir, "dbg.FEMlocations."+self.cfdname), "w")
	  for p in uinterp_pts: f.write( "%g;%g;%g\n"%(p[0], p[1], p[2]) )
	  f.close()
	  f=open(os.path.join(self.scratchdircomm.scratchdir, "dbg.FEMdisplacements."+self.cfdname), "w")
	  for u in disp: f.write( "%g;%g;%g\n"%(u[0], u[1], u[2]) )
	  f.close()
	
	uinterp=spi.NearestNDInterpolator(uinterp_pts, disp)
        u_pts=[uinterp(p[0], p[1], p[2]) for p in self.pts]
        
        print "Pair %s/%s: min cmpt="%(self.cfdname,self.femname), \
	      np.min(u_pts), "max cmpt=", np.max(u_pts)
	      
        f=open(os.path.join(self.scratchdircomm.scratchdir, "displacements."+self.cfdname), 'w')
        f.write("(\n")
        for j,u in enumerate(u_pts): 
	  f.write("(%g %g %g)\n"%(u[0], u[1], u[2]))
        f.write(")\n")
        f.flush()
        os.fsync(f.fileno())
        f.close()
        
	DETRUIRE(CONCEPT=( _F(NOM=(utab) ) ), INFO=1);


def FSILoop(scratchdircomm, pairs, runfunc):

    sol=None

    try:
      while True:

	# wait for and read CFD results (pressure fields)
	scratchdircomm.waitForCFDResults(pairs)
	for p in pairs: p.reread()
	
	# do FEM
	sol, finalinstant=runfunc(sol, pairs)
	
	# write FEM results (displacement fields)
	for p in pairs: p.writeDisplacements(sol, finalinstant)

	# remove all signal files, release CFD simulation
	scratchdircomm.releaseFEMResults(pairs)

    except GetOutOfLoop:
      pass    
    
    return sol
    
    
