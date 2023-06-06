import sys
import re
import matplotlib.pyplot as plt

ebn0 = []
ber = []
for line in sys.stdin:
    m = re.match(r'EbN0 = (.*?) dB, BER = (.*)', line.strip())
    if m:
        ebn0.append(float(m.group(1)))
        ber.append(float(m.group(2)))

plt.plot(ebn0, ber, 'ko-')
plt.yscale('log')
plt.ylim([1e-5, 1])
plt.xlim([0, 20])
plt.ylabel('BER')
plt.xlabel('EbN0 (dB)')
plt.tight_layout()
plt.show()
