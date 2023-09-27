import simpy
from colorama import Fore, Back, Style
import random
env = simpy.Environment()

class Socket:
    def __init__(self):
        self.data = None
        self.other_socket = None
        self.func = None

    def bind(self, other):
        if (self.other_socket is None and other.other_socket is None):
            self.other_socket = other
            other.other_socket = self
        else:
            print("bind error")
    def register_transport(self, func):
        self.func = func

    def transport(self, payload, delay):
        if self.other_socket.func is not None:
            self.other_socket.func(payload, delay)

class Module:
    def __init__(self, env, name):
        self.env = env
        self.name = name
        pass
    def __register(self, func):
        pass

class Initiator(Module):
    def __init__(self, env, name):
        super().__init__(env, name)
        self.socket = Socket()
        self.lst = [i for i in range(15)]
        self.thread_proc = env.process(self.main_thread(env))

    def main_thread(self, env):
        payload = {
            'command': random.randint(0, 1),
            'data_addr': self.lst,
            'len': len(self.lst)
        }
        delay = 1
        while True:
            print(Fore.LIGHTGREEN_EX + 'time: {} Initiator: transport'.format(env.now) + Fore.RESET)
            self.socket.transport(payload, delay)
            print(Fore.LIGHTGREEN_EX + 'time: {} Initiator: transport finished'.format(env.now) + Fore.RESET)
            print(Fore.LIGHTYELLOW_EX + 'data: {}'.format(self.lst) + Fore.RESET)
            print()

            yield env.timeout(delay)


class Target(Module):
    def __init__(self, env, name):
        super().__init__(env, name)
        self.socket = Socket()
        self.socket.register_transport(self.transport)
    def transport(self, payload, delay):
        cmd = payload['command']
        if cmd == 1:
            rand = random.randrange(0, payload['len'])
            payload['data_addr'][rand] = random.randint(20, 40)
        print(Fore.LIGHTRED_EX + 'Target get data' + Fore.RESET)



class Top(Module):
    def __init__(self, env, name):
        super().__init__(env, name)
        self.initiator = Initiator(env, "initiator")
        self.target = Target(env, "target")
        self.initiator.socket.bind(self.target.socket)

if __name__ == '__main__':
    t = Top(env, 'top')
    env.run(until=10)
