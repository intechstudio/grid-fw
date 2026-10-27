# piosim

Cycle-accurate simulator for the RP2350 `uart_duplex.pio` (single state machine, RX and TX
together). It executes the real PIO opcodes, so changes to the `.pio` are tested as written.
Plain Python 3, no dependencies.

Models: 1 instruction per cycle plus delay, the 2-flop input synchroniser, inverted RX, autopush /
autopull at 11 bits, back-to-back TX frames. Does **not** model pad slew, wire delay, or the
fractional clock divider.

The program argument is either a `.pio` (assembled with `pioasm`: `$PIOASM`, then
`rp2350/build/pioasm/pioasm`, then `PATH`) or a pioasm-generated `.pio.h`.

```
python3 tools/piosim/piosim.py  common/src/pio/uart_duplex.pio      # two copies, full duplex, random phase / skew
python3 tools/piosim/duplex.py  common/src/pio/uart_duplex.pio      # both directions at once, phase sweeping (exit 1 on any error)
python3 tools/piosim/burst.py   common/src/pio/uart_duplex.pio      # decode rate by position in a back-to-back burst
python3 tools/piosim/ideal.py   common/src/pio/uart_duplex.pio 0 5000   # against an ideal UART at given ppm errors
```

`duplex.py` is the most complete test: both SMs send bursts of back-to-back frames (random length and
idle gap) while receiving, carrying a counting sequence so any lost or corrupted frame is detected.
After each transmission on side B, B is paused `--step` cycles (default 1), so the phase between the
two SMs, and the TX/RX phase inside each, creeps through every alignment of the 682-cycle frame.
Errors are reported bucketed by that relative phase. Options: `--every frame` (sweep per frame instead
of per burst), `--step`, `--cycles`, `--max-burst`, `--max-gap`, `--seed`.

Good result: `duplex.py` no errors over several full sweeps (try `--cycles 20000000`), `burst.py` all 1.0, `ideal.py` every frame good in both directions.
The pre-fix program (before commit 3ac00820) decodes `[1.0, 1.0, 0.0, ...]` in `burst.py` and
about 40% of frames in `ideal.py`.

`piosim.py` output: `ok` is frames that passed framing (start 0, stop 1). `data_match` is a greedy
subsequence match and under-counts after a lost frame; trust `ok/total`. A few bad frames in the
first ~100k cycles are warm-up (the sim starts mid-stream).
