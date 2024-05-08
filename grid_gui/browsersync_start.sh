echo "Starting browser sync [npm install -g browser-sync]"
browser-sync start --server --directory build --files "build/*.html build/*.css build/*.js build/*.wasm"