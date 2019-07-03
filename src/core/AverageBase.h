/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2011-2017 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed.org for more information.

   This file is part of plumed, version 2.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#ifndef __PLUMED_core_AverageBase_h
#define __PLUMED_core_AverageBase_h
#include "ActionPilot.h"
#include "ActionAtomistic.h"
#include "ActionWithValue.h"
#include "ActionWithArguments.h"

namespace PLMD {

class AverageBase :
  public ActionPilot,
  public ActionAtomistic,
  public ActionWithValue,
  public ActionWithArguments {
private:
  bool firststep, clearnextstep;
  unsigned clearstride;
public:
  static void registerKeywords( Keywords& keys );
  explicit AverageBase( const ActionOptions& );
  void clearDerivatives( const bool& force=false ) {}
  unsigned getNumberOfDerivatives() const ;
  void getInfoForGridHeader( std::string& gtype, std::vector<std::string>& argn, std::vector<std::string>& min,
                             std::vector<std::string>& max, std::vector<unsigned>& nbin,
                             std::vector<double>& spacing, std::vector<bool>& pbc, const bool& dumpcube ) const ;
  void getGridPointIndicesAndCoordinates( const unsigned& ind, std::vector<unsigned>& indices, std::vector<double>& coords ) const ;
  void getGridPointAsCoordinate( const unsigned& ind, const bool& setlength, std::vector<double>& coords ) const ;
/// These are required because we inherit from both ActionAtomistic and ActionWithArguments
  void lockRequests();
  void unlockRequests();
  void calculateNumericalDerivatives( ActionWithValue* a=NULL ) { plumed_error(); }
  void calculate() {}
  void apply() {}
  void update();
  virtual void clearAccumulatedData() = 0;
  virtual void accumulateData( const double& cweight ) = 0;
};

}
#endif