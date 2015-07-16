/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef INSIGHT_FLATPLATEBL_H
#define INSIGHT_FLATPLATEBL_H

#include "openfoam/openfoamanalysis.h"

namespace insight {

class FlatPlateBL 
: public OpenFOAMAnalysis
{
public:
  
#include "flatplatebl__FlatPlateBL__Parameters.h"
/*
PARAMETERSET>>> FlatPlateBL Parameters

inherits OpenFOAMAnalysis::Parameters

geometry=set
{
 HBydeltae = double 60.0 "Domain height above plate, divided by final BL thickness"
 WBydeltae = double 20.0 "Domain height above plate, divided by final BL thickness"
 L = double 5.0 "[m] Length of the domain"
 LapByL = double 0.1 "Length of the approach zone, divided by length of plate"
} "Geometrical properties of the domain"

mesh=set
{
 nh = int 50 "# cells in vertical direction"
 ypluswall = double 1 "yPlus of first cell at the wall grid layer at the final station"
 dxplus = double 1000 "lateral mesh spacing at the final station"
 dzplus = double 1000 "streamwise mesh spacing at the final station"
 twod = bool true "select method of transition enforcement"
 gradaxi = double 50 "grading from plate beginning towards inlet boundary"

 tripping = selectablesubset
 {{
    none
    set {
    }
    
    blocks
    set {
     n = int 4 "number evenly distributed tripping block across plate width"
     wbyh = double 6 "width of the blocks"
     lbyh = double 3 "length of the blocks"
     Reh = double 1000 "Reynolds number (formulated with height) of the tripping box"
    }
    
    drag
    set {
      CD = double 1 "Drag coefficient"
      lbyh = double 3 "length of the blocks"
      Reh = double 1000 "Reynolds number (formulated with height) of the tripping zone"
    }
    
  }} none "Tripping from laminar to turbulent flow"
  
} "Properties of the computational mesh"
    
operation=set
{
  uinf = double 17.78 "[m/s] free-stream velocity"
} "Definition of the operation point under consideration"

fluid = set 
{
  nu = double 1.8e-5 "[m^2/s] Viscosity of the fluid"
} "Parameters of the fluid"

run = set 
{
 iter = int 30000 "number of outer iterations after which the solver should stop"
 
 regime = selectablesubset
 {{
  
  steady
  set{
   iter = int 1000 "number of outer iterations after which the solver should stop"
  }
  
  unsteady
  set{
   inittime = double 10 "[T] length of grace period before averaging starts (as multiple of flow-through time)"
   meantime = double 10 "[T] length of time period for averaging of velocity and RMS (as multiple of flow-through time)"
   mean2time = double 10 "[T] length of time period for averaging of second order statistics (as multiple of flow-through time)"
  }
  
 }} steady "The simulation regime"

} "Solver parameters"

<<<PARAMETERSET
*/
  
protected:
#ifndef SWIG
    const static std::vector<double> sec_locs_;
#endif

    double Cw_, delta2e_, H_, W_, Re_theta2e_, uinf_, Re_L_, ypfac_e_, deltaywall_e_, gradh_, T_, dtrip_, gradax_, gradaxi_;
    int nax_, naxi_, nlat_;
    
    double avgStart_, avg2Start_, end_;
    
    std::string in_, out_, top_, cycl_prefix_, approach_, trip_;
    inline std::string tripMaster() const { return trip_+"_master"; }
    inline std::string tripSlave() const { return trip_+"_slave"; }
  
  /**
   * number of profiles for homogeneous averages
   */
  int n_hom_avg_=10;

public:
  declareType("Flat Plate Boundary Layer Test Case");
  
  FlatPlateBL(const NoParameters& p = NoParameters());
  
  virtual ParameterSet defaultParameters() const;
  virtual void calcDerivedInputData();
  virtual void createCase(insight::OpenFOAMCase& cm);
  virtual void createMesh(insight::OpenFOAMCase& cm);

  virtual void evaluateAtSection
  (
    OpenFOAMCase& cm, 
    ResultSetPtr results, double x, int i,
    const Interpolator& cf,
    const std::string& UMeanName,
    const std::string& RFieldName
  );  
  virtual insight::ResultSetPtr evaluateResults(insight::OpenFOAMCase& cm);
  
  virtual insight::Analysis* clone();
  
  /**
   * solves the function G(Alpha,D) numerically
   */
  static double G(double Alpha, double D);
  
  /**
   * computes the friction coefficient of a flat plate
   * @Re Reynolds number formulated with running length
   */
  static double cw(double Re, double Cplus=5.0);

  /**
   * computes the friction coefficient of a flat plate at station x
   * @Re Reynolds number formulated with running distance x
   */
  static double cf(double Re, double Cplus=5.0);
  
  static arma::mat integrateDelta123(const arma::mat& uByUinf_vs_y);
  static double searchDelta99(const arma::mat& uByUinf_vs_y);
};

}

#endif // INSIGHT_FLATPLATEBL_H
