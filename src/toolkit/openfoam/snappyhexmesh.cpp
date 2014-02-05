/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  hannes <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "snappyhexmesh.h"

#include "boost/filesystem.hpp"

using namespace boost;
using namespace boost::filesystem;

namespace insight
{
  
namespace snappyHexMeshFeats
{

void Feature::modifyFiles(const OpenFOAMCase& ofc, 
	      const boost::filesystem::path& location) const
{
}

Geometry::Geometry( Parameters const& p )
: p_(p)
{
}

void Geometry::addIntoDictionary(OFDictData::dict& sHMDict) const
{
  OFDictData::dict geodict;
  geodict["type"]="triSurfaceMesh";
  geodict["name"]=p_.name();
  //boost::filesystem::path x; x.f
  sHMDict.subDict("geometry")[p_.fileName().filename().c_str()]=geodict;

  OFDictData::dict castdict;
  OFDictData::list levels;
  levels.push_back(p_.minLevel());
  levels.push_back(p_.maxLevel());
  castdict["level"]=levels;
  sHMDict.subDict("castellatedMeshControls").subDict("refinementSurfaces")[p_.name()]=castdict;
}

void Geometry::modifyFiles(const OpenFOAMCase& ofc, 
	      const boost::filesystem::path& location) const
{
  boost::filesystem::path from(p_.fileName());
  boost::filesystem::path to(location/"constant"/"triSurface"/p_.fileName().filename());
  if (!exists(to.parent_path()))
    create_directories(to.parent_path());
  copy_file(from, to);
}

Feature* Geometry::clone() const
{
  return new Geometry(p_);
}

}

void setStdCastellatedCtrls(OFDictData::dict& castellatedCtrls)
{
  castellatedCtrls["maxLocalCells"]=500000;
  castellatedCtrls["maxGlobalCells"]=2000000;
  castellatedCtrls["minRefinementCells"]=10;
  castellatedCtrls["maxLoadUnbalance"]=0.1;
  castellatedCtrls["nCellsBetweenLevels"]=1;
  castellatedCtrls["resolveFeatureAngle"]=30.0;
}

void setStdSnapCtrls(OFDictData::dict& snapCtrls)
{
  snapCtrls["nSmoothPatch"]=3;
  snapCtrls["tolerance"]=4.0;
  snapCtrls["nSolveIter"]=30;
  snapCtrls["nRelaxIter"]=5;  
}

void setStdLayerCtrls(OFDictData::dict& layerCtrls)
{
  layerCtrls["relativeSizes"]=true;
  layerCtrls["expansionRatio"]=1.2;
  layerCtrls["finalLayerThickness"]=0.5;
  layerCtrls["minThickness"]=0.1;  
  layerCtrls["nGrow"]=1;  
  layerCtrls["featureAngle"]=30.0;  
  layerCtrls["nRelaxIter"]=3;  
  layerCtrls["nSmoothSurfaceNormals"]=1;  
  layerCtrls["nSmoothNormals"]=3;  
  layerCtrls["nSmoothThickness"]=10;  
  layerCtrls["maxFaceThicknessRatio"]=0.5;  
  layerCtrls["maxThicknessToMedialRatio"]=0.3;  
  layerCtrls["minMedianAxisAngle"]=130.0;  
  layerCtrls["nBufferCellsNoExtrude"]=0;  
  layerCtrls["nLayerIter"]=50;  
}

void setStdQualityCtrls(OFDictData::dict& qualityCtrls)
{
  qualityCtrls["maxNonOrtho"]=65.0;
  qualityCtrls["maxBoundarySkewness"]=20.0;
  qualityCtrls["maxInternalSkewness"]=4.0;
  qualityCtrls["maxConcave"]=80.0;  
  qualityCtrls["minFlatness"]=0.5;  
  qualityCtrls["minVol"]=1e-13;  
  qualityCtrls["minArea"]=-1.0;  
  qualityCtrls["minTwist"]=0.02;  
  qualityCtrls["minDeterminant"]=0.001;  
  qualityCtrls["minFaceWeight"]=0.02;  
  qualityCtrls["minVolRatio"]=0.01;  
  qualityCtrls["minTriangleTwist"]=-1.0;  
  qualityCtrls["nSmoothScale"]=4;  
  qualityCtrls["errorReduction"]=0.75;  
}


void snappyHexMesh(const OpenFOAMCase& ofc, 
		  const boost::filesystem::path& location,
		  const OFDictData::list& PiM,
		  const boost::ptr_vector<snappyHexMeshFeats::Feature>& ops,
		  bool overwrite
		  )
{
  using namespace snappyHexMeshFeats;
  
  OFDictData::dict sHMDict;
  
  // setup dict structure
  sHMDict["castellatedMesh"] = true;
  sHMDict["snap"] = true;
  sHMDict["addLayers"] = true;
  sHMDict["debug"] = 0;
  sHMDict["mergeTolerance"] = 1e-6;
  OFDictData::dict& geomCtrls=sHMDict.addSubDictIfNonexistent("geometry");
  OFDictData::dict& castellatedCtrls = sHMDict.addSubDictIfNonexistent("castellatedMeshControls");
  castellatedCtrls.addListIfNonexistent("features");
  castellatedCtrls.addSubDictIfNonexistent("refinementSurfaces");
  castellatedCtrls.addSubDictIfNonexistent("refinementRegions");
  OFDictData::dict& snapCtrls=sHMDict.addSubDictIfNonexistent("snapControls");
  OFDictData::dict& layerCtrls=sHMDict.addSubDictIfNonexistent("addLayersControls");
  layerCtrls.addSubDictIfNonexistent("layers");
  OFDictData::dict& qualityCtrls=sHMDict.addSubDictIfNonexistent("meshQualityControls");

  //  populate with defaults
  setStdSnapCtrls(snapCtrls);
  setStdCastellatedCtrls(castellatedCtrls);
  castellatedCtrls["locationInMesh"]=PiM;
  setStdLayerCtrls(layerCtrls);
  setStdQualityCtrls(qualityCtrls);

  BOOST_FOREACH( const snappyHexMeshFeats::Feature& feat, ops)
  {
    feat.modifyFiles(ofc, location);
    feat.addIntoDictionary(sHMDict);
  }
  
  // then write to file
  boost::filesystem::path dictpath = location / "system" / "snappyHexMeshDict";
  if (!exists(dictpath.parent_path())) 
  {
    boost::filesystem::create_directories(dictpath.parent_path());
  }
  
  {
    std::ofstream f(dictpath.c_str());
    writeOpenFOAMDict(f, sHMDict, boost::filesystem::basename(dictpath));
  }

  std::vector<std::string> opts;
  if (overwrite) opts.push_back("-overwrite");
        
  ofc.executeCommand(location, "snappyHexMesh", opts);
}
  
}