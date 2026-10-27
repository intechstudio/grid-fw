#!/usr/bin/env python3
"""Cycle-accurate sim of two uart_duplex PIO SMs wired TX->RX each way.

usage: piosim.py <uart_duplex.pio | uart_duplex.pio.h> [seed]

Executes the real opcodes (not a re-implementation of the program), so edits to
the .pio are tested as written. A .pio is assembled with pioasm (env PIOASM, the
rp2350/build one, or PATH); a pioasm-generated header is read directly.
"""
import os, re, shutil, subprocess, sys, tempfile, random

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))


class Program(list):
    wrap = 0


def find_pioasm():
    for c in (os.environ.get('PIOASM'), os.path.join(REPO, 'rp2350/build/pioasm/pioasm'), shutil.which('pioasm')):
        if c and os.path.exists(c):
            return c
    sys.exit('pioasm not found: set PIOASM or build rp2350 first')


def load(path):
    if path.endswith('.pio'):
        with tempfile.TemporaryDirectory() as d:
            out = os.path.join(d, 'out.h')
            subprocess.run([find_pioasm(), '-o', 'c-sdk', path, out], check=True)
            s = open(out).read()
    else:
        s = open(path).read()
    prog = Program(int(m, 16) for m in re.findall(r'0x([0-9a-f]{4}),', s.split('_program_instructions')[1]))
    prog.wrap = int(re.search(r'_wrap (\d+)', s).group(1))
    return prog

class SM:
    def __init__(self, prog, name):
        self.prog = prog
        self.wrap = prog.wrap
        self.name = name
        self.pc = 0
        self.delay = 0
        self.x = 0
        self.osr = 0
        self.osr_cnt = 32   # bits shifted out; >=11 means empty (autopull)
        self.isr = 0
        self.isr_cnt = 0
        self.txq = []       # words waiting
        self.tx = 1         # pin level
        self.rxframes = []
        self.rx_sync = [0, 0]  # 2-flop synchroniser (inverted RX level)
        self.stalls = 0
        self.out_times = []

    def step(self, rx_level, t, feeder):
        # synchroniser: pio sees pin 2 cycles late, inverted
        inv = 0 if rx_level else 1
        seen = self.rx_sync[1]
        self.rx_sync = [inv, self.rx_sync[0]]
        if self.delay:
            self.delay -= 1
            return
        op = self.prog[self.pc]
        code = op >> 13
        dly = (op >> 8) & 31
        arg = op & 0xff
        nxt = self.pc + 1 if self.pc != self.wrap else 0
        if code == 0:  # jmp
            cond = (arg >> 5) & 7
            addr = arg & 31
            if cond == 0: nxt = addr
            elif cond == 1:
                if self.x == 0: nxt = addr
            elif cond == 2:
                taken = self.x != 0
                self.x = (self.x - 1) & 0xffffffff
                if taken: nxt = addr
            elif cond == 6:
                if seen: nxt = addr
            else: raise Exception('cond')
        elif code == 2:  # in pins,1
            bit = seen
            self.isr = (self.isr >> 1) | (bit << 31)
            self.isr_cnt += 1
            if self.isr_cnt >= 11:
                self.rxframes.append((t, self.isr))
                self.isr = 0; self.isr_cnt = 0
        elif code == 3:  # out pins,1
            if self.osr_cnt >= 11:
                if not self.txq:
                    self.txq.append(feeder())
                self.osr = self.txq.pop(0); self.osr_cnt = 0
            self.tx = self.osr & 1
            self.osr >>= 1
            self.osr_cnt += 1
            self.out_times.append(t)
        elif code == 7:  # set x
            self.x = arg & 31
        elif code == 5:  # nop (mov y,y)
            pass
        else:
            raise Exception('op %04x' % op)
        self.pc = nxt
        self.delay = dly

def frame_ok(f): return ((f >> 21) & 1) and not ((f >> 31) & 1)
def frame_data(f): return (~(f >> 22)) & 0xff
def tx_frame(c): return (c << 1) | (1 << 10)
IDLE = 0x7ff

def run(prog, seed, cycles=3_000_000, skew_ppm=0, load_pct=100, bload=None):
    # bload: not None -> B never sends data (one-way A->B), only idle frames
    rnd = random.Random(seed)
    A, B = SM(prog, 'A'), SM(prog, 'B')
    # random phase offset of B relative to A
    boff = rnd.randrange(0, 62 * 11)
    sent = {'A': [], 'B': []}

    def mk_feeder(name):
        st = {'burst': 0, 'gap': 0}
        def feed():
            if st['burst'] == 0 and st['gap'] == 0:
                if rnd.random() < load_pct / 100.0:
                    st['burst'] = rnd.randint(5, 60)
                else:
                    st['gap'] = rnd.randint(1, 30)
            if st['burst']:
                st['burst'] -= 1
                c = rnd.randint(1, 127)
                sent[name].append(c)
                if st['burst'] == 0: st['gap'] = rnd.randint(0, 3)
                return tx_frame(c)
            st['gap'] -= 1
            return IDLE
        return feed
    fa = mk_feeder('A'); fb = mk_feeder('B')
    if bload is not None:
        fb = (lambda: IDLE)
    skip_every = int(1e6 / skew_ppm) if skew_ppm else 0
    for t in range(cycles):
        # wire delay 0: pin values from previous cycle
        A.step(B.tx, t, fa)
        if t >= boff and not (skip_every and t % skip_every == 0):
            B.step(A.tx, t, fb)
    out = {}
    for rx, tx, nm in ((B, 'A', 'A->B'), (A, 'B', 'B->A')):
        good = [frame_data(f) for _, f in rx.rxframes if frame_ok(f)]
        bad = sum(1 for _, f in rx.rxframes if not frame_ok(f))
        # compare good data against the sent sequence (subsequence match)
        s = sent[tx]; i = 0; matched = 0
        for g in good:
            j = i
            while j < len(s) and s[j] != g: j += 1
            if j < len(s): matched += 1; i = j + 1
        out[nm] = dict(sent=len(s), ok=len(good), bad=bad, data_match=matched)
    return out

if __name__ == '__main__':
    prog = load(sys.argv[1])
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    for ppm in (0, 100):
        for pct in (20, 100):
            print('skew', ppm, 'ppm  load', pct, '%', run(prog, seed, skew_ppm=ppm, load_pct=pct))
