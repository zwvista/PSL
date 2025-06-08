#!/bin/bash

WORKSPACE="PSL.xcworkspace"
CONFIGURATION="Debug"  # 或者使用 Release

# 获取所有 scheme
SCHEMES=$(xcodebuild -workspace "$WORKSPACE" -list | awk '/Schemes:/,/^$/' | tail -n +2 | sed 's/^[ \t]*//')

# 遍历每个 scheme 并构建
for SCHEME in $SCHEMES; do
  echo "🔨 Building scheme: $SCHEME"
  xcodebuild -workspace "$WORKSPACE" -scheme "$SCHEME" -configuration "$CONFIGURATION" build
  if [ $? -ne 0 ]; then
    echo "❌ Build failed for scheme: $SCHEME"
    exit 1
  fi
done

echo "✅ All schemes built successfully."
