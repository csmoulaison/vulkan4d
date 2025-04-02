BIN=bin

# Shader compilation
GLSLC=glslc
SHADER_SRC=src/vulkan/shaders
SHADER_OUT=$BIN/shaders

printf "Compiling GLSL...\n"

$GLSLC $SHADER_SRC/shader.vert -o $SHADER_OUT/vert.spv
if [ $? -ne 0 ]; then
	exit 1
fi

$GLSLC $SHADER_SRC/shader.frag -o $SHADER_OUT/frag.spv
if [ $? -ne 0 ]; then
	exit 1
fi

# Executable compilation
CC=gcc
EXE=vulkan4d
SRC=src/xcb/xcb_main.c
INCLUDE=src/
LIBS="-lX11 -lX11-xcb -lm -lxcb -lxcb-xfixes -lxcb-keysyms -lvulkan"
FLAGS="-g -O3 -Wall"


printf "Compiling executable...\n"

$CC -o $BIN/$EXE $SRC -I $INCLUDE $FLAGS $LIBS

if [ $? -eq 0 ]; then
    printf "Compilation was \033[0;32m\033[1msuccessful\033[0m.\n"
    exit 0
else
	exit 1
fi
