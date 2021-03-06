/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
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
 */

#ifndef TOOLS_H
#define TOOLS_H

#include "base/boost_include.h"
#include "base/linearalgebra.h"

class vtkPolyData;
class vtkCellArray;


namespace insight
{
  
struct TemporaryCaseDir
{
  bool keep_;
  boost::filesystem::path dir;

  TemporaryCaseDir(bool keep=false, const std::string& prefix="");
  ~TemporaryCaseDir();
};


class SharedPathList 
: public std::vector<boost::filesystem::path>
{
public:
  SharedPathList();
  virtual ~SharedPathList();
  virtual boost::filesystem::path getSharedFilePath(const boost::filesystem::path& file);
  
  void insertIfNotPresent(const boost::filesystem::path& sp);
  void insertFileDirectoyIfNotPresent(const boost::filesystem::path& sp);
  
  static SharedPathList searchPathList;
};


class ExecTimer
: public boost::timer::auto_cpu_timer
{
public:
    ExecTimer(const std::string& name);

    ~ExecTimer();
};



class LineMesh_to_OrderedPointTable
        : public std::vector<arma::mat>
{
    void calcConnectionInfo(vtkCellArray* lines);

public:
    typedef std::vector<int> idList;
    typedef std::map<int,idList> idListMap;

    idListMap pointCells_, cellPoints_;
    std::set<int> endPoints_;

    LineMesh_to_OrderedPointTable(vtkPolyData* pd);

    inline int nEndpoints() const { return endPoints_.size(); }

    void printSummary(std::ostream&, vtkPolyData* pd=NULL) const;

    /**
     * @brief txyz
     * convert point list into a single matrix
     * @return
     * matrix with first column: distance coordinate,
     * cols 2,3,4: x, y, z
     */
    arma::mat txyz() const;
};


}

#endif // TOOLS_H
