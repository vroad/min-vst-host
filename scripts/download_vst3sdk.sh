#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
json_file="${repo_root}/vst3sdk-source.json"
target_dir="${repo_root}/external/vst3sdk"

owner=$(jq -r '.owner' "${json_file}")
repo=$(jq -r '.repo' "${json_file}")
rev=$(jq -r '.rev' "${json_file}")

repo_url="https://github.com/${owner}/${repo}.git"

echo "Cloning ${repo_url}@${rev}"
rm -rf "${target_dir}"
git clone --depth 1 --recurse-submodules --shallow-submodules "${repo_url}" "${target_dir}"

echo "SDK checked out to ${target_dir}"
