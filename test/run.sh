make clean
make
../fs_ref.x info ../disk2 # > output_ref
./test_fs.x info ../disk2 # > output_my
# diff output_ref output_my