#!/usr/bin/env python3
import os
import sys
import json
import random
import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit
from modem import Modem


def test_fsk_tx():
    for fs in (48000, 44100):
        bufsz = fs//300
        modem = Modem(fs, bufsz, ans=False)
        if not test_fsk_tx_tones(modem, fs, bufsz, 2*np.pi*(1080 + 100), 2*np.pi*(1080 - 100)):
            print(f'failed test with fs={fs}, bufsz={bufsz}, ans=False', file=sys.stderr)
            return False
        modem = Modem(fs, bufsz, ans=True)
        if not test_fsk_tx_tones(modem, fs, bufsz, 2*np.pi*(1750 + 100), 2*np.pi*(1750 - 100)):
            print(f'failed test with fs={fs}, bufsz={bufsz}, ans=True', file=sys.stderr)
            return False
    return True


def test_fsk_tx_tones(modem, fs, bufsz, omega0, omega1, usar_silencio=False):
    # alimenta o modem com uma quantidade aleatória de listas contendo bits aleatórios
    all_bits = []
    for _ in range(random.randint(1, 3)):
        bits = []
        for _ in range(random.randint(8, 15)):
            bits.append(os.urandom(1)[0] & 1)
        modem.put_bits(bits)
        all_bits.extend(bits)

    samples = modem.get_samples()
    # obtém fase inicial assumindo que está usando a frequência certa no primeiro bit
    omega = omega1 if all_bits[0] else omega0
    def func(x, phi):
        return np.sin(omega*x)*np.cos(phi) + np.cos(omega*x)*np.sin(phi)
    params, covs = curve_fit(func, np.arange(0, len(samples))/fs, samples)
    phi, = params

    desvios = 0

    while len(all_bits) > 0:
        bit = all_bits[0]
        all_bits = all_bits[1:]

        if len(samples) != bufsz:
            print('modem não respeitou o bufsz requisitado', file=sys.stderr)
            return False
        for sample in samples:
            if abs(np.sin(phi) - sample) > 1e-12:
                desvios += 1
            phi += (omega1 if bit else omega0)/fs
        
        samples = modem.get_samples()

    if desvios != 0:
        print(f'modem desviou {desvios} vezes de uma senóide pura', file=sys.stderr)
        return False

    return True


def main():
    scores = {}
    scores['fsk-tx'] = 2*test_fsk_tx()
    print(json.dumps({'scores':scores}))


if __name__ == '__main__':
    main()
