/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2011-2023 The plumed team
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
#include "core/ActionWithMatrix.h"
#include "core/ActionRegister.h"

//+PLUMEDOC MCOLVAR QUATERNION_PRODUCT_MATRIX
/*
Calculate the outer product matrix from two vectors of quaternions

Calculate the outer product matrix from two vectors of quaternions. Quaternions are made of four numbers, one real number, and three imaginary numbers \textit{i}, \textit{j}, and \textit{k}. The imaginary numbers are not commutative:

\[ij =k\] \[jk=i\] \[ki=j\] \[ji = -k\] \[ik = -j\] \[kj = -i\] \[ii = jj = kk = -1\]

Thus multiplying two quaternions $\mathbf{q_1} = w_1 + x_1i + y_1j + z_1k  $ and $\mathbf{q_2} = w_2 + x_2i + y_2j + z_2k$ yields the following four components:

\[w_{12} = w_1w_2 - x_1x_2 - y_1y_2 -z_1z_2\]
\[x_{12}i = (w_1x_2 + x_1w_2 + y_1z_2 - z_1y_2  )i\]
\[y_{12}j = (w_1y_2 - x_1z_2 + y_1w_2 + z_1x_2)j\]
\[z_{12}k = (w_1z_2 + x_1y_2 - y_1x_2 + z_1w_2)k\]

Quaternions can also be multiplied by a real number, and the conjugate $\mathbf{\overline{q}} = w -xi - yj - zk$ is analogous to complex numbers.


This action takes two equal sized vectors of quaternions of length $n$, and returns four $n\times n$ outer product matrices, corresponding to components w, x, y, and z in the above example. These matrices are real numbers, and will not behave with the usual nuances of quaternion algebra in any CUSTOMS, or other actions, that will need to be accounted for manually. The vectors of quaternions can be the same vectors, or different. Note, since the QUATERNION action returns unit quaternions, all quaternions returned here will also be unit quaternions.

```plumed
#in a system of 4 molecules, each with 3 atoms
#define quaternions 1-4
quats12: QUATERNION ATOMS1=1,2,3 ATOMS2=4,5,6
quats34: QUATERNION ATOMS1=7,8,9 ATOMS2=10,11,12

#take the outer product of 1,2 and 3,4
qp: QUATERNION_PRODUCT_MATRIX ARG=quats12.*,quats34.* #notice the asterisk
#the components need to be passed in the order w1,x1,y1,z1,w2,x2,y2,z2
#now use in an order parameter
bpw: CUSTOM ARG=qp.* VAR=w,x,y,z FUNC=exp(w/2+x/2+y/2+z/2) PERIODIC=NO
```

*/
//+ENDPLUMEDOC

