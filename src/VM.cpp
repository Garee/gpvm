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
  try {
    /* Create a vector of available platforms. */
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    /* Create a vector of available devices on the platform. */
    std::vector<cl::Device> devices;
    platforms[0].getDevices(CL_DEVICE_TYPE_DEFAULT, &devices);
    
    /* Get the number of compute units for the first available device. */
    DeviceInfo dInfo;
    int computeUnits = dInfo.max_compute_units(devices[0]);

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
    cl_uint2 *queues = new cl_uint2[computeUnits * QUEUE_DEPTH];

    for (int i = 0; i < computeUnits * QUEUE_DEPTH; i++) {
      queues[i].x = 0;
      queues[i].y = 0;
    }

    cl::Buffer queueBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, computeUnits * QUEUE_DEPTH * sizeof(cl_uint2));
    commandQueue.enqueueWriteBuffer(queueBuffer, CL_TRUE, 0, computeUnits * QUEUE_DEPTH * sizeof(cl_uint2), queues);

    /* Set kernel arguments. */
    kernel.setArg(0, queueBuffer);

    /* Run the kernel on NDRange. */
    cl::NDRange global(computeUnits);
    cl::NDRange local(1);
    commandQueue.enqueueNDRangeKernel(kernel, cl::NullRange, global, local);
    
    /* Wait for completion. */
    commandQueue.finish();
    
    commandQueue.enqueueReadBuffer(queueBuffer, CL_TRUE, 0, computeUnits * QUEUE_DEPTH * sizeof(cl_uint2), queues);
    for (int i = 0; i < computeUnits * QUEUE_DEPTH; i++) {
	std::cout << queues[i].x << " " << queues[i].y << std::endl;
    }

    /* Cleanup */
    delete[] queues;
  } catch (cl::Error error) {
    std::cout << "EXCEPTION: " << error.what() << " [" << error.err() << "]" << std::endl;
  }

  return 0;
}
