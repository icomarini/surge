#!/bin/bash

glslc=/home/ico/projects/extern/shaderc/install/bin/glslc
options="-Werror"

shaders_dir=/home/ico/projects/surge/shaders
build_dir=/home/ico/projects/surge/build/shaders
mkdir -p $build_dir

for shader in ${shaders_dir}/*.vert
do
    shader_bytecode=${build_dir}/$(basename $shader).spv
    [ -e $shader_bytecode ] && rm $shader_bytecode
    $glslc $options $shader -o $shader_bytecode
    test -f $shader_bytecode && echo "Compiled ${shader} to ${shader_bytecode}"
done

for shader in ${shaders_dir}/*.frag
do
    shader_bytecode=${build_dir}/$(basename $shader).spv
    [ -e $shader_bytecode ] && rm $shader_bytecode
    $glslc $shader -o $shader_bytecode
    test -f $shader_bytecode && echo "Compiled ${shader} to ${shader_bytecode}"
done

