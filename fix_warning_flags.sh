#!/bin/bash

set -e

echo "🔍 Searching project.pbxproj files..."

find . -name "project.pbxproj" | while read -r file; do
  echo "Processing: $file"

  # 如果已经包含 WARNING_CFLAGS 就跳过
  if grep -q 'WARNING_CFLAGS = "-Wno-logical-op-parentheses";' "$file"; then
    echo "  ⏭️  Already patched, skip"
    continue
  fi

  # 使用 awk 插入（比 sed 更稳定）
  awk '
  {
    print $0
    if ($0 ~ /SDKROOT = macosx;/) {
      print "\t\t\t\tWARNING_CFLAGS = \"-Wno-logical-op-parentheses\";"
    }
  }
  ' "$file" > "$file.tmp" && mv "$file.tmp" "$file"

  echo "  ✅ Patched"
done

echo "🎉 Done."