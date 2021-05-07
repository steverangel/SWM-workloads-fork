
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include "hacc_timestep.h"
#include "swm-include.h"

HaccTimestep::HaccTimestep (

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
        ) :
    config(config),
    enable_hacc_fft(enable_hacc_fft),
    enable_hacc_exchange(enable_hacc_exchange),
    enable_hacc_checksum(enable_hacc_checksum)
{ 

    // Compute models
    rcb = new HaccComputeRCBTree (
            config,
            ninteractions_per_rank_mean,
            ninteractions_per_rank_delta,
            ninteractions_per_rank_per_wallsecond
            );

    fft_forward_solve = new HaccFFTForwardSolve (
            config,
            buffer_copy_MBps,
            fft_work_per_second
            );

    fft_backward_solve_gradient = new HaccFFTBackwardSolveGradient (
            config,
            buffer_copy_MBps,
            fft_work_per_second
            );

    exchange = new HaccExchange (
            config,
            buffer_copy_MBps
            );


    /*
    cerr << "Hacc sizes: " << endl;
    cerr << "Hacc pid: " << process->process_id << endl;
    cerr << "Hacc timestep: " << sizeof(HaccTimestep) << endl;
    cerr << "Hacc rcb: " << sizeof(HaccComputeRCBTree) << endl;
    cerr << "Hacc fft_forward_solve: " << sizeof(HaccFFTForwardSolve) << endl;
    cerr << "Hacc fft_backward_solve: " << sizeof(HaccFFTBackwardSolveGradient) << endl;
    cerr << "Hacc exchange : " << sizeof(HaccExchange) << endl;
    */
}

void
HaccTimestep::sub_cycle() {
	rcb->build_tree_and_evaluate_forces();
}

void
HaccTimestep::map2_poisson_forward() {
    // TODO:compute: forward CIC
    if (enable_hacc_fft)
    {
      fft_forward_solve->forward_solve();
    }
}

void
HaccTimestep::map2_poisson_backward_gradient() {
    for (int idim=0; idim<3; idim++) {
      if (enable_hacc_fft)
      {
        fft_backward_solve_gradient->backward_solve_gradient(idim);
      }

      if (enable_hacc_exchange)
      {
          // TODO:compute: FFT copy array backwards
          //    (*exchange)();
      }
    }
}

void
HaccTimestep::do_swfft() {
  fft_forward_solve->forward_solve();
  fft_backward_solve_gradient->backward_solve_gradient(0);
}

void
HaccTimestep::do_steps() {

    for (int istep=0; istep<nstep; istep++) {
        if (config.myrank==0)
          std::cout << "Start of timestep:" << istep << std::endl;
        
        // Half-kick at first step
        if (istep == 0) {
            map2_poisson_forward();
            map2_poisson_backward_gradient();
        }

        // Sub-stepping
        for (int isubstep=0;isubstep<nsub; isubstep++) {
            sub_cycle();
        }

        // FFT memory reallocation
        if (enable_hacc_fft && do_drop_memory) {
          // FFT memory reallocation will rebuild the solver, which calls
          // MPI_Cart_create.  MPI_Cart_create is a collective op which
          // contains an implicit synchronization and acts as a barrier,
          // so it's important to emulate its effect
          //backend.comm_emulate_cart_create();
          SWM_Barrier(
              SWM_COMM_WORLD,
              config.request_vc,
              config.response_vc,
              NO_BUFFER,
              AUTO,
              NULL,
              AUTOMATIC,
              AUTOMATIC
          );
        }

        // Checksum particles
        if (enable_hacc_checksum)
          SWM_Allreduce(
                8,  //msg_size
                config.pkt_rsp_bytes, // pkt_rsp_bytes
                SWM_COMM_WORLD,  //comm_id
                config.request_vc,  //reqvc
                config.response_vc,  //rspvc
                NO_BUFFER,  //sendbuf
                NO_BUFFER   //recvbuf
                );
        
        // Get rho into spectral domain
        map2_poisson_forward();
        
        // Checksum grid density
        if (enable_hacc_checksum)
          SWM_Allreduce(
                8,                    // msg_size
                config.pkt_rsp_bytes, // pkt_rsp_bytes
                SWM_COMM_WORLD,       // comm_id
                config.request_vc,    // reqvc
                config.response_vc,   // rspvc
                NO_BUFFER,            // sendbuf
                NO_BUFFER             // recvbuf
                );
       
        // P(k) computation may occur here, and does a couple of Allreduce
        // on arrays of doubles It seems P(k) is never computed within the
        // timesteps though, and the .ini and .fin power spectra are
        // computed outside of the time loop.
        
        // Solve gradient and back to real domain
        map2_poisson_backward_gradient();

        // AFAIK, this is never called
        // map2_poisson_backward_potential();

        if (enable_hacc_checksum)
        {
          for(int ar=0; ar<3; ar++) {
              SWM_Allreduce(
                      8,  //msg_size
                      config.pkt_rsp_bytes, // pkt_rsp_bytes
                      SWM_COMM_WORLD,  //comm_id
                      config.request_vc,  //reqvc
                      config.response_vc,  //rspvc
                      NO_BUFFER,  //sendbuf
                      NO_BUFFER   //recvbuf
                      );
          }
          SWM_Barrier(
                SWM_COMM_WORLD,
                config.request_vc,
                config.response_vc,
                NO_BUFFER,
                AUTO,
                NULL,
                AUTOMATIC,
                AUTOMATIC
                );
        }
    }
}
