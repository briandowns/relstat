#!/bin/sh

set -e

if [ -n "${DEBUG}" ]; then
    set -x
fi

LATEST=$(curl --silent "https://api.github.com/repos/rancher/ecm-distro-tools/releases/latest" | jq -r .tag_name)

if [ "$1" = "show" ]; then
    echo "${LATEST}"
fi

if [ "$1" = "update" ]; then
    echo "Updating ECM Distro Tools to: ${LATEST}"
    docker pull "rancher/ecm-distro-tools:${LATEST}"
    printf "To update the code, run from your fork:\n\tgit pull upstream master\n"
fi

exit 0
