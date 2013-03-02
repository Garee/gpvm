
#include <vector>
#include <string>
#define NBYTES 8

using namespace std;

vector<uint64_t> read_bytecode(string tdc_file) {
         FILE * fd=fopen(tdc_file.c_str(),"r");
         vector<uint64_t> bytewords;
        uint64_t byteword=0;
        uint hwb=0;
         uint64_t byte=0;
         while((int64_t)byte!=EOF) {
         byte=(uint64_t)fgetc(fd);
            byteword+=(byte<<(8*(NBYTES-1-hwb)));
            hwb=hwb+1;
            if (hwb==NBYTES){
                hwb=0;
                bytewords.push_back(byteword);
                byteword=0;
            }
         }
        return bytewords;
    }


vector< vector<uint64_t> > wl2td(const vector<uint64_t>& tdc_wl) {
    vector< vector<uint64_t> > packets;	
    uint64_t npackets=(uint64_t)tdc_wl.front();tdc_wl.pop_front();
    npackets = npackets >> (NBYTES*8-16);
    if (npackets==0){
        exit(0);
    }
    for(uint np=0;np<=npackets-1 ;np++) {
        vector<uint64_t> packet;
        int length=0;
        // get the header
        for(uint phw=0;phw<=2 ;phw++) {
            uint64_t hw=tdc_wl.front();tdc_wl.pop_front();
            if (phw==1) // get the length                
                length= (hw & F_Length) >> FS_Length;
            packet.push_back(hw);
        }
        // get the payload 
        for(uint j=0;j<=length-1 ;j++) {
            uint64_t plw=tdc_wl.front();tdc_wl.pop_front();
            packet.push_back(plw);
        }
        packets.push_back(packet);
    }
    return packet;
}



