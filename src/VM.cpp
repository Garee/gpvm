#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "DeviceInfo.h"

const int COMPLETE = -1;
const int READ = 0;
const int WRITE = 1;
const int QUEUE_SIZE = 16;

const char *KERNEL_MACROS = "-D QUEUE_SIZE=16 -D COMPLETE=-1 -D READ=0 -D WRITE=1";
const char *KERNEL_FILE = "kernels/vm.cl";

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

    /* Calculate the number of queues we need. */
    int nQueues = computeUnits * computeUnits;

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
    cl_uint2 *queues = new cl_uint2[nQueues * QUEUE_SIZE];
    cl_uint2 *readQueues = new cl_uint2[nQueues * QUEUE_SIZE];

    /* Initialise queue elements to zero. */
    for (int i = 0; i < nQueues * QUEUE_SIZE; i++) {
      queues[i].x = 0;
      queues[i].y = 0;
      readQueues[i].x = 0;
      readQueues[i].y = 0;
    }
    
    /* Allocate memory for the queue details. Each vector stores the head index,
       the tail index and the type of the last operation. */
    cl_uint3 *qDetails = new cl_uint3[nQueues];
    cl_uint3 *rqDetails = new cl_uint3[nQueues];

    /* Initialise elements to zero. */
    for (int i = 0; i < nQueues; i++) {
      qDetails[i].x = 0; // Head
      qDetails[i].y = 0; // Tail
      qDetails[i].z = 0; // Type of last operation (r/w).
      rqDetails[i].x = 0; // Head
      rqDetails[i].y = 0; // Tail
      rqDetails[i].z = 0; // Type of last operation (r/w).
    }

    int *state = new int;
    *state = WRITE;

    /* Create memory buffers on the device. */
    cl::Buffer qBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, nQueues * QUEUE_SIZE * sizeof(cl_uint2));
    cl::Buffer qDetailsBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, nQueues * sizeof(cl_uint3));
    cl::Buffer rqBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, nQueues * QUEUE_SIZE * sizeof(cl_uint2));
    cl::Buffer rqDetailsBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, nQueues * sizeof(cl_uint3));
    cl::Buffer stateBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(int));
    commandQueue.enqueueWriteBuffer(qBuffer, CL_TRUE, 0, nQueues * QUEUE_SIZE * sizeof(cl_uint2), queues);
    commandQueue.enqueueWriteBuffer(qDetailsBuffer, CL_TRUE, 0, nQueues * sizeof(cl_uint3), qDetails);
    commandQueue.enqueueWriteBuffer(rqBuffer, CL_TRUE, 0, nQueues * QUEUE_SIZE * sizeof(cl_uint2), readQueues);
    commandQueue.enqueueWriteBuffer(rqDetailsBuffer, CL_TRUE, 0, nQueues * sizeof(cl_uint3), rqDetails);
    commandQueue.enqueueWriteBuffer(stateBuffer, CL_TRUE, 0, sizeof(int), state);

    /* Set kernel arguments. */
    kernel.setArg(0, qBuffer);
    kernel.setArg(1, qDetailsBuffer);
    kernel.setArg(2, rqBuffer);
    kernel.setArg(3, rqDetailsBuffer);
    kernel.setArg(4, computeUnits);
    kernel.setArg(5, stateBuffer);

    /* Set the NDRange. */
    cl::NDRange global(computeUnits), local(1);

    /* Run the kernel on NDRange until completion. */
    while (*state != COMPLETE) {
      commandQueue.enqueueNDRangeKernel(kernel, cl::NullRange, global, local);
      commandQueue.finish();
      commandQueue.enqueueReadBuffer(stateBuffer, CL_TRUE, 0, sizeof(int), state);
    }

    /* Read the modified queue buffer. */
    commandQueue.enqueueReadBuffer(qBuffer, CL_TRUE, 0, nQueues * QUEUE_SIZE * sizeof(cl_uint2), queues);

    /* Print the queues. */
    for (int i = 0; i < nQueues * QUEUE_SIZE; i++) {
      if ((i % QUEUE_SIZE) == 0) std::cout << std::endl;
      std::cout << "(" << queues[i].x << " " << queues[i].y << ")" << " ";
    }
    std::cout << std::endl;

    commandQueue.enqueueReadBuffer(qDetailsBuffer, CL_TRUE, 0, nQueues * sizeof(cl_uint3), qDetails);
    std::cout << "---------------" << std::endl;
    for (int i = 0; i < nQueues; i++) {
      std::cout << "(" << qDetails[i].x << " " << qDetails[i].y << " " << qDetails[i].z << ")" << " ";
    }
    std::cout << std::endl;

    std::cout << "===============================================" << std::endl;
    /* Read the modified queue buffer. */
    commandQueue.enqueueReadBuffer(rqBuffer, CL_TRUE, 0, nQueues * QUEUE_SIZE * sizeof(cl_uint2), readQueues);

    /* Print the queues. */
    for (int i = 0; i < nQueues * QUEUE_SIZE; i++) {
      if ((i % QUEUE_SIZE) == 0) std::cout << std::endl;
      std::cout << "(" << readQueues[i].x << " " << readQueues[i].y << ")" << " ";
    }
    std::cout << std::endl;

    commandQueue.enqueueReadBuffer(rqDetailsBuffer, CL_TRUE, 0, nQueues * sizeof(cl_uint3), rqDetails);
    std::cout << "---------------" << std::endl;
    for (int i = 0; i < nQueues; i++) {
      std::cout << "(" << rqDetails[i].x << " " << rqDetails[i].y << " " << rqDetails[i].z << ")" << " ";
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
