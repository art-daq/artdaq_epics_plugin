#!/bin/bash

if [ "${SETUP_UPS:-x}" == "x" ]; then
    echo "UPS Must be set up before running this script"
    exit 1
fi

version=$1

for qual in `ups list -aK+ epics $version|awk '{print $4}'|sort|uniq`;do
    echo "Declaring epics $version -q $qual current"
    ups declare -c epics $version -q $qual
done
