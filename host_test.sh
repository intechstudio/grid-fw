cmake -S ./grid_common/host_test -B ./grid_common/host_test/build
make -C ./grid_common/host_test/build
./grid_common/host_test/build/UnitTest
