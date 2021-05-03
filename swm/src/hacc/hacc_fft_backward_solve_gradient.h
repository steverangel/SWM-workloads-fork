#ifndef _HACC_FFT_BACKWARD_SOLVE_GRADIENT_H_
#define _HACC_FFT_BACKWARD_SOLVE_GRADIENT_H_

#include "hacc_fft.h"

class HaccFFT;

class HaccFFTBackwardSolveGradient : public HaccFFT {

    public:

        HaccFFTBackwardSolveGradient(

                    HaccConfig & config,
                    double buffer_copy_MBps,
                    double fft_work_per_second
                    );

        //BOZO w/ axis
        void backward_solve_gradient(int);

};

#endif
