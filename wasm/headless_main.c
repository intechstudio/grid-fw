// Headless (Node.js and browser-library) entry point for the VSN1 module
// simulator. No SDL, no main loop of its own - this file exists only to
// give the build an entry point; all behavior is the sim_core.h API
// (grid_sim_init/tick/input_*/get_*), which the loading JS calls directly.
// See wasm/example.js for a minimal Node.js usage example.

#include "sim_core.h"

int main(int argc, char** argv) { return 0; }
