GPVM
==

GPVM is a parallel Gannet virtual machine implementation using OpenCL for GPU execution.

Obtaining the Source Code
-------------------------
The source code for the virtual machine is maintained using git and can be obtained by cloning the public Github
repository located at:

<pre>
https://github.com/Garee/gpvm
</pre>

With git installed, enter the following command at a terminal to clone the repository in the current working
directory:

<pre>
git clone https://github.com/Garee/gpvm.git
</pre>

Compiling the program
---------------------
A Makefile is supplied with the source code that can be used to build the program. An implementation of OpenCL
must be installed for the program to compile. Please check your vendor for specific installation instructions.

1. Build the program executable.
<pre>
make
</pre>

2. Removes object files but leaves the executable.
<pre>
make clean
</pre>

3. Removes all files produced from compilation.
<pre>
make distclean
</pre>

4. Runs the unit tests.
<pre>
make test
</pre>

Running the program
-------------------
<pre>
Usage: ./vm [bytecode-file] [n-services]
</pre>

* bytecode-file - The file that contains the bytecode produced by the Gannet compiler.
* n-services - The number of service nodes that the virtual machine should use.

Running the example
-------------------
<pre>
./vm examples/4/test_ocl_gannet_4.tdc64 6
</pre>

The service configuration file is examples/OclGannet.yml.

User Guide
----------

A user must complete the following steps to run a program within the virtual machine:

1. Write a task description program.
2. Implement the services used within this program.
3. Configure these services.
4. Compile the task description program and service configuration using the Gannet compiler to produce the
input bytecode.
5. Provide any input data and allocate memory for results.

### Service Implementations

All services are implemented within the service compute function in the file kernels/VM.cl. To add a service,
provide an additional case to the case statement using the associated service opcode and implement the service
in the new block.

### Input/Output Data

Inputs and outputs are handled by the data store. Implement the populateData(...) function that is located within
src/UserData.cpp. Example implementations have been provided. You will likely be using the provided ptr and
const services within your task description program to access your data.

### Important Files

* src/VM.cpp - The host program, access your results here.
* src/UserData.cpp - Contains the populateData(...) function which is used to populate the data store with
input data.
* kernels/VM.cl - The kernel program that implements the virtual machine. Contains the service implemen-
tations in the service compute function.
* include/SharedMacros.h - Contains configurable macros which may be used to tweak the system.
* tests/vmtest.c - Contains all unit tests.


