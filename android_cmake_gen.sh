NDK=/home/user/!crap/android-ndk-r23
ABI=arm64-v8a
MINSDKVERSION=21

cmake \
    -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=$ABI \
    -DANDROID_NATIVE_API_LEVEL=$MINSDKVERSION \
    -S . -B build "$@"
