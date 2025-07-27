#!/bin/bash

glslc=/usr/local/bin/glslc
options="" #"-Werror"

root_folder=$(dirname "$0")
shaders_dir=${root_folder}/shaders
build_dir=${root_folder}/build/shaders
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

