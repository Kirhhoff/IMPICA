make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- oldconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- dep
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j1
