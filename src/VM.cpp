#define __CL_ENABLE_EXCEPTIONS
#define _IN_HOST

#include <CL/cl.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "DeviceInfo.h"
#include "SharedMacros.h"
#include "SharedTypes.h"

const char *KERNEL_NAME = "vm";
const char *KERNEL_FILE = "kernels/vm.cl";
const char *KERNEL_BUILD_OPTIONS = "-I include";

void toggleState(cl::CommandQueue& commandQueue, cl::Buffer& stateBuffer, int *state);
subt *createSubt();

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
    program.build(devices, KERNEL_BUILD_OPTIONS);

    /* Create the kernel. */
    cl::Kernel kernel(program, KERNEL_NAME);
    
    /* Calculate the memory required to store the queues. */
    int qBufSize = (nQueues * QUEUE_SIZE) + nQueues;

    /* Allocate memory for the queues. */
    packet *queues = new packet[qBufSize];
    packet *readQueues = new packet[qBufSize];
    
    /* Initialise queue elements to zero. */
    for (int i = 0; i < qBufSize; i++) {
      queues[i].x = 0;
      queues[i].y = 0;
      readQueues[i].x = 0;
      readQueues[i].y = 0;
    }

    /* Which stage of the READ/WRITE cycle are we in? */
    int *state = new int;
    *state = WRITE;
    
    /* The code store. */
    bytecode *cStore = new bytecode[CSTORE_SIZE];
    
    /* The subtask table. */
    subt *subt = createSubt();
    
    /* Input data to be worked on. */
    cl_char *in = new cl_char[IN_SIZE];
    
    /* Somewhere to store the results. */
    cl_char *result = new cl_char[RESULT_SIZE];

    /* Scratch array for storing temporary results. */
    cl_char *scratch = new cl_char[SCRATCH_SIZE];
    
    /* Create memory buffers on the device. */
    cl::Buffer qBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, qBufSize * sizeof(packet));
    commandQueue.enqueueWriteBuffer(qBuffer, CL_TRUE, 0, qBufSize * sizeof(packet), queues);

    cl::Buffer rqBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, qBufSize * sizeof(packet));
    commandQueue.enqueueWriteBuffer(rqBuffer, CL_TRUE, 0, qBufSize * sizeof(packet), readQueues);

    cl::Buffer stateBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(int));
    commandQueue.enqueueWriteBuffer(stateBuffer, CL_TRUE, 0, sizeof(int), state);
    
    cl::Buffer cStoreBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, CSTORE_SIZE * sizeof(bytecode));
    commandQueue.enqueueWriteBuffer(cStoreBuffer, CL_TRUE, 0, CSTORE_SIZE * sizeof(bytecode), cStore);

    cl::Buffer subtBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(subt));
    commandQueue.enqueueWriteBuffer(subtBuffer, CL_TRUE, 0, sizeof(subt), subt);

    cl::Buffer inBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(cl_char));
    commandQueue.enqueueWriteBuffer(inBuffer, CL_TRUE, 0, sizeof(cl_char), in);
    
    cl::Buffer resultBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(cl_char));
    commandQueue.enqueueWriteBuffer(resultBuffer, CL_TRUE, 0, sizeof(cl_char), result);
    
    cl::Buffer scratchBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(cl_char));
    commandQueue.enqueueWriteBuffer(scratchBuffer, CL_TRUE, 0, sizeof(cl_char), scratch);

    /* Set kernel arguments. */
    kernel.setArg(0, qBuffer);
    kernel.setArg(1, rqBuffer);
    kernel.setArg(2, computeUnits);
    kernel.setArg(3, stateBuffer);
    kernel.setArg(4, cStoreBuffer);
    kernel.setArg(5, subtBuffer);
    kernel.setArg(6, inBuffer);
    kernel.setArg(7, resultBuffer);
    kernel.setArg(8, scratchBuffer);
    
    /* Set the NDRange. */
    cl::NDRange global(computeUnits), local(1);

    /* Run the kernel on NDRange until completion. */
    while (*state != COMPLETE) {
      commandQueue.enqueueNDRangeKernel(kernel, cl::NullRange, global, local);
      commandQueue.finish();
      toggleState(commandQueue, stateBuffer, state);
    }

    /* Read the modified queue buffer. */
    commandQueue.enqueueReadBuffer(qBuffer, CL_TRUE, 0, qBufSize * sizeof(packet), queues);
    
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
    delete[] cStore;
    delete[] in;
    delete[] result;
    delete[] scratch;
    delete subt;
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

subt *createSubt() {
  subt *table = new subt;
  table->av_recs[0] = 1; // First index keeps track of top of the stack index.
  
  /* Populate the stack with the available records in the subtask table. */
  for (int i = 1; i < SUBT_SIZE + 1; i++) {
    table->av_recs[i] = i - 1;
  }
  
  return table;
}
