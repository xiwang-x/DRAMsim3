#ifndef __CPU_H
#define __CPU_H

#include <fstream>
#include <functional>
#include <random>
#include <string>
#include "memory_system.h"

//#define DEBUG


extern int cfd;
extern int mfd;
extern int END;
namespace dramsim3 {

class CPU {
   public:
    CPU(const std::string& config_file, const std::string& output_dir)
        : memory_system_(
              config_file, output_dir,
              std::bind(&CPU::ReadCallBack, this, std::placeholders::_1),
              std::bind(&CPU::WriteCallBack, this, std::placeholders::_1)),
          clk_(0) {}
    virtual void ClockTick() = 0;
    void ReadCallBack(uint64_t addr) { return; }
    void WriteCallBack(uint64_t addr) { return; }
    void PrintStats() { memory_system_.PrintStats(); }
		void EndPipe(){close(mfd);close(cfd);};

   protected:
    MemorySystem memory_system_;
    uint64_t clk_;
};

class RandomCPU : public CPU {
   public:
    using CPU::CPU;
    void ClockTick() override;

   private:
    uint64_t last_addr_;
    bool last_write_ = false;
    std::mt19937_64 gen;
    bool get_next_ = true;
};

class StreamCPU : public CPU {
   public:
    using CPU::CPU;
    void ClockTick() override;

   private:
    uint64_t addr_a_, addr_b_, addr_c_, offset_ = 0;
    std::mt19937_64 gen;
    bool inserted_a_ = false;
    bool inserted_b_ = false;
    bool inserted_c_ = false;
    const uint64_t array_size_ = 2 << 20;  // elements in array
    const int stride_ = 64;                // stride in bytes
};

class TraceBasedCPU : public CPU {
   public:
    TraceBasedCPU(const std::string& config_file, const std::string& output_dir,
                  const std::string& trace_file,
									const std::string& response_trace);
    ~TraceBasedCPU() { 
		trace_file_.close(); 
		}
    void ClockTick() override;
		//pipe related func
    int InitPipe();
    void EndPipe();
    int ReadPipe();
    int WritePipe(uint64_t addr, uint64_t cycle);

    //pipe related data
    //
    //Send buffers
    //char  cmd_send[7];
    //char  cycle_send[18];
    char  addr_send[18+18+7];

    //Recv buffers
    //char  cmd_recv[7];
    //char  cycle_recv[18];
    char  addr_recv[18+18+7];


		//int mfd;
		//int cfd;
    std::ifstream cfd_pipe_;
   private:
    std::ifstream trace_file_;
    Transaction trans_;
    bool get_next_ = true;
};

}  // namespace dramsim3
#endif
