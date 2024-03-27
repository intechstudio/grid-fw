emcc main.c -o index.html -s NO_EXIT_RUNTIME=1 -s 'EXPORTED_RUNTIME_METHODS=["ccall"]' 
emrun index.html