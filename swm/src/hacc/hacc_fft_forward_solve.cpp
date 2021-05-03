#include "hacc_fft_forward_solve.h"
#include "swm-include.h"

HaccFFTForwardSolve::HaccFFTForwardSolve(

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

void
HaccFFTForwardSolve::forward_solve() {
        distribution_3_to_2(0);
        do_fft_compute(0);
        distribution_2_to_3(0);
        distribution_3_to_2(1);
        do_fft_compute(1);
        distribution_2_to_3(1);
        distribution_3_to_2(2);
        do_fft_compute(2);

        //SWM_Compute(10000);
        //uint32_t rsp_bytes;
        //SWM_Allreduce(4, rsp_bytes, SWM_COMM_WORLD, config.request_vc, config.response_vc, NO_BUFFER, NO_BUFFER);
}
