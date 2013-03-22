#include "DeviceInfo.h"

void DeviceInfo::show(const cl::Device& device) {
  cl_int err=CL_SUCCESS;
  std::cout << "\nDevice Info:"<< std::endl;
  
  for (std::map<std::string,InfoTuple>::reverse_iterator iter_=infotbl.rbegin(); iter_!=infotbl.rend();iter_++) {
    // for (std::map<std::string,InfoTuple>::iterator iter_=infotbl.begin(); iter_!=infotbl.end();iter_++) {
    std::string infostr = (*iter_).first;
    InfoTuple& infotuple=(*iter_).second;
    if (infotuple.infotype==ULONG) {
      cl_ulong info;
      err=device.getInfo(infotuple.infocode,&info);
      ulong_props[infostr]=info;
      std::cout <<infostr << "\t"<< info << "\n";
    } else if (infotuple.infotype==UINT ){
      cl_uint info;
      err=device.getInfo(infotuple.infocode,&info);
      uint_props[infostr]=info;
      std::cout <<infostr << "\t"<< info << "\n";
    } else if (infotuple.infotype==STRING) {
      std::string info;
      err=device.getInfo(infotuple.infocode,&info);
      string_props[infostr]=info;
      std::cout <<infostr << "\t"<< info << "\n";
    }
    if (err != CL_SUCCESS && err!=CL_TRUE ) { // ad hoc: a lot of
      std::cerr << "ERROR: " << infostr <<" ["<< infotuple.infocode <<"]: "<< err << std::endl;
      // exit( EXIT_FAILURE);
    }
  }
  std::cout << std::endl;
  
}

unsigned long DeviceInfo::global_mem_size(const cl::Device& device) {
  cl_ulong info;
  device.getInfo(infotbl["CL_DEVICE_GLOBAL_MEM_SIZE"].infocode,&info);
  return info;
}


unsigned long DeviceInfo::global_mem_max_alloc_size(const cl::Device& device) {
  cl_ulong info;
  device.getInfo(infotbl["CL_DEVICE_MAX_MEM_ALLOC_SIZE"].infocode,&info);
  return info;
}


unsigned int DeviceInfo::max_compute_units(const cl::Device& device) {
  cl_uint info;
  device.getInfo(infotbl["CL_DEVICE_MAX_COMPUTE_UNITS"].infocode,&info);
  return info;
}

unsigned int DeviceInfo::max_workgroup_size(const cl::Device& device) {
  cl_uint info;
  device.getInfo(infotbl["CL_DEVICE_MAX_WORK_GROUP_SIZE"].infocode,&info);
  return info;
}

bool DeviceInfo::isGPU(const cl::Device& device) {
  cl_device_type devtype;
  cl_int err=device.getInfo(CL_DEVICE_TYPE,&devtype);
  if (err != CL_SUCCESS && err!=CL_TRUE) {
    std::cerr << "ERROR: Device type check failed "<<err<<std::endl;
  }
  return (devtype==CL_DEVICE_TYPE_GPU);
}

bool DeviceInfo::isCPU(const cl::Device& device) {
  cl_device_type devtype;
  cl_int err=device.getInfo(CL_DEVICE_TYPE,&devtype);
  if (err != CL_SUCCESS && err!=CL_TRUE) {
    std::cerr << "ERROR: Device type check failed "<<err<<std::endl;
  }
  return (devtype==CL_DEVICE_TYPE_CPU);
}
