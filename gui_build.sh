emcc grid_gui/main.c -o grid_gui/index.html -s NO_EXIT_RUNTIME=1 -s 'EXPORTED_RUNTIME_METHODS=["ccall"]' 
python3 -m http.server -d grid_gui