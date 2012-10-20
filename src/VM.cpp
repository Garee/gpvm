#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "DeviceInfo.h"

const int QUEUE_DEPTH = 16;
const std::string KERNEL_FILE("kernels/vm.cl");

int main() {

  DeviceInfo dInfo;

  try {
    /* Create a vector of available platforms. */
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    /* Create a vector of available devices on the platform. */
    std::vector<cl::Device> devices;
    platforms[0].getDevices(CL_DEVICE_TYPE_DEFAULT, &devices);
    
    /* Get the number of compute units for the first available device. */
    DeviceInfo dInfo;
    unsigned int computeUnits = dInfo.max_compute_units(devices[0]);

    /* Create a platform context for the available devices. */
    cl::Context context(devices);

    /* Create a command queue using the first available device. */
    cl::CommandQueue commandQueue = cl::CommandQueue(context, devices[0]);

    /* Read the kernel program source. */
    std::ifstream kernelSourceFile(KERNEL_FILE.c_str());
    std::string kernelSource(std::istreambuf_iterator<char>(kernelSourceFile), (std::istreambuf_iterator<char>()));
    cl::Program::Sources source(1, std::make_pair(kernelSource.c_str(), kernelSource.length() + 1));
    
    /* Create a program in the context using the kernel source code. */
    cl::Program program = cl::Program(context, source);
    
    /* Build the program for the available devices. */
    program.build(devices);

    /* Create the kernel. */
    cl::Kernel kernel(program, "add");
    
    /* Create memory buffers. */
    cl_uint2 *a = new cl_uint2[QUEUE_DEPTH];
    cl_uint2 *b = new cl_uint2[QUEUE_DEPTH];

    for (int i = 0; i < QUEUE_DEPTH; i++) {
      a[i].x = 0;
      a[i].y = 0;
      b[i].x = 1;
      b[i].y = 1;
    }

    cl::Buffer aBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, QUEUE_DEPTH * sizeof(cl_uint2));
    cl::Buffer bBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, QUEUE_DEPTH * sizeof(cl_uint2));

    commandQueue.enqueueWriteBuffer(aBuffer, CL_TRUE, 0, QUEUE_DEPTH * sizeof(cl_uint2), a);
    commandQueue.enqueueWriteBuffer(bBuffer, CL_TRUE, 0, QUEUE_DEPTH * sizeof(cl_uint2), b);

    /* Set kernel arguments. */
    kernel.setArg(0, aBuffer);
    kernel.setArg(1, bBuffer);

    /* Run the kernel on NDRange. */
    cl::NDRange global(computeUnits);
    cl::NDRange local(1);
    commandQueue.enqueueNDRangeKernel(kernel, cl::NullRange, global, local);
    
    /* Wait for completion. */
    commandQueue.finish();

    commandQueue.enqueueReadBuffer(bBuffer, CL_TRUE, 0, QUEUE_DEPTH * sizeof(cl_uint2), b);

    /* Get results. */
    std::cout << "------------" << std::endl;
    for (int i = 0; i < QUEUE_DEPTH; i++) {
      std::cout << b[i].x << " " << b[i].y << std::endl;
    }

    delete[] a;
    delete[] b;
  } catch (cl::Error error) {
    std::cout << "EXCEPTION: " << error.what() << " [" << error.err() << "]" << std::endl;
  }

  return 0;
}