namespace PLMD {
namespace crystdistrib {

class QuaternionProductMatrix : public ActionWithMatrix {
private:
  unsigned nderivatives;
public:
  static void registerKeywords( Keywords& keys );
  explicit QuaternionProductMatrix(const ActionOptions&);
  unsigned getNumberOfDerivatives();
  unsigned getNumberOfColumns() const override {
    return getConstPntrToComponent(0)->getShape()[1];
  }
  void setupForTask( const unsigned& task_index, std::vector<unsigned>& indices, MultiValue& myvals ) const ;
  void performTask( const std::string& controller, const unsigned& index1, const unsigned& index2, MultiValue& myvals ) const override;
  void runEndOfRowJobs( const unsigned& ival, const std::vector<unsigned> & indices, MultiValue& myvals ) const override ;
};

PLUMED_REGISTER_ACTION(QuaternionProductMatrix,"QUATERNION_PRODUCT_MATRIX")

void QuaternionProductMatrix::registerKeywords( Keywords& keys ) {
  ActionWithMatrix::registerKeywords(keys);
  keys.addInputKeyword("compulsory","ARG","vector","the labels of the quaternion vectors that you are outer product of");
  keys.addOutputComponent("w","default","matrix","the real component of quaternion");
  keys.addOutputComponent("i","default","matrix","the i component of the quaternion");
  keys.addOutputComponent("j","default","matrix","the j component of the quaternion");
  keys.addOutputComponent("k","default","matrix","the k component of the quaternion");
}

QuaternionProductMatrix::QuaternionProductMatrix(const ActionOptions&ao):
  Action(ao),
  ActionWithMatrix(ao) {
  if( getNumberOfArguments()!=8 ) {
    error("should be eight arguments to this action.  Four quaternions for each set of atoms.  You can repeat actions");
  }
  unsigned nquat = getPntrToArgument(0)->getNumberOfValues();
  for(unsigned i=0; i<8; ++i) {
    Value* myarg=getPntrToArgument(i);
    if( i==4 ) {
      nquat = getPntrToArgument(i)->getNumberOfValues();
    }
    if( myarg->getRank()!=1 ) {
      error("all arguments to this action should be vectors");
    }
    if( (myarg->getPntrToAction())->getName()!="QUATERNION_VECTOR" ) {
      error("all arguments to this action should be quaternions");
    }
    std::string mylab=getPntrToArgument(i)->getName();
    std::size_t dot=mylab.find_first_of(".");
    if( (i==0 || i==4) && mylab.substr(dot+1)!="w" ) {
      error("quaternion arguments are in wrong order");
    }
    if( (i==1 || i==5) && mylab.substr(dot+1)!="i" ) {
      error("quaternion arguments are in wrong order");
    }
    if( (i==2 || i==6) && mylab.substr(dot+1)!="j" ) {
      error("quaternion arguments are in wrong order");
    }
    if( (i==3 || i==7) && mylab.substr(dot+1)!="k" ) {
      error("quaternion arguments are in wrong order");
    }
  }
  std::vector<unsigned> shape(2);
  shape[0]=getPntrToArgument(0)->getShape()[0];
  shape[1]=getPntrToArgument(4)->getShape()[0];
  addComponent( "w", shape );
  componentIsNotPeriodic("w");
  addComponent( "i", shape );
  componentIsNotPeriodic("i");
  addComponent( "j", shape );
  componentIsNotPeriodic("j");
  addComponent( "k", shape );
  componentIsNotPeriodic("k");
  nderivatives = buildArgumentStore(0);
}

unsigned QuaternionProductMatrix::getNumberOfDerivatives() {
  return nderivatives;
}

void QuaternionProductMatrix::setupForTask( const unsigned& task_index, std::vector<unsigned>& indices, MultiValue& myvals ) const {
  unsigned start_n = getPntrToArgument(0)->getShape()[0], size_v = getPntrToArgument(4)->getShape()[0];
  if( indices.size()!=size_v+1 ) {
    indices.resize( size_v+1 );
  }
  for(unsigned i=0; i<size_v; ++i) {
    indices[i+1] = start_n + i;
  }
  myvals.setSplitIndex( size_v + 1 );
}

void QuaternionProductMatrix::performTask( const std::string& controller, const unsigned& index1, const unsigned& index2, MultiValue& myvals ) const {
  unsigned ostrn, ind2=index2;
  if( index2>=getPntrToArgument(0)->getShape()[0] ) {
    ind2 = index2 - getPntrToArgument(0)->getShape()[0];
  }

  std::vector<double> quat1(4), quat2(4);

  // Retrieve the first quaternion
  for(unsigned i=0; i<4; ++i) {
    quat1[i] = getArgumentElement( i, index1, myvals );
  }
  // Retrieve the second quaternion
  for(unsigned i=0; i<4; ++i) {
    quat2[i] = getArgumentElement( 4+i, ind2, myvals );
  }

  //make q1 the conjugate
  quat1[1] *= -1;
  quat1[2] *= -1;
  quat1[3] *= -1;


  double pref=1;
  double pref2=1;
  double conj=1;
//real part of q1*q2
  for(unsigned i=0; i<4; ++i) {
    if( i>0 ) {
      pref=-1;
      pref2=-1;
    }
    myvals.addValue( getConstPntrToComponent(0)->getPositionInStream(), pref*quat1[i]*quat2[i] );
    if( doNotCalculateDerivatives() ) {
      continue ;
    }
    if (i>0) {
      conj=-1;
    }
    addDerivativeOnVectorArgument( false, 0, i, index1, conj*pref*quat2[i], myvals );
    addDerivativeOnVectorArgument( false, 0, 4+i, ind2, pref2*quat1[i], myvals );
  }
  //i component
  pref=1;
  conj=1;
  pref2=1;
  for (unsigned i=0; i<4; i++) {
    if(i==3) {
      pref=-1;
    } else {
      pref=1;
    }
    if(i==2) {
      pref2=-1;
    } else {
      pref2=1;
    }
    myvals.addValue( getConstPntrToComponent(1)->getPositionInStream(), pref*quat1[i]*quat2[(5-i)%4]);
    if( doNotCalculateDerivatives() ) {
      continue ;
    }
    if (i>0) {
      conj=-1;
    }
    addDerivativeOnVectorArgument( false, 1, i, index1, conj*pref*quat2[(5-i)%4], myvals );
    addDerivativeOnVectorArgument( false, 1, 4+i, ind2, pref2*quat1[(5-i)%4], myvals );
  }

  //j component
  pref=1;
  conj=1;
  pref2=1;
  for (unsigned i=0; i<4; i++) {
    if(i==1) {
      pref=-1;
    } else {
      pref=1;
    }
    if (i==3) {
      pref2=-1;
    } else {
      pref2=1;
    }
    myvals.addValue( getConstPntrToComponent(2)->getPositionInStream(), pref*quat1[i]*quat2[(i+2)%4]);
    if( doNotCalculateDerivatives() ) {
      continue ;
    }
    if (i>0) {
      conj=-1;
    }
    addDerivativeOnVectorArgument( false, 2, i, index1, conj*pref*quat2[(i+2)%4], myvals );
    addDerivativeOnVectorArgument( false, 2, 4+i, ind2, pref2*quat1[(i+2)%4], myvals );
  }

  //k component
  pref=1;
  conj=1;
  pref2=1;
  for (unsigned i=0; i<4; i++) {
    if(i==2) {
      pref=-1;
    } else {
      pref=1;
    }
    if(i==1) {
      pref2=-1;
    } else {
      pref2=1;
    }
    myvals.addValue( getConstPntrToComponent(3)->getPositionInStream(), pref*quat1[i]*quat2[(3-i)]);
    if( doNotCalculateDerivatives() ) {
      continue ;
    }
    if (i>0) {
      conj=-1;
    }
    addDerivativeOnVectorArgument( false, 3, i, index1, conj*pref*quat2[3-i], myvals );
    addDerivativeOnVectorArgument( false, 3, 4+i, ind2, pref2*quat1[3-i], myvals );

  }


}

void QuaternionProductMatrix::runEndOfRowJobs( const unsigned& ival, const std::vector<unsigned> & indices, MultiValue& myvals ) const {
  if( doNotCalculateDerivatives() || !matrixChainContinues() ) {
    return ;
  }

  for(unsigned j=0; j<getNumberOfComponents(); ++j) {
    unsigned nmat = getConstPntrToComponent(j)->getPositionInMatrixStash(), nmat_ind = myvals.getNumberOfMatrixRowDerivatives( nmat );
    std::vector<unsigned>& matrix_indices( myvals.getMatrixRowDerivativeIndices( nmat ) );
    unsigned ntwo_atoms = myvals.getSplitIndex();
    // Quaternion for first molecule
    unsigned base = 0;
    for(unsigned k=0; k<4; ++k) {
      matrix_indices[nmat_ind] = base + ival;
      base += getPntrToArgument(k)->getShape()[0];
      nmat_ind++;
    }
    // Loop over row of matrix
    for(unsigned i=1; i<ntwo_atoms; ++i) {
      unsigned ind2 = indices[i];
      if( ind2>=getPntrToArgument(0)->getShape()[0] ) {
        ind2 = indices[i] - getPntrToArgument(0)->getShape()[0];
      }
      base = 4*getPntrToArgument(0)->getShape()[0];
      // Quaternion of second molecule
      for(unsigned k=0; k<4; ++k) {
        matrix_indices[nmat_ind] = base + ind2;
        base += getPntrToArgument(4+k)->getShape()[0];
        nmat_ind++;
      }
    }
    myvals.setNumberOfMatrixRowDerivatives( nmat, nmat_ind );
  }

}

}
}
