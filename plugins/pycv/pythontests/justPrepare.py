import numpy as np
import plumedCommunications

plumedInit={"Value": plumedCommunications.defaults.COMPONENT,}

def plumedPrepare(_: plumedCommunications.PythonCVInterface):
    toret = {"setAtomRequest": f"1"}
    return toret


def plumedCalculate(action: plumedCommunications.PythonCVInterface):
    at: np.ndarray = action.getPositions()
    return at[0][0]+at[0][1]
