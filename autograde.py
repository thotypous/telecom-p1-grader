#!/usr/bin/env python3
import os
import sys
import json
import random
import numpy as np
from scipy.optimize import curve_fit
from scipy.interpolate import interp1d
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


def test_rx_vary_fs_side(snr_db=None, timing_offset=None, tudo_de_uma_vez=False):
    ber_arr = []
    for fs in (48000, 44100):
        for side in False, True:
            ber_arr.append(test_rx(fs, side, snr_db, timing_offset, tudo_de_uma_vez))
    return max(ber_arr)


def test_rx(fs, side=False, snr_db=None, timing_offset=None, tudo_de_uma_vez=False):
    bufsz = fs//300
    print(f'testando com fs={fs}, bufsz={bufsz}, side={side}, snr_db={snr_db}, timing_offset={timing_offset}', file=sys.stderr)

    modulador = Modem(fs, bufsz, ans=side)
    demodulador = Modem(fs, bufsz, ans=not side)

    bits = [os.urandom(1)[0] & 1 for _ in range(random.randint(1000, 2000))]

    # adiciona um preâmbulo conhecido para dar tempo do demodulador se ajustar
    bits = [1,1,1,1,0] + bits
    modulador.put_bits(bits)
    signal = np.concatenate([modulador.get_samples() for _ in range(len(bits))])

    if snr_db is not None:
        signal = add_awgn(signal, snr_db)
    if timing_offset is not None:
        signal = add_timing_offset(signal, timing_offset)

    if tudo_de_uma_vez:
        demodulador.put_samples(signal)
        demod_bits = demodulador.get_bits()
    else:
        demod_bits = []
        for i in range(0, len(signal), bufsz):
            demodulador.put_samples(signal[i:i+bufsz])
            demod_bits.extend(demodulador.get_bits())

    def remove_preamble(arr):
        i = 0
        while i < len(arr) and arr[i] == 1:
            i += 1
        return arr[i+1:]

    bits = remove_preamble(bits)
    demod_bits = remove_preamble(demod_bits)

    l = min(len(bits), len(demod_bits))
    missed_err = max(len(bits)-l, len(demod_bits)-l)
    incorrect_err = sum(abs(np.array(bits[:l]) - np.array(demod_bits[:l])))
    ber = (missed_err + incorrect_err)/len(bits)

    print(f"total miss={missed_err}, total incorrect={incorrect_err}, BER={ber}", file=sys.stderr)
    return ber


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
                if abs(np.sin(phi) - sample) > 6e-8:
                    desvios += 1
                phi += (omega1 if bit else omega0)/fs

            samples = modem.get_samples()

        if desvios != 0:
            print(f'modem desviou {desvios} vezes de uma senóide pura', file=sys.stderr)
            return False

        if verificar_silencio:
            for sample in samples:
                if abs(np.sin(phi) - sample) > 6e-8:
                    print('modem não produziu tom de marca (bit 1) quando não tinha bits disponíveis para serem modulados', file=sys.stderr)
                    return False
                phi += omega1/fs

    return True


def add_awgn(signal, target_snr_db):
    # https://stackoverflow.com/a/53688043
    sig_avg_watts = np.mean(signal**2)
    sig_avg_db = 10*np.log10(sig_avg_watts)
    noise_avg_db = sig_avg_db - target_snr_db
    noise_avg_watts = 10 ** (noise_avg_db / 10)
    return signal + np.random.normal(0, np.sqrt(noise_avg_watts), len(signal))


def add_timing_offset(signal, period_ratio_offset):
    x = np.arange(0, len(signal), 1)
    new_x = np.arange(0, len(signal), 1+period_ratio_offset)
    return interp1d(x, signal, fill_value='extrapolate')(new_x)



def main():
    scores = {}

    print('=> testando modulação', file=sys.stderr)
    scores['modulacao'] = 1 if testar_modulacao(verificar_silencio=False) else 0

    print('=> testando modulação com intervalos de ociosidade', file=sys.stderr)
    scores['modulacao-com-ociosidade'] = 1 if testar_modulacao(verificar_silencio=True) else 0

    if scores['modulacao'] < 1 or scores['modulacao-com-ociosidade'] < 1:
        print('arrume primeiro o seu modulador para conseguirmos testar o seu demodulador!', file=sys.stderr)
        print(json.dumps({'scores':scores}))
        return

    print('=> testando demodulação sem ruído nem offset de temporização, sinal todo enviado de uma vez (esperado BER < 0.0011)', file=sys.stderr)
    scores['demod-sinal-limpo-tudo-junto'] = 1 if test_rx_vary_fs_side(tudo_de_uma_vez=True) < 0.0011 else 0

    print('=> testando demodulação sem ruído nem offset de temporização (esperado BER < 0.0011)', file=sys.stderr)
    scores['demod-sinal-limpo'] = 1 if test_rx_vary_fs_side() < 0.0011 else 0

    print('=> testando demodulação com um pouco de ruído mas sem offset de temporização (esperado BER < 0.0011)', file=sys.stderr)
    scores['demod-soh-ruido'] = 1 if test_rx_vary_fs_side(snr_db=6) < 0.0011 else 0

    print('=> testando demodulação sem ruído mas com offset de temporização (esperado BER < 0.0011)', file=sys.stderr)
    scores['demod-soh-offset'] = 1 if test_rx_vary_fs_side(timing_offset=0.005) < 0.0011 else 0

    print('=> testando demodulação com um pouco de ruído e com offset de temporização (esperado BER < 0.0011)', file=sys.stderr)
    scores['demod-ruido-e-offset'] = 1 if test_rx_vary_fs_side(snr_db=6, timing_offset=0.005) < 0.0011 else 0

    print('=> testando demodulação com muito ruído e pouco offset de temporização (esperado BER < 0.01)', file=sys.stderr)
    scores['demod-muito-ruido-pouco-offset'] = 1 if test_rx_vary_fs_side(snr_db=-6, timing_offset=1e-5) < 0.01 else 0

    print(json.dumps({'scores':scores}))


if __name__ == '__main__':
    main()
