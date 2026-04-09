#!/usr/bin/env bash

set -euo pipefail

MESON_PREFIX="${1:?meson install prefix required}"
CONAN_DIR="${2:?dependency output dir required}"
PACKAGE_DIR="${3:?package output dir required}"

if [[ ! -d "${MESON_PREFIX}" ]]; then
    echo "error: Meson install prefix does not exist: ${MESON_PREFIX}" >&2
    exit 1
fi

rm -rf "${PACKAGE_DIR}"
mkdir -p "${PACKAGE_DIR}"

cp -a "${MESON_PREFIX}/." "${PACKAGE_DIR}/"

if [[ -d "${CONAN_DIR}" ]]; then
    for name in bin include lib etc share loader; do
        src="${CONAN_DIR}/${name}"
        if [[ -d "${src}" ]]; then
            mkdir -p "${PACKAGE_DIR}/${name}"
            cp -a "${src}/." "${PACKAGE_DIR}/${name}/"
        fi
    done
fi
