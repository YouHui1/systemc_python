import cppyy
import cppyy.ll
from cppyy import gbl as cpp
import mmap
import os
from colorama import Fore, Back, Style

cppyy.add_include_path('/usr/local/install/systemc-2.3.4/include')
cppyy.add_library_path('/usr/local/install/systemc-2.3.4/lib-linux64')
cppyy.load_library('systemc')

def block():
    with open('blocking.cpp', 'r') as file:
        s = file.read()
        cppyy.cppdef(s)
def nonblock():
    with open('nonblock.cpp', 'r') as file:
        s = file.read()
        cppyy.cppdef(s)


nonblock()

def pycb(addr):
    print(Fore.LIGHTYELLOW_EX + Back.LIGHTBLACK_EX + 'python response addr {}'.format(addr) + Fore.RESET + Back.RESET)

class Py_Initiator(cpp.MyInitiator_Nb):
    def __init__(self, name):
        super().__init__(name)
        self.cb = pycb
    # def nb_transport_bw_func(self, payload, phase, delay):
    #     cpp.MyInitiator_Nb.nb_transport_bw_func(self, payload, phase, delay)
        # print(Fore.LIGHTYELLOW_EX + Back.LIGHTBLACK_EX + 'python addr: {}'.format(payload.get_address()))
class Py_Target(cpp.MyTarget_Nb):
    def __init__(self, name):
        super().__init__(name)
        self.cb = pycb
    # def nb_transport_fw_func(self, payload, phase, delay):
    #     cpp.MyTarget_Nb.nb_transport_fw_func(self, payload, phase, delay)
        # print(Fore.LIGHTYELLOW_EX + Back.LIGHTBLACK_EX + 'python addr: {}'.format(payload.get_address()))
class Top(cpp.MyTop):
    def __init__(self, name):
        super().__init__(name)
        self.m_init = Py_Initiator('init')
        self.m_target = Py_Target('targ')
        self.m_init.m_initiator_port.bind(self.m_target.m_target_port);

if __name__ == '__main__':
    # nonblock()
    # print(cpp.sc_core.SC_NS)
    m_top_module = Top("my_top_module_nb");
    cpp.sc_core.sc_start(20, cpp.sc_core.SC_NS);
    pass
