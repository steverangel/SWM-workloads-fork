#ifndef _HACC_TIMESTEP_HPP
#define _HACC_TIMESTEP_HPP

#include "hacc_config.h"
#include "hacc_fft_backward_solve_gradient.h"
#include "hacc_fft_forward_solve.h"
#include "hacc_exchange.h"
#include "hacc_compute_rcbtree.h"

//#include "swm_user_code.h"

class HaccComputeRCBTree;

class HaccTimestep 
{

    public:

        HaccConfig & config;
        HaccComputeRCBTree * rcb;
        HaccFFTBackwardSolveGradient * fft_backward_solve_gradient;
        HaccFFTForwardSolve * fft_forward_solve;
        HaccExchange * exchange;

        HaccTimestep (

                HaccConfig & config,

                const double ninteractions_per_rank_mean,
                const double ninteractions_per_rank_delta,
                const double ninteractions_per_rank_per_wallsecond,

                const double buffer_copy_MBps,
                const double fft_work_per_second,

                const bool enable_hacc_fft,
                const bool enable_hacc_exchange,
                const bool enable_hacc_checksum

                /*
                HaccComputeRCBTree & rcb,
                HaccFFTBackwardSolveGradient & fft_backward,
                HaccFFTForwardSolve & fft_forward,
                HaccExchange & exchange
                */
                );
         //HaccTimestep () { do_steps();};

        ~HaccTimestep() {
            //std::cout << "~HaccTimestep()" << std::endl;
        }

        void sub_cycle();

        void map2_poisson_forward();

        void map2_poisson_backward_gradient();

        void do_steps();

        void do_swfft();

    private:
    
        // Some timestep-related parameters fixed by the CORAL testcase
        const int nstep = 1;//3;
        const int nsub  = 0;//5;
        const bool do_drop_memory = true;
        const bool enable_hacc_fft;
        const bool enable_hacc_exchange;
        const bool enable_hacc_checksum;

};

#endif
