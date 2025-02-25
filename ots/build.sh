#!/bin/bash
set -euxo pipefail
echo hi
cd "$(dirname "$0")"
cd source/devcpp
make clean
make -j$(nproc)
cd "../.."
cp source/devcpp/YurOTS YurOTS
./YurOTS