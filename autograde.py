#!/usr/bin/env python3
import json
from modem import Modem

def test_fsk_tx():
    modem = Modem(48000, 48000//300, ans=False)
    return 0

def main():
    scores = {}
    scores['fsk-tx'] = 2*test_fsk_tx()
    print(json.dumps({'scores':scores}))


if __name__ == '__main__':
    main()
