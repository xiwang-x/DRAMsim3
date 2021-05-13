#include "cpu.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sstream>

   #include <signal.h>
       #include <poll.h>


int cfd = 0;
int mfd = 0;
namespace dramsim3 {

bool is_empty(std::ifstream& pFile)
{
    return pFile.peek() == std::ifstream::traits_type::eof();
}



void RandomCPU::ClockTick() {
    // Create random CPU requests at full speed
    // this is useful to exploit the parallelism of a DRAM protocol
    // and is also immune to address mapping and scheduling policies
    memory_system_.ClockTick();
    if (get_next_) {
        last_addr_ = gen();
        last_write_ = (gen() % 3 == 0);
    }
    get_next_ = memory_system_.WillAcceptTransaction(last_addr_, last_write_);
    if (get_next_) {
        memory_system_.AddTransaction(last_addr_, last_write_);
    }
    clk_++;
    return;
}

void StreamCPU::ClockTick() {
    // stream-add, read 2 arrays, add them up to the third array
    // this is a very simple approximate but should be able to produce
    // enough buffer hits

    // moving on to next set of arrays
    memory_system_.ClockTick();
    if (offset_ >= array_size_ || clk_ == 0) {
        addr_a_ = gen();
        addr_b_ = gen();
        addr_c_ = gen();
        offset_ = 0;
    }

    if (!inserted_a_ &&
        memory_system_.WillAcceptTransaction(addr_a_ + offset_, false)) {
        memory_system_.AddTransaction(addr_a_ + offset_, false);
        inserted_a_ = true;
    }
    if (!inserted_b_ &&
        memory_system_.WillAcceptTransaction(addr_b_ + offset_, false)) {
        memory_system_.AddTransaction(addr_b_ + offset_, false);
        inserted_b_ = true;
    }
    if (!inserted_c_ &&
        memory_system_.WillAcceptTransaction(addr_c_ + offset_, true)) {
        memory_system_.AddTransaction(addr_c_ + offset_, true);
        inserted_c_ = true;
    }
    // moving on to next element
    if (inserted_a_ && inserted_b_ && inserted_c_) {
        offset_ += stride_;
        inserted_a_ = false;
        inserted_b_ = false;
        inserted_c_ = false;
    }
    clk_++;
    return;
		
}

TraceBasedCPU::TraceBasedCPU(const std::string& config_file,
                             const std::string& output_dir,
                             const std::string& trace_file,
                             const std::string& response_file)
    : CPU(config_file, output_dir) {
    //trace_file_.open(trace_file);
		//char* temp = &trace_file[0];
		const char* rqst = trace_file.c_str();
    //mfd = open(temp, O_RDONLY  | O_NONBLOCK);
    mfd = open(rqst, O_RDONLY  | O_NONBLOCK);
    if (mfd == -1) {
        std::cerr << "Trace file does not exist" << std::endl;
        AbruptExit(__FILE__, __LINE__);
    }

#ifdef DEBUG
	  std::cout<< "DRAMSIM::MFD (Memory Pipe) "<<rqst << " Opened in DRAMSIM\n";
#endif


		const char* resp = response_file.c_str();
		//char pipe[20] = "./pipe_to_cpu";
    cfd = open( resp, O_WRONLY | O_NONBLOCK);
    while(cfd == -1) {
    		cfd = open(resp, O_WRONLY | O_NONBLOCK);
        //std::cerr << "Trace file does not exist" << std::endl;
        //AbruptExit(__FILE__, __LINE__);
    }
	  //std::cout<< "DRAMSIM::CFD: " <<resp <<"\n";
#ifdef DEBUG
	  std::cout<< "DRAMSIM::CFD (CPU Pipe) " << resp <<" Opened in DRAMSIM\n";
#endif
}

/*int TraceBasedCPU::WritePipe(uint64_t addr, uint64_t cycle){
      int len = 0;
      sprintf( addr_send, "%016lx", addr );
      sprintf( cycle_send, "%lu", cycle );
			strcat(addr_send, cycle_send);
      write( cfd, addr_send, strlen(addr_send) );
			//printf("addr_send len = %d\n", strlen(addr_send));


      return 0;

}*/


int TraceBasedCPU::ReadPipe(){
      int len = 0;

      if(read(mfd, addr_recv, 17+17+7) <= 0)
          return -1;
      len = strlen(addr_recv);
      if( addr_recv[len] == '\n' ){
          addr_recv[len] = '\0';
      }   

#ifdef DEBUG
   		printf ("DRAMSIM::Received Request:%s\n",addr_recv);
#endif			
			char * i;
			std::stringstream ss;
			std::string cmd;
			//printf ("Splitting string \"%s\" into tokens:\n",addr_recv);
  		i = strtok (addr_recv," ");
			//trans_.addr =(int64_t)i;
			trans_.addr = strtoull(i, NULL, 16);
    	i = strtok (NULL, " ");

    	//i = strtok (NULL, " ");
			ss << i;
			ss >> cmd;
			if(cmd == "END")
				END = 1;
			else if(cmd == "WRITE")
				trans_.is_write = true;
			else
				 trans_.is_write = false;

#ifdef DEBUG
			printf ("DRAMSIM::Command Token:%s\n", i);
#endif
    	i = strtok (NULL, " ");
			//trans_.added_cycle =(int64_t)i;
			trans_.added_cycle = strtoull(i, NULL, 0);
#ifdef DEBUG
			printf ("DRAMSIM::Address Token:%lu\n", trans_.addr);
      printf( "DRAMSIM::Cycle Token = %lu\n",trans_.added_cycle);
			if(trans_.added_cycle == 9999)
      printf( "DRAMSIM::All requests have been received!!!!\n");
#endif			

      /*if(read(mfd, cmdbuf, 6) <= 0)
          return -1; 
      len = strlen(addrbuf);
      if( cmdbuf[len] == '\n' ){
          cmdbuf[len] = '\0';
      }   
      if( strcmp( cmdbuf, " END " ) == 0 ){
          END = 1;
          return 0;
      }   


      if(read(mfd, cycle_recv, 17) <= 0)
          return -1;
      len = strlen(cycle);
      if( cycle[len] == '\n' ){
          cycle[len] = '\0';
      }*/
    
   		//std::cout << addr_recv<<std::endl; 
      //printf("%s %s %s\n",addrbuf, cmdbuf, cycle);  
      return 0;

}




void TraceBasedCPU::ClockTick() {
    memory_system_.ClockTick();
		//if (!END && poll(&(struct pollfd){ .fd = mfd, .events = POLLIN }, 1, 0)==1) {

    /* data available */
		if (!END ){
        if (get_next_) {
            //get_next_ = false;
#ifdef DEBUG
	  				std::cout<< "DRAMSIM::Enter ReadPipe()\n";
#endif
						if(ReadPipe()>=0)
            	get_next_ = false;
						else
							get_next_ = true;
#ifdef DEBUG
	  				std::cout<< "DRAMSIM::Complete ReadPipe()\n";
#endif
    				//std::cout << addr_recv << '\n';
            //trace_file_ >> trans_;


        }
				if(get_next_ == false){
        if (trans_.added_cycle <= clk_) {
            get_next_ = memory_system_.WillAcceptTransaction(trans_.addr,
                                                             trans_.is_write);
            if (get_next_) {
                memory_system_.AddTransaction(trans_.addr, trans_.is_write);
            }
        }
				}	

		}
		/*
    if (!trace_file_.eof()) {
        if (get_next_) {
            get_next_ = false;
            trace_file_ >> trans_;
        }
        if (trans_.added_cycle <= clk_) {
            get_next_ = memory_system_.WillAcceptTransaction(trans_.addr,
                                                             trans_.is_write);
            if (get_next_) {
                memory_system_.AddTransaction(trans_.addr, trans_.is_write);
            }
        }
    }*/
    clk_++;
    return;
}

}  // namespace dramsim3
