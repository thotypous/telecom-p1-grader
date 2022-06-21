#!/usr/bin/env python3
import os
import sys
import json
import random
import numpy as np
from scipy.optimize import curve_fit
from modem import Modem


def testar_modulacao(verificar_silencio=False):
    for fs in (48000, 44100):
        bufsz = fs//300
        print(f'testando com fs={fs}, bufsz={bufsz}, ans=False', file=sys.stderr)
        modem = Modem(fs, bufsz, ans=False)
        if not test_fsk_tx_tones(modem, fs, bufsz, 2*np.pi*(1080 + 100), 2*np.pi*(1080 - 100), verificar_silencio):
            print(f'falhou no teste com fs={fs}, bufsz={bufsz}, ans=False', file=sys.stderr)
            return False
        print(f'testando com fs={fs}, bufsz={bufsz}, ans=True', file=sys.stderr)
        modem = Modem(fs, bufsz, ans=True)
        if not test_fsk_tx_tones(modem, fs, bufsz, 2*np.pi*(1750 + 100), 2*np.pi*(1750 - 100), verificar_silencio):
            print(f'falhou no teste com fs={fs}, bufsz={bufsz}, ans=True', file=sys.stderr)
            return False
    return True


def test_fsk_tx_tones(modem, fs, bufsz, omega0, omega1, verificar_silencio):
    for i in range(random.randint(5, 10)):
        # alimenta o modem com uma quantidade aleatória de listas contendo bits aleatórios
        all_bits = []
        for _ in range(random.randint(5, 10)):
            bits = []
            for _ in range(random.randint(0, 12)):
                bits.append(os.urandom(1)[0] & 1)
            modem.put_bits(bits)
            all_bits.extend(bits)

        if len(all_bits) == 0:
            continue

        samples = modem.get_samples()
        if i == 0 or (not verificar_silencio):
            # obtém fase inicial assumindo que está usando a frequência certa no primeiro bit
            omega = omega1 if all_bits[0] else omega0
            func = lambda t, phi: np.sin(omega*t/fs + phi)
            params, _ = curve_fit(func, np.arange(0, len(samples)), samples)
            phi, = params
            print(f'fase inicial: {phi}', file=sys.stderr)

        desvios = 0

        while len(all_bits) > 0:
            bit = all_bits[0]
            all_bits = all_bits[1:]

            if len(samples) != bufsz:
                print('modem não respeitou o bufsz requisitado', file=sys.stderr)
                return False
            for sample in samples:
                if abs(np.sin(phi) - sample) > 1e-8:
                    desvios += 1
                phi += (omega1 if bit else omega0)/fs
            
            samples = modem.get_samples()

        if desvios != 0:
            print(f'modem desviou {desvios} vezes de uma senóide pura', file=sys.stderr)
            return False

        if verificar_silencio:
            for sample in samples:
                if abs(np.sin(phi) - sample) > 1e-8:
                    print('modem não produziu tom de marca (bit 1) quando não tinha bits disponíveis para serem modulados', file=sys.stderr)
                    return False
                phi += omega1/fs

    return True


def main():
    scores = {}
    
    print('=> testando modulação', file=sys.stderr)
    scores['modulacao'] = 1*testar_modulacao(verificar_silencio=False)

    print('=> testando modulação com intervalos de ociosidade', file=sys.stderr)
    scores['modulacao-com-ociosidade'] = 1*testar_modulacao(verificar_silencio=True)
    
    print(json.dumps({'scores':scores}))


if __name__ == '__main__':
    main()
