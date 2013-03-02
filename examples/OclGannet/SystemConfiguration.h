
/** \file SystemConfiguration.h
   
 \brief Gannet Service-based SoC project - C++/SystemC System Configuration
        
        Generated from SBA.yml with create_Cxx_SystemConfiguration.rb

  (c) 2008-2012 Wim Vanderbauwhede <wim@dcs.gla.ac.uk>
    
*/

//==============================================================================
//
// System Configuration
//
// GENERATED from YAML configuration using create_Cxx_SystemConfiguration.rb
//
//==============================================================================

// 

#ifndef _SBA_SYSTEM_CONFIGURATION_H_
#define _SBA_SYSTEM_CONFIGURATION_H_

//#include <map>

using namespace std;

typedef unsigned int UINT;

namespace SBA {

const UINT M_OclGannet_MAT_mult = 66049;
const UINT M_OclGannet_MAT_add = 66050;
const UINT M_OclGannet_MAT_sub = 66051;
const UINT M_OclGannet_MAT_unit = 66052;
const UINT M_OclGannet_MAT_det = 66053;
const UINT M_OclGannet_MAT_trans = 66054;
const UINT M_OclGannet_LET_assign = 66049;
const UINT M_OclGannet_LET_buf = 66050;
const UINT M_OclGannet_MEM_ptr = 65793;
const UINT M_OclGannet_MEM_const = 65794;
const UINT S_OclGannet_MAT = 5;
const UINT S_OclGannet_MAT = 6;
const UINT S_OclGannet_MEM = 1;
const UINT S_OclGannet_MAT = 2;
const UINT S_OclGannet_MAT = 3;
const UINT S_OclGannet_MAT = 4;
const UINT SC_OclGannet_MAT = 258;
const UINT SC_OclGannet_LET = 258;
const UINT SC_OclGannet_MEM = 257;

// Not elegant, but static arrays are a lot faster than linked lists!
const UINT NSERVICES = 6;
const UINT SERVICE_ADDRESSES[6]={6,5,4,3,2,1};
     
} // SBA
#endif /*_SBA_SYSTEM_CONFIGURATION_H_*/
