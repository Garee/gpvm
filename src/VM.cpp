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

packet pkt_create(uint type, uint source, uint arg, uint sub, uint payload);
void pkt_set_type(packet *p, uint type);
void pkt_set_source(packet *p, uint source);
void pkt_set_arg_pos(packet *p, uint arg);
void pkt_set_sub(packet *p, uint sub);
void pkt_set_payload_type(packet *p, uint ptype);
void pkt_set_payload(packet *p, uint payload);

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
      platforms[0].getDevices(CL_DEVICE_TYPE_CPU, &devices);
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
    
    /* The code store stores bytecode in QUEUE_SIZE chunks. */
    bytecode *cStore = new bytecode[CSTORE_SIZE * QUEUE_SIZE];

    /* Populate the code store. */
    cStore[0] = 0x0000000200000000UL;
    cStore[1] = 0x6040000000000002UL;
    cStore[2] = 0x6040000000000002UL;
    
    /* Create initial packet. */
    packet p = pkt_create(REFERENCE, computeUnits + 1, 0, 0, 0);
    queues[nQueues] = p;   // Initial packet.
    queues[0].x = 1;       // Tail index is 1.
    queues[0].y = WRITE;   // Last operation is write.
    
    /* The subtask table. */
    subt *subt = createSubt();
    
    /* Each computate unit has its own scratch array for storing temporary results. */
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

/* Return a newly created packet. */
packet pkt_create(uint type, uint source, uint arg, uint sub, uint payload) {
  packet p;
  p.x = 0;
  p.y = 0;
  pkt_set_type(&p, type);
  pkt_set_source(&p, source);
  pkt_set_arg_pos(&p, arg);
  pkt_set_sub(&p, sub);
  pkt_set_payload_type(&p, VAL);
  pkt_set_payload(&p, payload);
  return p;
}

/* Set the packet type. */
void pkt_set_type(packet *p, uint type) {
  (*p).x = ((*p).x & ~PKT_TYPE_MASK) | ((type << PKT_TYPE_SHIFT) & PKT_TYPE_MASK);
}

/* Set the packet source address. */
void pkt_set_source(packet *p, uint source) {
  (*p).x = ((*p).x & ~PKT_SRC_MASK) | ((source << PKT_SRC_SHIFT) & PKT_SRC_MASK);
}

/* Set the packet argument position. */
void pkt_set_arg_pos(packet *p, uint arg) {
  (*p).x = ((*p).x & ~PKT_ARG_MASK) | ((arg << PKT_ARG_SHIFT) & PKT_ARG_MASK);
}

/* Set the packet subtask. */
void pkt_set_sub(packet *p, uint sub) {
  (*p).x = ((*p).x & ~PKT_SUB_MASK) | ((sub << PKT_SUB_SHIFT) & PKT_SUB_MASK);
}

/* Set the packet payload type. */
void pkt_set_payload_type(packet *p, uint ptype) {
  (*p).x = ((*p).x & ~PKT_PTYPE_MASK) | ((ptype << PKT_PTYPE_SHIFT) & PKT_PTYPE_MASK);
}

/* Set the packet payload. */
void pkt_set_payload(packet *p, uint payload) {
  (*p).y = payload;
}
