
#ifndef _HACC_FFT_FORWARD_SOLVE_H_
#define _HACC_FFT_FORWARD_SOLVE_H_

#include "hacc_fft.h"

class HaccFFTForwardSolve : public HaccFFT {

    public:
        
        HaccFFTForwardSolve(

                    HaccConfig & config,
                    double buffer_copy_MBps,
                    double fft_work_per_second
                    );

        void forward_solve();

};

#endif
