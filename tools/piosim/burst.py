"""Per-position decode rate inside a back-to-back burst, one-way (receiver stays idle).

usage: burst.py <uart_duplex.pio | .pio.h>
"""
import sys, random
sys.path.insert(0, __import__('os').path.dirname(__file__) or '.')
import piosim
from piosim import SM, tx_frame, IDLE, frame_ok, frame_data
def burst_test(prog, n, trials=40, gap=12):
    pos_ok = [0]*n; pos_tot=[0]*n
    for tr in range(trials):
        rnd = random.Random(tr)
        A, B = SM(prog,'A'), SM(prog,'B')
        data = [rnd.randint(1,127) for _ in range(n)]
        q = [IDLE]*(rnd.randint(2,9)) + [tx_frame(c) for c in data] + [IDLE]*gap
        def feed():
            return q.pop(0) if q else IDLE
        idle = lambda: IDLE
        for t in range(682*(len(q)+4)):
            A.step(B.tx, t, feed); B.step(A.tx, t, idle)
        got = [(frame_ok(f), frame_data(f)) for _, f in B.rxframes]
        # align by order: count frames; compare i-th
        for i in range(n):
            pos_tot[i]+=1
            if i < len(got) and got[i][0] and got[i][1]==data[i]: pos_ok[i]+=1
    return [round(o/t,2) for o,t in zip(pos_ok,pos_tot)]
if __name__=='__main__':
    prog = piosim.load(sys.argv[1])
    for n in (1,2,3,4,8):
        print(n, burst_test(prog,n))
