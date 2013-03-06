#define __CL_ENABLE_EXCEPTIONS
#define _IN_HOST

#include <CL/cl.hpp>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <bitset>
#include "DeviceInfo.h"
#include "SharedMacros.h"
#include "SharedTypes.h"
#include "Packet.h"

const char *KERNEL_NAME = "vm";
const char *KERNEL_FILE = "kernels/vm.cl";
const char *KERNEL_BUILD_OPTIONS = "-I include";

const int NPACKET_SHIFT = ((NBYTES * 8) - 16);
const int FS_Length = 32;
const long F_Length = 0x0000ffff00000000;

void toggleState(cl::CommandQueue& commandQueue, cl::Buffer& stateBuffer, int *state);
subt *createSubt();
void validateArguments(int argc);
std::deque<bytecode> readBytecode(char *bytecodeFile);
std::deque< std::deque<bytecode> > words2Packets(std::deque<bytecode>& bytecodeWords);

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

    /* Read the bytecode from file. */
    std::deque<bytecode> bytecodeWords = readBytecode(argv[1]);
    std::deque< std::deque<bytecode> > packets = words2Packets(bytecodeWords);

    /* Populate the code store. */
    for (std::deque< std::deque<bytecode> >::iterator iterP = packets.begin(); iterP != packets.end(); iterP++) {
      std::deque<bytecode> packet = *iterP;
      for (std::deque<bytecode>::iterator iterW = packet.begin(); iterW != packet.end(); iterW++) {
	bytecode word = *iterW;
        int packetN = iterP - packets.begin(); // Which packet?
        int wordN = iterW - packet.begin(); // Which word?
	codeStore[((packetN + 1) * QUEUE_SIZE) + wordN] = word;
      }
    }
    
    /* Create initial packet. */
    packet p = pkt_create(REFERENCE, computeUnits + 1, 0, 0, 1);
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
    data[0] = 5;
    data[1] = 256;
    data[2] = data[1] + (4);
    data[3] = data[2] + (4);
    data[4] = 4;
    data[5] = data[3] + (4);
    data[data[0] + 1] = data[5] + (4); // Pointer to scratch free/scratch memory.
    
    data[data[1]] = 3;
    data[data[1] + 1] = 2;
    data[data[1] + 2] = 1;
    data[data[1] + 3] = 5;
    
    data[data[2]] = 1;
    data[data[2] + 1] = 1;
    data[data[2] + 2] = 2;
    data[data[2] + 3] = 3;
    
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

    std::cout << data[data[5]] << " " << data[data[5] + 1] << std::endl;
    std::cout << data[data[5] + 2] << " " << data[data[5] + 3] << std::endl;
    
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

std::deque<bytecode> readBytecode(char *bytecodeFile) {
  std::ifstream f(bytecodeFile);
  std::deque<bytecode> bytecodeWords;
  
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

std::deque< std::deque<bytecode> > words2Packets(std::deque<bytecode>& bytecodeWords) {
  int nPackets = bytecodeWords.front() >> NPACKET_SHIFT; bytecodeWords.pop_front();
  std::deque< std::deque<bytecode> > packets;
  for (int p = 0; p < nPackets; p++) {
    std::deque<bytecode> packet;
    
    int length = 0;
    for (int i = 0; i < 3; i++) {
      bytecode headerWord = bytecodeWords.front();
      bytecodeWords.pop_front();
      if (i == 0) {
        length = (headerWord & F_Length) >> FS_Length;
      }
    }
    
    for (int i = 0; i < length; i++) {
      bytecode payloadWord = bytecodeWords.front();
      bytecodeWords.pop_front();
      packet.push_back(payloadWord);
    }
    
    packets.push_back(packet);
  }
  
  return packets;
}
