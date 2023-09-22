// execute:
//     g++ -g -Wall -lsystemc -m64 -pthread main.cpp
//         -L/$(your systemc path)/lib-linux64
//         -I/$(your systemc path)/include  -I/$(your systemc
//         path)/src/tlm_utils -o sim

#include <systemc>

#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/peq_with_get.h"

class MyInitiator_Nb : public sc_core::sc_module
{
public:
  SC_HAS_PROCESS(MyInitiator_Nb); // must have this line
  explicit MyInitiator_Nb(sc_core::sc_module_name name)
      : sc_core::sc_module(name),
        m_initiator_port("initiator_port"), m_test_peq("peq")
  {
    m_initiator_port.register_nb_transport_bw(this, &MyInitiator_Nb::nb_transport_bw_func);
    cb = nullptr;
    SC_THREAD(SendReqThread);
    SC_THREAD(SendEndRespThread);
    sensitive << m_test_peq.get_event(); // sensitive event list
  }

  void SendReqThread()
  {
    int t_cycle_cnt = 0;
    tlm::tlm_phase t_phase = tlm::BEGIN_REQ;
    sc_core::sc_time t_delay = sc_core::SC_ZERO_TIME;
    while (1)
    {
      t_cycle_cnt++;
      t_delay = sc_core::sc_time(t_cycle_cnt, sc_core::SC_NS);
      tlm::tlm_generic_payload *t_payload = new tlm::tlm_generic_payload();
      t_payload->set_address(0x10000 + t_cycle_cnt);
      std::cout << this->name() << "\033[34m [" << sc_core::sc_time_stamp() << "]"
                << " call nb_transport_fw, BEGIN_REQ phase, addr=0x" << std::hex << t_payload->get_address()
                << " delay cycle " << t_cycle_cnt
                << " \033[0m" << std::endl;
      m_initiator_port->nb_transport_fw(*t_payload, t_phase, t_delay);

      /***********************************/
      // here can add your block logic,
      // basic block logic: need wait recv END_REQ phase
      wait(m_slv_end_req_evt);
      /***********************************/

      wait(1, sc_core::SC_NS);
    }
  }

  virtual tlm::tlm_sync_enum nb_transport_bw_func(tlm::tlm_generic_payload &payload,
                                          tlm::tlm_phase &phase, sc_core::sc_time &delay)
  {
    switch (phase)
    {
    case tlm::END_REQ:
      std::cout << this->name()
                << "\033[35m [" << sc_core::sc_time_stamp() << "]"
                << " nb_transport_bw_func recv END_REQ phase, addr=0x" << std::hex << payload.get_address()
                << " \033[0m" << std::endl;
      m_slv_end_req_evt.notify();
      if (cb != nullptr) cb(payload.get_address());
      break;
    case tlm::BEGIN_RESP:
      std::cout << this->name() << "\033[35m [" << sc_core::sc_time_stamp() << "]"
                << " nb_transport_bw_func recv BEGIN_RESP phase, addr=0x" << std::hex << payload.get_address()
                << " \033[0m" << std::endl;
      m_test_peq.notify(payload);
      break;
    default:
      assert(false);
    }
    return tlm::TLM_ACCEPTED;
  }

  void SendEndRespThread()
  {
    tlm::tlm_generic_payload *t_get = NULL;
    tlm::tlm_phase t_phase = tlm::END_RESP;
    sc_core::sc_time t_delay = sc_core::SC_ZERO_TIME;
    while (1)
    {
      wait(); // wait sensitive event list

      // here must get next transaction entil t_get is NULL
      while ((t_get = m_test_peq.get_next_transaction()) != NULL)
      {
        std::cout << this->name() << "\033[34m [" << sc_core::sc_time_stamp() << "]"
                  << " call nb_transport_fw, END_RESP phase, addr=0x" << std::hex << t_get->get_address()
                  << " \033[0m" << std::endl;
        m_initiator_port->nb_transport_fw(*t_get, t_phase, t_delay);
        t_get = NULL;

        // in this block, must can't wait any event or cycle delay
        //  if not, the time of transaction obtained will not accurate
      }
    }
  }

  sc_core::sc_event m_slv_end_req_evt;
  tlm_utils::simple_initiator_socket<MyInitiator_Nb> m_initiator_port;
  tlm_utils::peq_with_get<tlm::tlm_generic_payload> m_test_peq;
  void(*cb)(uint64_t);
};

