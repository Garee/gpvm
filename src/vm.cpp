#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

const int N = 16;
const std::string KERNEL_FILE("kernels/add.cl");

int main() {
  int *a = new int[N];
  int *b = new int[N];
  for (int i = 0; i < N; ++i) {
    a[i] = i;
    b[i] = N - i;
  }

  try {
    /* Create a vector of available platforms. */
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    /* Create a vector of available devices on the platform. */
    std::vector<cl::Device> devices;
    platforms[0].getDevices(CL_DEVICE_TYPE_DEFAULT, &devices);

    /* Create a platform context for the available devices. */
    cl::Context context(devices);

    /* Create a command queue using the first available device. */
    cl::CommandQueue commandQueue = cl::CommandQueue(context, devices[0]);

    /* Read the kernel program source. */
    std::ifstream kernelSourceFile(KERNEL_FILE.c_str());
    std::string kernelSource(std::istreambuf_iterator<char>(kernelSourceFile), 
			     (std::istreambuf_iterator<char>()));
    cl::Program::Sources source(1, std::make_pair(kernelSource.c_str(), kernelSource.length() + 1));
    
    /* Create a program in the context using the kernel source code. */
    cl::Program program = cl::Program(context, source);
    
    /* Build the program for the available devices. */
    program.build(devices);

    /* Create the kernel. */
    cl::Kernel kernel(program, "add");
    
    /* Create memory buffers. */
    cl::Buffer aBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, N * sizeof(int));
    cl::Buffer bBuffer = cl::Buffer(context, CL_MEM_READ_ONLY, N * sizeof(int));
    cl::Buffer resultBuffer = cl::Buffer(context, CL_MEM_WRITE_ONLY, N * sizeof(int));

    /* Copy the arrays 'a' and 'b' to the memory buffers. */
    commandQueue.enqueueWriteBuffer(aBuffer, CL_TRUE, 0, N * sizeof(int), a);
    commandQueue.enqueueWriteBuffer(bBuffer, CL_TRUE, 0, N * sizeof(int), b);

    /* Set kernel arguments. */
    kernel.setArg(0, aBuffer);
    kernel.setArg(1, bBuffer);
    kernel.setArg(2, resultBuffer);

    /* Run the kernel on NDRange. */
    cl::NDRange global(N);
    cl::NDRange local(1);
    commandQueue.enqueueNDRangeKernel(kernel, cl::NullRange, global, local);
    
    /* Wait for completion. */
    commandQueue.finish();

    /* Get results. */
    int *c = new int[N];
    commandQueue.enqueueReadBuffer(resultBuffer, CL_TRUE, 0, N * sizeof(int), c);

    for (int i = 0; i < N; ++i) {
      std::cout << a[i] << " + " << b[i] << " = " << c[i] << std::endl;
    }

    /* Cleanup. */
    delete[] a;
    delete[] b;
    delete[] c;
  } catch (cl::Error error) {
    std::cout << "EXCEPTION: " << error.what() << " [" << error.err() << "]" << std::endl;
  }

  return 0;
}
