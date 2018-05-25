make clean
make
./fs_ref.x info disk  # > output_ref
./test_fs.x info disk # > output_my
# diff output_ref output_my
