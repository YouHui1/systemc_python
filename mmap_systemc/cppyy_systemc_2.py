import cppyy
import cppyy.ll
from cppyy import gbl as cpp
import mmap
import os


myDir = os.path.dirname( os.path.realpath(__file__))

cppyy.add_include_path('/usr/local/install/systemc-2.3.4/include')
cppyy.add_library_path('/usr/local/install/systemc-2.3.4/lib-linux64')
cppyy.load_library('systemc')

cppyy.cppdef('''
#include <systemc>
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string>
''')

cppyy.cppdef('''
using namespace sc_core;
using namespace sc_dt;
using namespace std;
''')

cppyy.cppdef('''
class Initiator : public sc_module {
public:
    string path;
    void* file_ptr;
    int fd;
public:
    SC_HAS_PROCESS(Initiator);
    tlm_utils::simple_initiator_socket<Initiator> socket;
    Initiator(const sc_module_name& name): socket("socket") {
        cout << "init" << endl;
        fd = open("data", O_RDWR);
        file_ptr = mmap(NULL, sizeof(int) * 20, PROT_READ, MAP_SHARED, fd, 0);
        if (file_ptr == (void*)-1) cout << "!!!" << endl;
        cout << "initfd " << fd << endl;
        if (fd == -1) {
            perror("open");
        }
        SC_THREAD(thread_process);
    }
    virtual ~Initiator() {
        munmap(file_ptr, 20 * sizeof(int));
        close(fd);
    }
    virtual void thread_process() {
        if (file_ptr == (void*)-1) {
            cout << "map failed" << endl;
            return;
        }
        tlm::tlm_generic_payload* trans = new tlm::tlm_generic_payload;
        sc_time delay = sc_time(10, SC_NS);

        tlm::tlm_command cmd = static_cast<tlm::tlm_command>(0);

        trans->set_command(cmd);
        trans->set_address(0);
        trans->set_data_ptr(reinterpret_cast<unsigned char*>(file_ptr));
        trans->set_data_length(sizeof(int) * 20);
        trans->set_streaming_width(sizeof(int) * 20);
        trans->set_byte_enable_ptr(0);
        trans->set_dmi_allowed(false);
        trans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        socket->b_transport(*trans, delay);

        if (trans->is_response_error()) {
            SC_REPORT_ERROR("TLM 2.0", "Response error from b_transport");
        }
        //...

        int recv_fd = open("ans", O_RDWR);
        cout << "recv_fd" << " " << recv_fd << endl;
        if (recv_fd == -1) {
            perror("open");
        }
        void* recv_ptr = mmap(NULL, sizeof(int) * 10, PROT_READ, MAP_SHARED, recv_fd, 0);
        int* recv_int = (int*) recv_ptr;
        for (int i = 0; i < 10; ++i) {
            cout << (*recv_int) << endl;
            recv_int++;
        }
        munmap(recv_ptr, sizeof(int) * 10);
        close(fd);
        // cout << "cpp_data: " << data[i] << endl;
        wait(delay);
    }
};

class Memory : public sc_module {
public:
  SC_HAS_PROCESS(Memory);
  tlm_utils::simple_target_socket<Memory> socket;
  enum {SIZE = 256};

    Memory(const sc_module_name& name) : socket("socket") {
        cout << "mem" << endl;
        socket.register_b_transport(this, &Memory::b_transport);

        fd = open("ans", O_CREAT | O_RDWR, 0664);
        ftruncate(fd, 10 * sizeof(int));
        cout << "memfd " << fd << endl;
        if (fd == -1) {
            perror("open");
        }
        file_ptr = mmap(NULL, sizeof(int) * 10, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (file_ptr == (void*)-1) cout << "!!!" << endl;
    }
    virtual ~Memory() {
    }
    virtual void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay){
        tlm::tlm_command  cmd = trans.get_command();
        // sc_dt::uint64     adr = trans.get_address() / 4;
        sc_dt::uint64     adr = trans.get_address();
        unsigned char *   ptr = trans.get_data_ptr();
        int* int_ptr = reinterpret_cast<int*>(ptr);
        unsigned int      len = trans.get_data_length();
        unsigned char *   byt = trans.get_byte_enable_ptr();
        unsigned int      wid = trans.get_streaming_width();
        if (adr >= sc_dt::uint64(SIZE) || byt != 0 || wid < len) {
            SC_REPORT_ERROR("TLM 2.0", "Target does not support given generic payload transaction");
        }

        int* addr = (int*)file_ptr;
        for (int i = 0; i < (len / 8); ++i) {
            cout << *((int*)int_ptr + i) << " " << *((int*)int_ptr + 10 + i) << endl;
            int x = *((int*)int_ptr + i) + *((int*)int_ptr + 10 + i);
            *addr = x;
            addr++;
        }
        munmap(file_ptr, 10 * sizeof(int));
        close(fd);

        trans.set_response_status(tlm::TLM_OK_RESPONSE);

    };

private:
    // vector<int> a, b;
    vector<int> ans;
    int fd;
    void* file_ptr;
};

class Top : public sc_module {
public:
  SC_HAS_PROCESS(Top);
   Initiator *initiator;
   Memory    *memory;
  Top(const sc_module_name& name){
     initiator = new Initiator("initiator");
     memory    = new Memory("memory");

     initiator->socket.bind( memory->socket );
  }
};
''')
def sc_start():
    cpp.sc_core.sc_start()

if __name__ == "__main__":
    top = cpp.Top("top")
    sc_start()
