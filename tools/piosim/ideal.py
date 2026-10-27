"""One uart_duplex SM against an ideal UART (11-bit frames, mid-bit sampling), full duplex.

usage: ideal.py <uart_duplex.pio | .pio.h> [ppm ...]   (ideal UART baud error, default 0 500 -500 5000 -5000)
"""
import sys, random
sys.path.insert(0, __import__('os').path.dirname(__file__) or '.')
import piosim
from piosim import SM, tx_frame, IDLE, frame_ok, frame_data

class IdealTx:
    """Ideal UART sender: 11-bit frames (start, 8 data, parity=0, stop), back-to-back bursts, bit time bt cycles."""
    def __init__(self, rnd, bt):
        self.rnd, self.bt = rnd, bt
        self.level = 1; self.bits = []; self.acc = 0.0; self.sent = []
        self.burst = 0; self.gap = 0
    def step(self):
        self.acc += 1
        if self.acc >= self.bt:
            self.acc -= self.bt
            if not self.bits:
                if self.burst == 0 and self.gap == 0: self.burst = self.rnd.randint(5, 60)
                if self.burst:
                    self.burst -= 1
                    c = self.rnd.randint(1, 127); self.sent.append(c)
                    self.bits = [0] + [(c >> i) & 1 for i in range(8)] + [0, 1]
                    if self.burst == 0: self.gap = self.rnd.randint(0, 3)
                else:
                    self.gap -= 1; self.bits = [1]*11
            self.level = self.bits.pop(0)

class IdealRx:
    """Ideal UART receiver sampling mid-bit (11-bit frames, parity ignored)."""
    def __init__(self, bt):
        self.bt = bt; self.prev = 1; self.cnt = None; self.t0 = 0; self.got = []; self.bad = 0
    def step(self, t, level):
        if self.cnt is None:
            if self.prev == 1 and level == 0:
                self.cnt = 0; self.t0 = t; self.bitsamp = []
        else:
            # sample at t0 + bt*(k+0.5)
            k = len(self.bitsamp)
            if t >= self.t0 + self.bt * (k + 0.5):
                self.bitsamp.append(level)
                if len(self.bitsamp) == 11:
                    b = self.bitsamp
                    if b[0] == 0 and b[10] == 1:
                        self.got.append(sum(b[1+i] << i for i in range(8)))
                    else:
                        self.bad += 1
                    self.cnt = None
        self.prev = level

def test(prog, seed, ppm, cycles=800_000):
    rnd = random.Random(seed)
    bt = 62.0 * (1 + ppm / 1e6)
    S = SM(prog, 'S')
    ix = IdealTx(rnd, bt)
    ir = IdealRx(bt)
    sent = []
    st = {'b': 0, 'g': 0}
    def feed():
        if st['b'] == 0 and st['g'] == 0: st['b'] = rnd.randint(5, 60)
        if st['b']:
            st['b'] -= 1
            c = rnd.randint(1, 127); sent.append(c)
            if st['b'] == 0: st['g'] = rnd.randint(0, 3)
            return tx_frame(c)
        st['g'] -= 1; return IDLE
    for t in range(cycles):
        ix.step()
        S.step(ix.level, t, feed)
        ir.step(t, S.tx)
    rx = [(frame_ok(f), frame_data(f)) for _, f in S.rxframes]
    rx_ok = sum(1 for ok, d in rx if ok)
    # exact sequence compare ignoring first 20 frames
    return dict(ideal_to_us=f"{rx_ok}/{len(rx)} ok (sent {len(ix.sent)})",
                us_to_ideal=f"{len(ir.got)} good, {ir.bad} bad (sent {len(sent)})")

if __name__ == '__main__':
    prog = piosim.load(sys.argv[1])
    ppms = [int(a) for a in sys.argv[2:]] or [0, 500, -500, 5000, -5000]
    for ppm in ppms:
        print('ppm', ppm, test(prog, 1, ppm))
