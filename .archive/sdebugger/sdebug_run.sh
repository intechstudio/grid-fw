podman run \
-v $p/home/suku/Documents/grid-fw/sdebugger/public:/usr/src/app/public/ \
-v $p/home/suku/Documents/grid-fw/sdebugger/scripts:/usr/src/app/scripts/ \
-v $p/home/suku/Documents/grid-fw/sdebugger/src:/usr/src/app/src/ \
-p 5001:5000 -p 35729:35729 -p 1040:1040 -it suku/sdebug
