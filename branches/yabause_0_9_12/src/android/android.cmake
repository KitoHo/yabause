SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

SET(CMAKE_C_COMPILER   arm-linux-androideabi-gcc)
SET(CMAKE_CXX_COMPILER arm-linux-androideabi-g++)
SET(CMAKE_ASM-ATT_COMPILER arm-linux-androideabi-as)

SET(CMAKE_FIND_ROOT_PATH /home/guillaume/projects/android/toolchain/sysroot/usr/)

SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(ANDROID ON)
