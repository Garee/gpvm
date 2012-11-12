#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "DeviceInfo.h"

const char *KERNEL_NAME = "vm";
const char *KERNEL_FILE = "kernels/vm.cl";
const char *KERNEL_MACROS = "-D QUEUE_SIZE=16 "
                            "-D COMPLETE=-1 " 
                            "-D READ=0 "
                            "-D WRITE=1";
const int QUEUE_SIZE = 16;
const int COMPLETE = -1;
const int READ = 0;
const int WRITE = 1;

void toggleState(cl::CommandQueue& commandQueue, cl::Buffer& stateBuffer, int *state);

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

    /* Create the kernel. */
    cl::Kernel kernel(program, KERNEL_NAME);
    
    /* Calculate the memory required to store the queues. */
    int qBufSize = (nQueues * QUEUE_SIZE) + nQueues;

    /* Allocate memory for the queues. */
    cl_uint2 *queues = new cl_uint2[qBufSize];
    cl_uint2 *readQueues = new cl_uint2[qBufSize];

    /* Initialise queue elements to zero. */
    for (int i = 0; i < qBufSize; i++) {
      queues[i].x = 0;
      queues[i].y = 0;
      readQueues[i].x = 0;
      readQueues[i].y = 0;
    }

    int *state = new int;
    *state = WRITE;

    /* Create memory buffers on the device. */
    cl::Buffer qBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, qBufSize * sizeof(cl_uint2));
    commandQueue.enqueueWriteBuffer(qBuffer, CL_TRUE, 0, qBufSize * sizeof(cl_uint2), queues);

    cl::Buffer rqBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, qBufSize * sizeof(cl_uint2));
    commandQueue.enqueueWriteBuffer(rqBuffer, CL_TRUE, 0, qBufSize * sizeof(cl_uint2), readQueues);

    cl::Buffer stateBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(int));
    commandQueue.enqueueWriteBuffer(stateBuffer, CL_TRUE, 0, sizeof(int), state);

    /* Set kernel arguments. */
    kernel.setArg(0, qBuffer);
    kernel.setArg(1, rqBuffer);
    kernel.setArg(2, computeUnits);
    kernel.setArg(3, stateBuffer);

    /* Set the NDRange. */
    cl::NDRange global(computeUnits), local(1);

    /* Run the kernel on NDRange until completion. */
    while (*state != COMPLETE) {
      commandQueue.enqueueNDRangeKernel(kernel, cl::NullRange, global, local);
      commandQueue.finish();
      toggleState(commandQueue, stateBuffer, state);
    }

    /* Read the modified queue buffer. */
    commandQueue.enqueueReadBuffer(qBuffer, CL_TRUE, 0, qBufSize * sizeof(cl_uint2), queues);

    /* Print the queue details. */
    for (int i = 0; i < nQueues; i++) {
      int x = ((queues[i].x & 0xFFFF0000) >> 16);
      int y = queues[i].x & 0xFFFF;
      std::cout << "(" << x << "," << y << " " << queues[i].y << ")" << " ";
    }
    std::cout << std::endl;
    std::cout << std::endl;

    /* Print the queues. */
    for (int i = nQueues; i < qBufSize; i++) {
      if ((i % QUEUE_SIZE) == 0) std::cout << std::endl;
      std::cout << "(" << queues[i].x << " " << queues[i].y << ")" << " ";
    }
    std::cout << std::endl;

    /* Cleanup */
    delete[] queues;
    delete[] readQueues;
    delete state;
  } catch (cl::Error error) {
    std::cout << "EXCEPTION: " << error.what() << " [" << error.err() << "]" << std::endl;
    std::cout << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << std::endl;
  }

  return 0;
}

void toggleState(cl::CommandQueue& commandQueue, cl::Buffer& stateBuffer, int *state) {
  commandQueue.enqueueReadBuffer(stateBuffer, CL_TRUE, 0, sizeof(int), state);
  if (*state == COMPLETE) return;
  *state = (*state == WRITE) ? READ : WRITE;
  commandQueue.enqueueWriteBuffer(stateBuffer, CL_TRUE, 0, sizeof(int), state);
  commandQueue.finish();
}
