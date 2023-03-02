# Soy

Welcome to Soy, a specialized Mixed-Integer Linear Program (MILP) solver for Piecewise Affine (PWA) dynamics.

Soy combines logical reasoning, arithmetic reasoning and stocastic local search for efficiently finding a feasible solution to a set of MILP constraints.


## Installation
For now we use Gurobi as an LP solver. Gurobi requires a license (a free
academic license is available), after getting one the software can be downloaded
[here](https://www.gurobi.com/downloads/gurobi-optimizer-eula/) and [here](https://www.gurobi.com/documentation/9.5/quickstart_linux/software_installation_guid.html#section:Installation) are
installation steps, there is a [compatibility
issue](https://support.gurobi.com/hc/en-us/articles/360039093112-C-compilation-on-Linux) that should be addressed.
A quick installation reference:
```
export INSTALL_DIR=/opt
sudo tar xvfz gurobi9.5.1_linux64.tar.gz -C $INSTALL_DIR
cd $INSTALL_DIR/gurobi951/linux64/src/build
sudo make
sudo cp libgurobi_c++.a ../../lib/
```
Next it is recommended to add the following to the .bashrc (but not necessary) 
```
export GUROBI_HOME="/opt/gurobi951/linux64"
export PATH="${PATH}:${GUROBI_HOME}/bin"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${GUROBI_HOME}/lib"
export GRB_LICENSE_FILE="/path/to/gurobi.lic" # Path to the gurobi license file
```

After setting up Gurobi, in the root directory of the project:
```
mkdir build
cd build
cmake ../
make -j12
```

If installation is successful, you should see all unit tests passing and an executable ``Soy'' is created in the build folder. 

## Dump the MILP problem in MPS format
Soy takes in MILP problem defined in the standard MPS format. Most off-the-shelf MICP solvers (e.g., Gurobi, CPLEX, Mosek, etc.) supports dumping the model in MPS format. 

## Solve the MILP

``./build/Soy [problem].mps --solution-file solution.txt``

This will invoke Soy on the problem. If a feasible solution is found, it will dump the feasible solution in "solution.txt".

## Contributing
We welcome both code contribution and benchmark contribution to test our solver.
