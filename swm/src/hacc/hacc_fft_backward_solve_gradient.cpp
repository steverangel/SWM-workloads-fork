
#include "hacc_fft_backward_solve_gradient.h"
#include  "swm-include.h"

HaccFFTBackwardSolveGradient::HaccFFTBackwardSolveGradient(

            HaccConfig & config,
            double buffer_copy_MBps,
            double fft_work_per_second
            ) :
    HaccFFT(
            config,
            buffer_copy_MBps,
            fft_work_per_second
           )
{}

//BOZO w/ axis
void
HaccFFTBackwardSolveGradient::backward_solve_gradient(int axis) {
        kspace_solve_gradient(axis);
        do_fft_compute(2);
        distribution_2_to_3(2);
        distribution_3_to_2(1);
        do_fft_compute(1);
        distribution_2_to_3(1);
        distribution_3_to_2(0);
        do_fft_compute(0);
        distribution_2_to_3(0);
}
