#!/bin/bash
# 一鍵鋪 LLVM build 環境：clone(淺) → Triple patch → symlink target → cmake
set -euo pipefail
LLVM=${LLVM_DIR:-$HOME/llvm-project}
HERE="$(cd "$(dirname "$0")" && pwd)"

# 1. clone（已存在就跳過）
if [ ! -d "$LLVM" ]; then
  git clone --depth 1 --branch llvmorg-20.1.8 \
    https://github.com/llvm/llvm-project.git "$LLVM"
fi

# 2. Triple patch（guard：已 patch 就跳過）
TH="$LLVM/llvm/include/llvm/TargetParser/Triple.h"
TC="$LLVM/llvm/lib/TargetParser/Triple.cpp"
if ! grep -q "toygpu" "$TH"; then
  # enum：插在 LastArchType 前一行（xtensa 是 20.x 的最後一個正式 arch）
  sed -i 's/^\(\s*\)xtensa,\(.*\)$/\1xtensa,\2\n\1toygpu,         \/\/ ToyGPU: software teaching GPU/' "$TH"
  echo "patched Triple.h"
fi
if ! grep -q "toygpu" "$TC"; then
  sed -i 's/^\(\s*\)case xtensa:\(\s*\)return "xtensa";/\1case xtensa:\2return "xtensa";\n\1case toygpu:      return "toygpu";/' "$TC"
  sed -i 's/\(\.Case("xtensa", Triple::xtensa)\)/\1\n    .Case("toygpu", Triple::toygpu)/' "$TC"
  sed -i 's/^\(\s*\)case Triple::xtensa:$/\1case Triple::xtensa:\n\1case Triple::toygpu:/' "$TC"
  echo "patched Triple.cpp"
fi

# 3. symlink target 原始碼進 tree
ln -sfn "$HERE/ToyGPU" "$LLVM/llvm/lib/Target/ToyGPU"
echo "linked llvm/lib/Target/ToyGPU -> $HERE/ToyGPU"

# 4. cmake 配置（只 build 我們的 experimental target；15G RAM 限 2 個 link job）
mkdir -p "$LLVM/build"
cmake -G Ninja -S "$LLVM/llvm" -B "$LLVM/build" \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DLLVM_TARGETS_TO_BUILD="" \
  -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=ToyGPU \
  -DLLVM_PARALLEL_LINK_JOBS=2 \
  -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF
echo
echo "ready:  ninja -C $LLVM/build llc"
