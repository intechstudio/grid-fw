// Headless smoke test for the VSN1 module simulator - run with plain
// `node wasm/example.js` after `./lua_build.sh && ./wasm_build.sh`
// (no browser, no display).
//
// Presses button 0, ticks the simulation, and prints what the module's
// real firmware logic did in response: the resulting LED color (driven by
// button 0's default Lua script, common/src/c/grid_ui_button.h's
// GRID_ACTIONSTRING_BUTTON_BUTTON) and the protocol message it sent to its
// "UI" transport port (grid_send()/etc. output).

const path = require('path');
const GridModuleSim = require(path.join(__dirname, 'build', 'grid_sim.js'));

function readLedRgb(Module, num) {
  const ptr = Module._malloc(3);
  try {
    Module.ccall('grid_sim_get_led_rgb', null, ['number', 'number', 'number', 'number'], [num, ptr, ptr + 1, ptr + 2]);
    return [Module.HEAPU8[ptr], Module.HEAPU8[ptr + 1], Module.HEAPU8[ptr + 2]];
  } finally {
    Module._free(ptr);
  }
}

// Element index is not the same as physical LED index - VSN1L's button 0
// lights physical LED 10, not LED 0 (see grid_module_vsnl_ui_init,
// common/src/c/grid_module.c). Look it up instead of assuming.
function ledIndicesForElement(Module, element, maxCount) {
  const ptr = Module._malloc(maxCount);
  try {
    const n = Module.ccall('grid_sim_get_element_led_indices', 'number', ['number', 'number', 'number'], [element, ptr, maxCount]);
    const out = [];
    for (let i = 0; i < n; i++) out.push(Module.HEAPU8[ptr + i]);
    return out;
  } finally {
    Module._free(ptr);
  }
}

function logModuleOutput(Module, label) {
  const out = Module.ccall('grid_sim_drain_module_output', 'string', [], []);
  if (out.length > 0) {
    console.log(label, JSON.stringify(out));
  }
}

GridModuleSim().then((Module) => {
  Module.ccall('grid_sim_init', null, [], []);

  const ledIndices = ledIndicesForElement(Module, 0, 4);
  if (ledIndices.length === 0) {
    console.error('FAIL: button 0 has no LED mapped - element/LED lookup table looks wrong');
    process.exit(1);
  }
  const led = ledIndices[0];
  console.log('button 0 -> physical LED', led);

  logModuleOutput(Module, 'module output after init:');

  const ledBefore = readLedRgb(Module, led);
  console.log('button 0 LED before press:', ledBefore);

  // Press, let the simulation run a few frames so the triggered BUTTON
  // event's Lua script actually executes, then release.
  Module.ccall('grid_sim_input_button', null, ['number', 'number'], [0, 1]);
  Module.ccall('grid_sim_tick', null, ['number'], [10]);
  Module.ccall('grid_sim_tick', null, ['number'], [10]);

  const ledPressed = readLedRgb(Module, led);
  console.log('button 0 LED while pressed:', ledPressed);
  logModuleOutput(Module, 'module output while pressed:');

  Module.ccall('grid_sim_input_button', null, ['number', 'number'], [0, 0]);
  Module.ccall('grid_sim_tick', null, ['number'], [10]);

  const ledReleased = readLedRgb(Module, led);
  console.log('button 0 LED after release:', ledReleased);
  logModuleOutput(Module, 'module output after release:');

  const changed = ledBefore.join(',') !== ledPressed.join(',');
  if (!changed) {
    console.error('FAIL: pressing button 0 did not change its LED - check element/event wiring');
    process.exit(1);
  }

  console.log('OK: button press drove real module Lua/LED logic headlessly.');
});
