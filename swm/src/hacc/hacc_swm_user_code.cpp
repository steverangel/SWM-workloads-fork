
#include "hacc_swm_user_code.h"

static std::string expand_environment_variables( std::string s )
{
    if ( s.find( "${" ) == std::string::npos )
    {
        return s;
    }
 
    std::string pre  = s.substr( 0, s.find( "${" ) );
    std::string post = s.substr( s.find( "${" ) + 2 );
 
    if ( post.find( '}' ) == std::string::npos )
    {
        return s;
    }
 
    std::string variable = post.substr( 0, post.find( '}' ) );
    std::string value    = "";
 
    post = post.substr( post.find( '}' ) + 1 );
 
    if ( getenv( variable.c_str() ) != NULL )
    {
        value = std::string( getenv( variable.c_str() ) );
    }
 
    return expand_environment_variables( pre + value + post );
}

HACCSWMUserCode::HACCSWMUserCode(
        boost::property_tree::ptree cfg,
        void**& generic_ptrs
        ) :
    process_cnt(cfg.get<uint32_t>("jobs.size", 1)),
    request_vc(cfg.get<SWM_VC>("jobs.cfg.request_vc", 0)), 
    response_vc(cfg.get<SWM_VC>("jobs.cfg.response_vc", 4)),
    pkt_rsp_bytes(cfg.get<uint32_t>("jobs.cfg.pkt_rsp_bytes", 0)),
    gen_cfg_filename(cfg.get<std::string>("jobs.cfg.gen_cfg_filename")),
    cfg(cfg)
{
    process_id = *((int*)generic_ptrs[0]);

    //for (int i=0; i<process_cnt; ++i) {
    //  if (process_id==i)
    //    std::cout << "Hello from rank:" << process_id << std::endl;
    //}
    //std::cerr << "Got filename: " << gen_cfg_filename << std::endl;

    //here we should parse the json pointed to by gen_cfg_filename and do whatever with it...
    std::ifstream gen_cfg_file;

    // in case there are environment paths in the variable...
    //gen_cfg_filename = expand_environment_variables(gen_cfg_filename);

    gen_cfg_file.open(gen_cfg_filename);
    //ASSERT(gen_cfg_file.is_open(), "Could not open gen_cfg_file: " << gen_cfg_filename << std::endl);

    //boost::property_tree::ptree gen_cfg;
    std::stringstream ss;
    std::string line;

    while(getline(gen_cfg_file, line)) {
        ss << line;
        //std::cout << line << std::endl;
    }

    boost::property_tree::read_json(ss, gen_cfg);

    nranks = gen_cfg.get<int>("nranks");
    assert(process_cnt==nranks);  // these should match!

    ng = gen_cfg.get<int>("ng");
    do_swfft = gen_cfg.get<bool>("do_swfft");
    box_length = do_swfft ? 0.f : gen_cfg.get<double>("box_length");

    assert(sscanf(gen_cfg.get<std::string>("rank_shape_3d").c_str(), "(%d, %d, %d)", &(rank_shape_3d[0]), &(rank_shape_3d[1]), &(rank_shape_3d[2])) == 3);
    assert(sscanf(gen_cfg.get<std::string>("rank_shape_2d_x").c_str(), "(%d, %d, %d)", &(rank_shape_2d_x[0]), &(rank_shape_2d_x[1]), &(rank_shape_2d_x[2])) == 3);
    assert(sscanf(gen_cfg.get<std::string>("rank_shape_2d_y").c_str(), "(%d, %d, %d)", &(rank_shape_2d_y[0]), &(rank_shape_2d_y[1]), &(rank_shape_2d_y[2])) == 3);
    assert(sscanf(gen_cfg.get<std::string>("rank_shape_2d_z").c_str(), "(%d, %d, %d)", &(rank_shape_2d_z[0]), &(rank_shape_2d_z[1]), &(rank_shape_2d_z[2])) == 3);

    if (process_id==0) {
      if (do_swfft)
        printf("Running SWFFT\n");
      else
        printf("Running HACC\n");
      printf("ng: %d\n", ng);
      printf("nranks: %d\n", nranks);
      if (!do_swfft)
        printf("box_length: %g\n", box_length);

      for(int i=0; i<3; i++) printf("rank_shape_3d[%d]: %d\n", i, rank_shape_3d[i]);
      for(int i=0; i<3; i++) printf("rank_shape_2d_x[%d]: %d\n", i, rank_shape_2d_x[i]);
      for(int i=0; i<3; i++) printf("rank_shape_2d_y[%d]: %d\n", i, rank_shape_2d_y[i]);
      for(int i=0; i<3; i++) printf("rank_shape_2d_z[%d]: %d\n", i, rank_shape_2d_z[i]);
    }
    
    gen_cfg_file.close();

}

void
HACCSWMUserCode::call() {

    //SWM_Init(); // this creates symbol undefined 

    // Perf model parameters
    const double ninteractions_per_rank_mean  = 1e10;
    const double ninteractions_per_rank_delta = 0.01;
    const double ninteractions_per_rank_per_wallsecond = 1e9;
    
    const double buffer_copy_MBps = 1000.0;
    const double fft_work_per_second = 1e9;

    bool enable_hacc_fft = cfg.get<bool>("enable_hacc_fft",true);
    bool enable_hacc_exchange = cfg.get<bool>("enable_hacc_exchange",true);
    bool enable_hacc_checksum = cfg.get<bool>("enable_hacc_checksum",true);

    // Configuration for this run
    HaccConfig config(
            ng,
            box_length,
            process_cnt,
            process_id,
            rank_shape_3d,
            rank_shape_2d_x,
            rank_shape_2d_y,
            rank_shape_2d_z,
            request_vc,
            response_vc,
            pkt_rsp_bytes
            );

    // Assemble timestep model
    timestep = new HaccTimestep (
            config,

            ninteractions_per_rank_mean,
            ninteractions_per_rank_delta,
            ninteractions_per_rank_per_wallsecond,

            buffer_copy_MBps,
            fft_work_per_second,

            enable_hacc_fft,
            enable_hacc_exchange,
            enable_hacc_checksum
            );


    if (do_swfft)
      timestep->do_swfft();
    else
      timestep->do_steps();

    SWM_Finalize();
}
