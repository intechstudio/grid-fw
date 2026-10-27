#!/usr/bin/env python3
"""Two uart_duplex SMs, both transmitting and receiving at the same time, with a
slowly sweeping relative phase.

usage: duplex.py <uart_duplex.pio | .pio.h> [--cycles N] [--every burst|frame]
                 [--step CYCLES] [--max-burst N] [--max-gap N] [--seed N]
                 [--warmup CYCLES]

Both sides send bursts of back-to-back frames (random length, random idle gap)
carrying a counting sequence 1..127, so every receiver can tell a lost or
corrupted frame from the sequence alone. After each transmission on side B, B's
clock is paused for --step cycles, so the phase between the two SMs (and with it
the TX/RX phase inside each SM) creeps by --step cycles per transmission and
walks through every alignment of the 682-cycle (11-bit) frame.

Exit status is 1 if any frame after --warmup was bad or out of sequence.
"""
import argparse, bisect, random, sys, os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import piosim
from piosim import SM, tx_frame, IDLE, frame_ok, frame_data

FRAME_CYCLES = 62 * 11


def run(prog, cycles, every, step, max_burst, max_gap, seed, warmup):
    rnd = random.Random(seed)
    A, B = SM(prog, 'A'), SM(prog, 'B')
    st = {'stall': 0, 'total': 0}
    sweep_log_t, sweep_log_total = [0], [0]
    t_now = [0]

    def sweep():
        st['stall'] += step
        st['total'] += step
        sweep_log_t.append(t_now[0])
        sweep_log_total.append(st['total'])

    def make_feeder(on_burst_end, on_frame):
        s = {'burst': 0, 'gap': 0, 'seq': 0}

        def feed():
            on_frame()
            if s['burst'] == 0 and s['gap'] == 0:
                s['burst'] = rnd.randint(1, max_burst)
            if s['burst']:
                s['burst'] -= 1
                val = s['seq'] % 127 + 1
                s['seq'] += 1
                if s['burst'] == 0:
                    s['gap'] = rnd.randint(0, max_gap)
                    on_burst_end()
                return tx_frame(val)
            s['gap'] -= 1
            return IDLE
        feed.state = s
        return feed

    nop = lambda: None
    feed_a = make_feeder(nop, nop)
    feed_b = make_feeder(sweep if every == 'burst' else nop, sweep if every == 'frame' else nop)

    for t in range(cycles):
        t_now[0] = t
        A.step(B.tx, t, feed_a)
        if st['stall']:
            st['stall'] -= 1
        else:
            B.step(A.tx, t, feed_b)

    def phase_at(t):
        i = bisect.bisect_right(sweep_log_t, t) - 1
        return sweep_log_total[i] % FRAME_CYCLES

    result = {}
    for rx, sender_feed, name in ((B, feed_a, 'A->B'), (A, feed_b, 'B->A')):
        bad = brk = good = 0
        bad_phase, brk_phase = [], []
        last = None
        for t, f in rx.rxframes:
            if t < warmup:
                last = None if not frame_ok(f) else frame_data(f)
                continue
            if not frame_ok(f):
                bad += 1
                bad_phase.append(phase_at(t))
                last = None
                continue
            d = frame_data(f)
            if last is not None and d != last % 127 + 1:
                brk += 1
                brk_phase.append(phase_at(t))
            else:
                good += 1
            last = d
        result[name] = dict(sent=sender_feed.state['seq'], received=len(rx.rxframes), good=good, bad_framing=bad,
                            seq_breaks=brk, bad_phases=bad_phase, break_phases=brk_phase)
    return result, st['total']


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('program')
    ap.add_argument('--cycles', type=int, default=4_000_000)
    ap.add_argument('--every', choices=('burst', 'frame'), default='burst', help='what counts as one transmission')
    ap.add_argument('--step', type=int, default=1, help='cycles B is paused after each transmission')
    ap.add_argument('--max-burst', type=int, default=12, help='max back-to-back frames per burst')
    ap.add_argument('--max-gap', type=int, default=4, help='max idle frames between bursts')
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--warmup', type=int, default=100_000, help='ignore frames before this cycle (sim starts mid-stream)')
    a = ap.parse_args()

    prog = piosim.load(a.program)
    res, total = run(prog, a.cycles, a.every, a.step, a.max_burst, a.max_gap, a.seed, a.warmup)

    failed = False
    print(f'phase swept {total} cycles in total ({total / FRAME_CYCLES:.2f} x one 682-cycle frame)')
    for name, r in res.items():
        print(f"{name}: sent {r['sent']}, rx {r['received']}, good {r['good']}, bad framing {r['bad_framing']}, sequence breaks {r['seq_breaks']}")
        errs = r['bad_phases'] + r['break_phases']
        if errs:
            failed = True
            buckets = {}
            for p in errs:
                buckets[p // 62] = buckets.get(p // 62, 0) + 1
            worst = ', '.join(f'{b * 62}-{b * 62 + 61}: {n}' for b, n in sorted(buckets.items()))
            print(f'   errors by relative phase (cycles mod 682): {worst}')
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
