#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "DeviceInfo.h"

const int QUEUE_SIZE = 16;
const char *KERNEL_FILE = "kernels/vm.cl";
const char *KERNEL_MACROS = "-D QUEUE_SIZE=16";

const int state = 1;

int main() {
  std::vector<cl::Platform> platforms;
  std::vector<cl::Device> devices;
  cl::Device device;
  cl::Program program;

  DeviceInfo dInfo;

  try {
    /* Create a vector of available platforms. */
    cl::Platform::get(&platforms);
    
    /* Create a vector of available devices (GPU Priority). */
    try {
      platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &devices);
    } catch (cl::Error error) {
      platforms[0].getDevices(CL_DEVICE_TYPE_DEFAULT, &devices);
    }

    /* Create a platform context for the available devices. */
    cl::Context context(devices);

    /* Use the first available device. */
    device = devices[0];
    
    /* Get the number of compute units for the device. */
    int computeUnits = dInfo.max_compute_units(device);

    /* Create a command queue for the device. */
    cl::CommandQueue commandQueue = cl::CommandQueue(context, device);

    /* Read the kernel program source. */
    std::ifstream kernelSourceFile(KERNEL_FILE);
    std::string kernelSource(std::istreambuf_iterator<char>(kernelSourceFile), (std::istreambuf_iterator<char>()));
    cl::Program::Sources source(1, std::make_pair(kernelSource.c_str(), kernelSource.length() + 1));
    
    /* Create a program in the context using the kernel source code. */
    program = cl::Program(context, source);
    
    /* Build the program for the available devices. */
    program.build(devices, KERNEL_MACROS);

    /* Create the qtest kernel. */
    cl::Kernel kernel(program, "qtest");
    
    /* Allocate memory for the queues. */
    cl_uint2 *queues = new cl_uint2[computeUnits * QUEUE_SIZE];

    /* Initialise queue elements to zero. */
    for (int i = 0; i < computeUnits * QUEUE_SIZE; i++) {
      queues[i].x = 0;
      queues[i].y = 0;
    }
    
    /* Allocate memory for the queue details. Each vector stores the head index,
       the tail index and the type of the last operation. */
    cl_uint3 *queueDetails = new cl_uint3[computeUnits];

    /* Initialise elements to zero. */
    for (int i = 0; i < computeUnits; i++) {
      queueDetails[i].x = 0; // Head
      queueDetails[i].y = 0; // Tail
      queueDetails[i].z = 0; // Type of last operation (r/w).
    }

    /* Create memory buffers on the device. */
    cl::Buffer queueBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, computeUnits * QUEUE_SIZE * sizeof(cl_uint2));
    cl::Buffer queueDetailsBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, computeUnits * sizeof(cl_uint3));
    commandQueue.enqueueWriteBuffer(queueBuffer, CL_TRUE, 0, computeUnits * QUEUE_SIZE * sizeof(cl_uint2), queues);
    commandQueue.enqueueWriteBuffer(queueDetailsBuffer, CL_TRUE, 0, computeUnits * sizeof(cl_uint3), queueDetails);

    /* Set kernel arguments. */
    kernel.setArg(0, queueBuffer);
    kernel.setArg(1, queueDetailsBuffer);
    kernel.setArg(2, state);

    /* Run the kernel on NDRange. */
    cl::NDRange global(computeUnits), local(1);
    commandQueue.enqueueNDRangeKernel(kernel, cl::NullRange, global, local);

    /* Wait for completion. */
    commandQueue.finish();
    
    /* Read the modified queue buffer. */
    commandQueue.enqueueReadBuffer(queueBuffer, CL_TRUE, 0, computeUnits * QUEUE_SIZE * sizeof(cl_uint2), queues);

    /* Print the queues. */
    for (int i = 0; i < computeUnits * QUEUE_SIZE; i++) {
      if ((i % QUEUE_SIZE) == 0) std::cout << std::endl;
      std::cout << "(" << queues[i].x << " " << queues[i].y << ")" << " ";
    }
    std::cout << std::endl;

    commandQueue.enqueueReadBuffer(queueDetailsBuffer, CL_TRUE, 0, computeUnits * sizeof(cl_uint3), queueDetails);
    std::cout << "---------------" << std::endl;
    for (int i = 0; i < computeUnits; i++) {
      std::cout << "(" << queueDetails[i].x << " " << queueDetails[i].y << " " << queueDetails[i].z << ")" << " ";
    }
    std::cout << std::endl;

    /* Cleanup */
    delete[] queues;
  } catch (cl::Error error) {
    std::cout << "EXCEPTION: " << error.what() << " [" << error.err() << "]" << std::endl;
    std::cout << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << std::endl;
  }

  return 0;
}
