#define __CL_ENABLE_EXCEPTIONS
#define _IN_HOST

#include <CL/cl.hpp>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <bitset>
#include "DeviceInfo.h"
#include "SharedMacros.h"
#include "SharedTypes.h"
#include "Packet.h"

const char *KERNEL_NAME = "vm";
const char *KERNEL_FILE = "kernels/vm.cl";
const char *KERNEL_BUILD_OPTIONS = "-g -I include";

void toggleState(cl::CommandQueue& commandQueue, cl::Buffer& stateBuffer, int *state);
subt *createSubt();
void validateArguments(int argc);
std::vector<bytecode> readBytecode(char *bytecodeFile);

int main(int argc, char **argv) {
  validateArguments(argc);
  
  std::vector<cl::Platform> platforms;
  std::vector<cl::Device> devices;
  cl::Device device;
  cl::Program program;
  
  DeviceInfo deviceInfo;

  try {
    /* Create a vector of available platforms. */
    cl::Platform::get(&platforms);

    /* Create a vector of available devices (GPU Priority). */
    try {
      /* Use CPU for debugging */
      platforms[0].getDevices(CL_DEVICE_TYPE_CPU, &devices);

      /* Use GPU in practice. */
      // platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &devices);
    } catch (cl::Error error) {
      platforms[0].getDevices(CL_DEVICE_TYPE_CPU, &devices);
    }

    /* Create a platform context for the available devices. */
    cl::Context context(devices);

    /* Use the first available device. */
    device = devices[0];

    /* Get the number of compute units for the device. */
    int computeUnits = deviceInfo.max_compute_units(device);

    /* Get the global memory size (in bytes) of the device. */
    // long globalMemSize = deviceInfo.global_mem_size(device);
    
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

    /* The code store stores bytecode in QUEUE_SIZE chunks. */
    bytecode *codeStore = new bytecode[CODE_STORE_SIZE * QUEUE_SIZE];
    
    /* Populate the code store. */
    codeStore[0] = 0x0000000300000000UL;
    codeStore[1] = 0x4000000100000000UL;
    codeStore[2] = 0x4000000200000000UL;
    codeStore[3] = 0x4000000300000000UL;

    codeStore[16] = 0x0000000100000100UL;
    codeStore[17] = 0x6040000000000000UL;

    codeStore[32] = 0x0000000100000100UL;
    codeStore[33] = 0x6040000000000001UL;

    codeStore[48] = 0x0000000100000100UL;
    codeStore[49] = 0x6040000000000002UL;
    
    std::vector<bytecode> bytecodeWords = readBytecode(argv[1]);
    for (std::vector<bytecode>::iterator it = bytecodeWords.begin(); it != bytecodeWords.end(); it++) {
      std::bitset<64> x(*it);
      std::cout << x << std::endl;
    }

    /* Create initial packet. */
    packet p = pkt_create(REFERENCE, NOWHERE, 0, 0, 0);
    queues[nQueues] = p;   // Initial packet.
    queues[0].x = 1 << 16; // Tail index is 1.
    queues[0].y = WRITE;   // Last operation is write.
    
    /* The subtask table. */
    subt *subtaskTable = createSubt();
    
    long avGlobalMemSize = 1024 * 1024 * 1024; // In bytes.
    long dataSize = avGlobalMemSize / 4; // How many 32-bit integers?
    
    /* Each computate unit has its own data array for storing temporary results. [input][data] */
    cl_uint *data = new cl_uint[avGlobalMemSize];

    /* Write input data to data buffer. */
    
    /* :1        :255            :X        :Y
       nargs     narg0..nargN    in/out    scratch */
    data[0] = 3;
    data[1] = 256;
    data[2] = 256 + 1;
    data[3] = 256 + 2;
    data[data[0] + 1] = 256 + 3;

    data[256] = 2;
    data[256 + 1] = 7;

    /* Create memory buffers on the device. */
    cl::Buffer qBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, qBufSize * sizeof(packet));
    commandQueue.enqueueWriteBuffer(qBuffer, CL_TRUE, 0, qBufSize * sizeof(packet), queues);

    cl::Buffer rqBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, qBufSize * sizeof(packet));
    commandQueue.enqueueWriteBuffer(rqBuffer, CL_TRUE, 0, qBufSize * sizeof(packet), readQueues);

    cl::Buffer stateBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(int));
    commandQueue.enqueueWriteBuffer(stateBuffer, CL_TRUE, 0, sizeof(int), state);

    cl::Buffer codeStoreBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, CODE_STORE_SIZE * QUEUE_SIZE * sizeof(bytecode));
    commandQueue.enqueueWriteBuffer(codeStoreBuffer, CL_TRUE, 0, CODE_STORE_SIZE * QUEUE_SIZE * sizeof(bytecode), codeStore);

    cl::Buffer subtaskTableBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(subt));
    commandQueue.enqueueWriteBuffer(subtaskTableBuffer, CL_TRUE, 0, sizeof(subt), subtaskTable);

    cl::Buffer dataBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, dataSize * sizeof(cl_uint));
    commandQueue.enqueueWriteBuffer(dataBuffer, CL_TRUE, 0, dataSize * sizeof(cl_uint), data);

    /* Set kernel arguments. */
    kernel.setArg(0, qBuffer);
    kernel.setArg(1, rqBuffer);
    kernel.setArg(2, computeUnits);
    kernel.setArg(3, stateBuffer);
    kernel.setArg(4, codeStoreBuffer);
    kernel.setArg(5, subtaskTableBuffer);
    kernel.setArg(6, dataBuffer);

    /* Set the NDRange. */
    cl::NDRange global(computeUnits), local(computeUnits);

    /* Run the kernel on NDRange until completion. */
    while (*state != COMPLETE) {
      commandQueue.enqueueNDRangeKernel(kernel, cl::NullRange, global, local);
      commandQueue.finish();
      toggleState(commandQueue, stateBuffer, state);
    }

    /* Read the modified buffers. */
    commandQueue.enqueueReadBuffer(qBuffer, CL_TRUE, 0, qBufSize * sizeof(packet), queues);
    commandQueue.enqueueReadBuffer(dataBuffer, CL_TRUE, 0, dataSize * sizeof(cl_uint), data);


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
    
    std::cout << "Result: " << data[258] << std::endl;

    /* Cleanup */
    delete[] queues;
    delete[] readQueues;
    delete[] codeStore;
    delete[] data;
    delete subtaskTable;
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


void validateArguments(int argc) {
  if (argc < 2) {
    std::cout << "Usage: ./vm [bytecode-file]" << std::endl;
    exit(EXIT_FAILURE);
  }
}

std::vector<bytecode> readBytecode(char *bytecodeFile) {
  std::ifstream f(bytecodeFile);
  std::vector<bytecode> bytecodeWords;
  
  if (f.is_open()) {
    while (f.good()) {
      bytecode word = 0;
      for (int i = 0; i < NBYTES; i++) {
        char c = f.get();
        word = (word << NBYTES) + c;
      }
      
      bytecodeWords.push_back(word);
    }
  }
  
  return bytecodeWords;
}
