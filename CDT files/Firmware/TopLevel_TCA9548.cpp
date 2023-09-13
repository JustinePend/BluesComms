#include "TopLevel_TCA9548.h"

TCA9548A mux(Wire, 0);

void TCA9548_Init(){
    mux.begin();
}

void TCA9548_setChannel(muxChs_t muxCh){
    mux.setChannel(muxCh);
}
