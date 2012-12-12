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
                            "-D WRITE=1 "
                            "-D CSTORE_SIZE=256 "
                            "-D SUBT_SIZE=1024";
const int QUEUE_SIZE = 16;
const int COMPLETE = -1;
const int READ = 0;
const int WRITE = 1;
const int CSTORE_SIZE = 256 * QUEUE_SIZE;
const int SUBT_SIZE = 1024;

typedef cl_ulong bytecode;

typedef struct subt_rec {
  cl_uint service_id;            // [32bits] Opcode
  cl_uint args[QUEUE_SIZE];      // [32bits] Pointers to data or constants.
  cl_uchar arg_mode[QUEUE_SIZE]; // [4bits] Arg status, [4bits] Arg mode
  cl_uchar subt_status;          // [4bits]  Subtask status, [4bits] number of args absent.
  cl_uchar return_to;            // [8bits]
  cl_ushort return_as;           // [16bits] Subtask address + argument position.
} subt_rec; 

/* The subtask table with associated available record stack. */
typedef struct subt {
  subt_rec recs[SUBT_SIZE];             // The subtask table records.
  cl_ushort av_recs[SUBT_SIZE + 1];     // Stack of available records.
} subt;

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

    /* Which stage of the READ/WRITE cycle are we in? */
    int *state = new int;
    *state = WRITE;

    /* The code store. */
    bytecode *cStore = new bytecode[CSTORE_SIZE];

    /* The subtask table. */
    subt *subt = createSubt();

    /* Create memory buffers on the device. */
    cl::Buffer qBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, qBufSize * sizeof(cl_uint2));
    commandQueue.enqueueWriteBuffer(qBuffer, CL_TRUE, 0, qBufSize * sizeof(cl_uint2), queues);

    cl::Buffer rqBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, qBufSize * sizeof(cl_uint2));
    commandQueue.enqueueWriteBuffer(rqBuffer, CL_TRUE, 0, qBufSize * sizeof(cl_uint2), readQueues);

    cl::Buffer stateBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(int));
    commandQueue.enqueueWriteBuffer(stateBuffer, CL_TRUE, 0, sizeof(int), state);

    cl::Buffer cStoreBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, CSTORE_SIZE * sizeof(bytecode));
    commandQueue.enqueueWriteBuffer(cStoreBuffer, CL_TRUE, 0, CSTORE_SIZE * sizeof(bytecode), cStore);

    cl::Buffer subtBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(subt));
    commandQueue.enqueueWriteBuffer(subtBuffer, CL_TRUE, 0, sizeof(subt), subt);

    /* Set kernel arguments. */
    kernel.setArg(0, qBuffer);
    kernel.setArg(1, rqBuffer);
    kernel.setArg(2, computeUnits);
    kernel.setArg(3, stateBuffer);
    kernel.setArg(4, cStoreBuffer);
    kernel.setArg(5, subtBuffer);

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
    delete[] cStore;
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
