// execute:
//     g++ -g -Wall -lsystemc -m64 -pthread main.cpp
//         -L/$(your systemc path)/lib-linux64
//         -I/$(your systemc path)/include  -I/$(your systemc
//         path)/src/tlm_utils -o sim

#include <systemc>

#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

class MyInitiator : public sc_core::sc_module
{
public:
  SC_HAS_PROCESS(MyInitiator); // must have this line
  explicit MyInitiator(sc_core::sc_module_name name)
      : sc_core::sc_module(name),
        m_initiator_port("initiator_port")
  {
    SC_THREAD(MainThread);
  }

  void MainThread()
  {
    int t_cycle_cnt = 0;
    sc_core::sc_time t_delay = sc_core::SC_ZERO_TIME;
    while (1)
    {
      t_cycle_cnt++;
      t_delay = sc_core::sc_time(t_cycle_cnt, sc_core::SC_NS);
      tlm::tlm_generic_payload *t_payload = new tlm::tlm_generic_payload();
      t_payload->set_address(0x10000 + t_cycle_cnt);
      std::cout << "\033[34m [" << sc_core::sc_time_stamp() << "]"
                << " call b_transport " << std::hex << t_payload->get_address()
                << " delay cycle " << t_cycle_cnt
                << " \033[0m" << std::endl;
      m_initiator_port->b_transport(*t_payload, t_delay); // here will call MyTarget::RecvReqFunc
      wait(1, sc_core::SC_NS);
    }
  }

  tlm_utils::simple_initiator_socket<MyInitiator> m_initiator_port;
};

class MyTarget : public sc_core::sc_module
{
public:
  SC_HAS_PROCESS(MyTarget); // must have this line
  explicit MyTarget(sc_core::sc_module_name name)
      : sc_core::sc_module(name), m_target_port("target_port")
  {
    m_target_port.register_b_transport(this, &MyTarget::RecvReqFunc);
  }

  //void at_target_2_phase::b_transport(tlm::tlm_generic_payload &payload, sc_core::sc_time &delay_time)
  void RecvReqFunc(tlm::tlm_generic_payload &payload, sc_core::sc_time &delay)
  {
    wait(delay);
    std::cout << "\033[35m [" << sc_core::sc_time_stamp() << "]"
              << " RecvReqFunc " << std::hex << payload.get_address() << " \033[0m" << std::endl;
  }

  tlm_utils::simple_target_socket<MyTarget> m_target_port;
};

class MyTop : public sc_core::sc_module
{
public:
  SC_HAS_PROCESS(MyTop);
  explicit MyTop(sc_core::sc_module_name name)
      : sc_core::sc_module(name),
        m_init("init"),
        m_target("target")
  {
    m_init.m_initiator_port.bind(m_target.m_target_port);
  }

  MyInitiator m_init;
  MyTarget m_target;
};

int sc_main(int argc, char **argv)
{
  MyTop m_top_module("my_top_module");
  sc_core::sc_start(20, sc_core::SC_NS);
  return 0;
}
