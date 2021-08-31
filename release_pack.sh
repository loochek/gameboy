VER=0.93

CC=clang cmake -DCMAKE_BUILD_TYPE=Release -S . -B build/
cd build
make -j8
tar -cvzf gb-$VER-linux-amd64.tar.gz gb
tar -cvzf gb-$VER-libretro-linux-amd64.tar.gz gb_libretro.so ../gb_libretro.info
mv *.tar.gz ../
rm -rf *
cd ..

./android_cmake_gen.sh -DCMAKE_BUILD_TYPE=Release
cd build
make -j8
llvm-strip-12 gb_libretro.so
tar -cvzf gb-$VER-libretro-android-aarch64.tar.gz gb_libretro.so ../gb_libretro.info
mv *.tar.gz ../
rm -rf *
cd ..
