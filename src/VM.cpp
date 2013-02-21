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
      /* Use CPU for development/debugging. */
      platforms[0].getDevices(CL_DEVICE_TYPE_CPU, &devices);
      
      // platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &devices);
    } catch (cl::Error error) {
      platforms[0].getDevices(CL_DEVICE_TYPE_CPU, &devices);
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
    program.build(devices, KERNEL_BUILD_OPTIONS);

    /* Create the kernel. */
    cl::Kernel kernel(program, KERNEL_NAME);

    /* Calculate the number of queues we need. */
    int nQueues = computeUnits * computeUnits;
    
    /* Calculate the memory required to store the queues. The first nQueue packets are used to store
       information regarding the queues themselves (head index, tail index and last operation performed). */
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
    bytecode *cStore = new bytecode[CSTORE_SIZE * QUEUE_SIZE];
    
    /* The subtask table. */
    subt *subt = createSubt();
    
    /* Scratch array for storing temporary results. */
    cl_char *scratch = new cl_char[SCRATCH_SIZE * computeUnits];
    
    /* Create memory buffers on the device. */
    cl::Buffer qBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, qBufSize * sizeof(packet));
    commandQueue.enqueueWriteBuffer(qBuffer, CL_TRUE, 0, qBufSize * sizeof(packet), queues);

    cl::Buffer rqBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, qBufSize * sizeof(packet));
    commandQueue.enqueueWriteBuffer(rqBuffer, CL_TRUE, 0, qBufSize * sizeof(packet), readQueues);

    cl::Buffer stateBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(int));
    commandQueue.enqueueWriteBuffer(stateBuffer, CL_TRUE, 0, sizeof(int), state);
    
    cl::Buffer cStoreBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, CSTORE_SIZE * QUEUE_SIZE * sizeof(bytecode));
    commandQueue.enqueueWriteBuffer(cStoreBuffer, CL_TRUE, 0, CSTORE_SIZE * QUEUE_SIZE * sizeof(bytecode), cStore);
    
    cl::Buffer subtBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(subt));
    commandQueue.enqueueWriteBuffer(subtBuffer, CL_TRUE, 0, sizeof(subt), subt);

    cl::Buffer scratchBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, SCRATCH_SIZE * sizeof(cl_char) * computeUnits);
    commandQueue.enqueueWriteBuffer(scratchBuffer, CL_TRUE, 0, SCRATCH_SIZE * sizeof(cl_char) * computeUnits, scratch);
    
    /* Set kernel arguments. */
    kernel.setArg(0, qBuffer);
    kernel.setArg(1, rqBuffer);
    kernel.setArg(2, computeUnits);
    kernel.setArg(3, stateBuffer);
    kernel.setArg(4, cStoreBuffer);
    kernel.setArg(5, subtBuffer);
    kernel.setArg(6, scratchBuffer);
    
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

  if (table) {
    table->av_recs[0] = 1; // The first element is the top of stack index.
  
    /* Populate the stack with the available records in the subtask table. */
    for (int i = 1; i < SUBT_SIZE + 1; i++) {
      table->av_recs[i] = i - 1;
    }
  }
    
  return table;
}
