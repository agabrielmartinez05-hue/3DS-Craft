#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
target=third_party/nlohmann/json.hpp
checksum=aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63
if [[ -f "$target" ]] && echo "$checksum  $target" | sha256sum --check --status; then exit 0; fi
mkdir -p third_party/nlohmann
staging=$(mktemp third_party/nlohmann/.json.XXXXXX)
trap 'rm -f "$staging"' EXIT
curl --fail --location --retry 2 --connect-timeout 15 --max-time 120 \
  https://raw.githubusercontent.com/nlohmann/json/v3.12.0/single_include/nlohmann/json.hpp -o "$staging"
echo "$checksum  $staging" | sha256sum --check --status
mv "$staging" "$target"
