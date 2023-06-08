import os
import signal
import json
import subprocess
import time
import re
import matplotlib.pyplot as plt

def plot_ebn0_ber(filename, inf):
    ebn0 = []
    ber = []
    for line in inf:
        print(line.strip('\n'))
        m = re.match(r'EbN0 = (.*?) dB, BER = (.*)', line.strip())
        if m:
            ebn0.append(float(m.group(1)))
            ber.append(float(m.group(2)))
    plt.clf()
    plt.plot(ebn0, ber, 'ko-')
    plt.yscale('log')
    plt.ylim([1e-5, 1])
    plt.xlim([0, 20])
    plt.ylabel('BER')
    plt.xlabel('EbN0 (dB)')
    plt.tight_layout()
    plt.savefig(filename)

def main():
    tests = [
        ('uart.trivial', 1, False),
        ('uart.unsync', 1, False),
        ('uart.noisy', 1, False),
        ('uart.noisy_unsync', 1, False),
        ('v21.sync', 2, True),
        ('v21.unsync', 2, True),
    ]
    scores = {}

    for test, weight, do_plot in tests:
        scores[test] = 0

        timeout = 60
        kwargs={'encoding': 'utf-8'}
        if do_plot:
            kwargs['stdout'] = subprocess.PIPE
        p = subprocess.Popen(['./grader/test', f'--gtest_filter={test}'],
                             **kwargs)
        try:
            if do_plot:
                plot_ebn0_ber(f'../artifacts/{test}.pdf', p.stdout)
            if p.wait(timeout=timeout) == 0:
                scores[test] = weight
                print('OK')
        except subprocess.TimeoutExpired:
            print('%s: TIMEOUT (%.3f s)' % (test, timeout))
            os.kill(p.pid, signal.SIGINT)

    print(json.dumps({'scores':scores}))


if __name__ == '__main__':
    main()