class MyTarget_Nb : public sc_core::sc_module
{
public:
  SC_HAS_PROCESS(MyTarget_Nb); // must have this line
  explicit MyTarget_Nb(sc_core::sc_module_name name)
      : sc_core::sc_module(name), m_target_port("target_port_nb")
  {
    m_target_port.register_nb_transport_fw(this, &MyTarget_Nb::nb_transport_fw_func);
    cb = nullptr;

    SC_THREAD(MainThread);
    SC_THREAD(BeginRespThread);
  }

  virtual tlm::tlm_sync_enum nb_transport_fw_func(tlm::tlm_generic_payload &payload,
                                          tlm::tlm_phase &phase, sc_core::sc_time &delay)
  {
    wait(delay); // here can add wait logic, also can delete wait & ignore the delay
    switch (phase)
    {
    case tlm::BEGIN_REQ:
      m_req_fifo.write(&payload);
      std::cout << this->name()
                << "\033[35m [" << sc_core::sc_time_stamp() << "]"
                << " nb_transport_fw_func recv BEGIN_REQ phase, addr=0x" << std::hex << payload.get_address()
                << " \033[0m" << std::endl;
      break;
    case tlm::END_RESP:
      std::cout << this->name() << "\033[35m [" << sc_core::sc_time_stamp() << "]"
                << " nb_transport_fw_func recv END_RESP phase, addr=0x" << std::hex << payload.get_address()
                << " \033[0m\n"
                << std::endl;
      if (cb != nullptr) cb(payload.get_address());
      break;
    default:
      assert(false);
    }
    return tlm::TLM_ACCEPTED;
  }

  void MainThread()
  {
    tlm::tlm_phase t_phase = tlm::END_REQ;
    sc_core::sc_time t_delay = sc_core::SC_ZERO_TIME;
    while (1)
    {
      tlm::tlm_generic_payload *t_payload = m_req_fifo.read();

      /***********************************/
      // here can add your block logic, for back pressure use
      /***********************************/

      std::cout << this->name() << "\033[36m [" << sc_core::sc_time_stamp() << "]"
                << " call nb_transport_bw, END_REQ phase, addr=0x" << std::hex << t_payload->get_address()
                << " \033[0m" << std::endl;
      m_target_port->nb_transport_bw(*t_payload, t_phase, t_delay);

      /***********************************/
      // after END_REQ phase, indicate slave recv req successfully,
      // then handle req, return BEGIN_RESP
      /***********************************/
      m_resp_fifo.write(t_payload);

      wait(1, sc_core::SC_NS);
    }
  }

  void BeginRespThread()
  {
    tlm::tlm_phase t_phase = tlm::BEGIN_RESP;
    // sc_core::sc_time t_delay = sc_core::SC_ZERO_TIME;
    sc_core::sc_time t_delay = sc_core::sc_time(1, sc_core::SC_NS);
    while (1)
    {
      tlm::tlm_generic_payload *t_payload = m_resp_fifo.read();
      std::cout << this->name() << "\033[32m [" << sc_core::sc_time_stamp() << "]"
                << " call nb_transport_bw, BEGIN_RESP phase, addr=0x" << std::hex << t_payload->get_address()
                << " \033[0m" << std::endl;
      m_target_port->nb_transport_bw(*t_payload, t_phase, t_delay);

      wait(1, sc_core::SC_NS);
    }
  }

  tlm_utils::simple_target_socket<MyTarget_Nb> m_target_port;
  sc_core::sc_fifo<tlm::tlm_generic_payload *> m_req_fifo;
  sc_core::sc_fifo<tlm::tlm_generic_payload *> m_resp_fifo;
  void(*cb)(uint64_t);
};

class MyTop : public sc_core::sc_module
{
public:
  SC_HAS_PROCESS(MyTop);
  explicit MyTop(sc_core::sc_module_name name)
      : sc_core::sc_module(name)
        // m_init("init"),
        // m_target("targ")
  {
    // m_init.m_initiator_port.bind(m_target.m_target_port);
  }

  // MyInitiator_Nb m_init;
  // MyTarget_Nb m_target;
};

int sc_main(int argc, char **argv)
{
  MyTop m_top_module("my_top_module_nb");
  sc_core::sc_start(20, sc_core::SC_NS);
  return 0;
}

