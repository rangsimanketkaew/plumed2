/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2012-2023 The plumed team
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
#include "Bias.h"
#include "core/ActionRegister.h"

namespace PLMD {
namespace bias {

//+PLUMEDOC BIAS LOWER_WALLS
/*
Defines a wall for the value of one or more collective variables,
 which limits the region of the phase space accessible during the simulation.

If the inputs to the arguments are scalars the restraining potential starts acting on the system when the value of the CV is
lower than a certain limit $a_i$ (AT) minus an offset $o_i$ (OFFSET).  The expression for the bias due to the wall is given by:

$$
  \sum_i {k_i}|(x_i-a_i-o_i)/s_i|^e_i
$$

In this expression $k_i$ (KAPPA) is an energy constant in internal unit of the code, $s_i$ (EPS) a rescaling factor and
$e_i$ (EXP) the exponent determining the power law. By default: EXP = 2, EPS = 1.0, OFFSET = 0.

The following input tells plumed to add both a lower and an upper walls on the distance
between atoms 3 and 5 and the distance between atoms 2 and 4. The lower and upper limits
are defined at different values. The strength of the walls is the same for the four cases.
It also tells plumed to print the energy of the walls.

```plumed
d1: DISTANCE ATOMS=3,5
d2: DISTANCE ATOMS=2,4
uwall: UPPER_WALLS ARG=d1,d2 AT=1.0,1.5 KAPPA=150.0,150.0 EXP=2,2 EPS=1,1 OFFSET=0,0
lwall: LOWER_WALLS ARG=d1,d2 AT=0.0,1.0 KAPPA=150.0,150.0 EXP=2,2 EPS=1,1 OFFSET=0,0
PRINT ARG=uwall.bias,lwall.bias
```

Alternatively, if the input to this action is a vector as in the example shown below:

```plumed
d1: DISTANCE ATOMS1=1,2 ATOMS2=3,4 ATOMS3=5,6
lwall: LOWER_WALLS ARG=d1 AT=1.0 KAPPA=150.0 EXP=2 EPS=1 OFFSET=0
PRINT ARG=lwall.bias
```

The bias that acts is given by the following expression:

$$
  \sum_i \sum_j {k_i}|(x_{ij}-a_i-o_i)/s_i|^e_i
$$

The sum over $i$ here is runs over the two arguments, while the sum over $j$ runs over the three components of the input vectors.
Notice, that regardless of whether the input is a scalar, vector or matrix the number of $k_i$, $a_i$, $o_i$, $e_i$ and $s_i$ values must be
equal to the number of arguments to the action.

*/
//+ENDPLUMEDOC

class LWalls : public Bias {
  std::vector<double> at;
  std::vector<double> kappa;
  std::vector<double> exp;
  std::vector<double> eps;
  std::vector<double> offset;
public:
  explicit LWalls(const ActionOptions&);
  void calculate() override;
  static void registerKeywords(Keywords& keys);
};

PLUMED_REGISTER_ACTION(LWalls,"LOWER_WALLS_SCALAR")

void LWalls::registerKeywords(Keywords& keys) {
  Bias::registerKeywords(keys);
  keys.setDisplayName("LOWER_WALLS");
  keys.add("hidden","NO_ACTION_LOG","suppresses printing from action on the log");
  keys.add("compulsory","AT","the positions of the wall. The a_i in the expression for a wall.");
  keys.add("compulsory","KAPPA","the force constant for the wall.  The k_i in the expression for a wall.");
  keys.add("compulsory","OFFSET","0.0","the offset for the start of the wall.  The o_i in the expression for a wall.");
  keys.add("compulsory","EXP","2.0","the powers for the walls.  The e_i in the expression for a wall.");
  keys.add("compulsory","EPS","1.0","the values for s_i in the expression for a wall");
  keys.addOutputComponent("force2","default","scalar","the instantaneous value of the squared force due to this bias potential");
}

LWalls::LWalls(const ActionOptions&ao):
  PLUMED_BIAS_INIT(ao),
  at(getNumberOfArguments(),0),
  kappa(getNumberOfArguments(),0.0),
  exp(getNumberOfArguments(),2.0),
  eps(getNumberOfArguments(),1.0),
  offset(getNumberOfArguments(),0.0) {
  // Note sizes of these vectors are automatically checked by parseVector :-)
  parseVector("OFFSET",offset);
  parseVector("EPS",eps);
  parseVector("EXP",exp);
  parseVector("KAPPA",kappa);
  parseVector("AT",at);
  checkRead();

  log.printf("  at");
  for(unsigned i=0; i<at.size(); i++) {
    log.printf(" %f",at[i]);
  }
  log.printf("\n");
  log.printf("  with an offset");
  for(unsigned i=0; i<offset.size(); i++) {
    log.printf(" %f",offset[i]);
  }
  log.printf("\n");
  log.printf("  with force constant");
  for(unsigned i=0; i<kappa.size(); i++) {
    log.printf(" %f",kappa[i]);
  }
  log.printf("\n");
  log.printf("  and exponent");
  for(unsigned i=0; i<exp.size(); i++) {
    log.printf(" %f",exp[i]);
  }
  log.printf("\n");
  log.printf("  rescaled");
  for(unsigned i=0; i<eps.size(); i++) {
    log.printf(" %f",eps[i]);
  }
  log.printf("\n");

  addComponent("force2");
  componentIsNotPeriodic("force2");
}

void LWalls::calculate() {
  double ene = 0.0;
  double totf2 = 0.0;
  for(unsigned i=0; i<getNumberOfArguments(); ++i) {
    double f = 0.0;
    const double cv=difference(i,at[i],getArgument(i));
    const double off=offset[i];
    const double epsilon=eps[i];
    const double lscale = (cv-off)/epsilon;
    if( lscale < 0.) {
      const double k=kappa[i];
      const double exponent=exp[i];
      double power = std::pow( -lscale, exponent );
      f = -( k / epsilon ) * exponent * power / lscale;
      ene += k * power;
      totf2 += f * f;
    }
    setOutputForce(i,f);
  }
  setBias(ene);
  getPntrToComponent("force2")->set(totf2);
}

}
}
