#!/bin/bash
#set -o pipefail
#

set -e

script_dir=$(cd "$(dirname "$0")" && pwd)
build_root=$(cd "${script_dir}/../.." && pwd)
run_unit_tests=ON

usage ()
{
    echo "build.sh [options]"
    echo "options"
    echo " -cl, --compileoption <value> specify a compile option to be passed to gcc"
    echo " Example: -cl -O1 -cl ..."
    echo ""
    exit 1
}

process_args ()
{
    save_next_arg=0
    extracloptions=" "
    for arg in $*
    do
      if [ $save_next_arg == 1 ]
      then
        # save arg to pass to gcc
        extracloptions="$arg $extracloptions"
        save_next_arg=0
      else
          case "$arg" in
              "-cl" | "--compileoption" ) save_next_arg=1;;
              * ) usage;;
          esac
      fi
    done
}

process_args $*

rm -r -f ~/azure-mqtt
mkdir ~/azure-mqtt
pushd ~/azure-mqtt
cmake -DcompileOption_C:STRING="$extracloptions" $build_root
make --jobs=$(nproc)
ctest -C "Debug" -V
popd
