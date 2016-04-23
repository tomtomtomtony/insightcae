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

#include "extrusion.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(Extrusion);
addToFactoryTable(Feature, Extrusion, NoParameters);

Extrusion::Extrusion(const NoParameters& nop): Feature(nop)
{}


Extrusion::Extrusion(FeaturePtr sk, VectorPtr L, bool centered)
: sk_(sk), L_(L), centered_(centered)
{}

void Extrusion::build()
{
  if (!centered_)
  {
    setShape(BRepPrimAPI_MakePrism( sk_->shape(), to_Vec(L_->value()) ).Shape());
  }
  else
  {
    gp_Trsf trsf;
    trsf.SetTranslation(to_Vec(-0.5*L_->value()));
    setShape
    (
      BRepPrimAPI_MakePrism
      ( 
	BRepBuilderAPI_Transform(sk_->shape(), trsf).Shape(), 
	to_Vec(L_->value()) 
      ).Shape()
    );
  }
  
  copyDatums(*sk_);
}


void Extrusion::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Extrusion",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression
      > ( (  ',' > qi::lit("centered") > qi::attr(true) ) | qi::attr(false) ) 
      > ')' )
      [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Extrusion>(qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}

}
}
