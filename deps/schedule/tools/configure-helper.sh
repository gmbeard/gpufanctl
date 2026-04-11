#!/usr/bin/env bash

set -o pipefail

projectdir=$(cd $(dirname ${0})/.. && pwd)
cxxflags=" -Wall -Werror -Wextra -Wconversion -Wshadow -Wunused-result "
sanitizerflags=" -fsanitize=address,undefined "

exec cmake \
    -G Ninja \
    -S "${projectdir}" \
    -B "${projectdir}/build" \
    --fresh \
    -DCMAKE_CXX_STANDARD=20 \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="${cxxflags} ${sanitizerflags}" \
    -DCMAKE_EXE_LINKER_FLAGS="${sanitizerflags}" \
    -DCMAKE_SHARED_LINKER_FLAGS="${sanitizerflags}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DSCHEDULE_ENABLE_TESTS=ON \
    -DSCHEDULE_ENABLE_EXAMPLES=ON
