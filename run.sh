#!/bin/bash
# This script runs the os.
echo "checking options..."
if [ "$1" == "--help" ]; then
    echo "Usage: ./run.sh [options]"
    echo "Options:"
    echo "  --help       Show this help message"
    echo "  --version    Show version information"
    echo "  --VIRTIO     select virtio version. 1 or 2"
    echo "               1 for virtio legacy, 2 for virtio modern"
    echo "  --FS_DEBUG   Enable filesystem debug mode"
    echo "               1 to enable, 0 to disable"
    exit 0
fi

virtio=""
fs_debug=""

echo "checking virtio option..."
echo "1 for virtio legacy, 2 for virtio modern"
read -p "Select virtio version (1 or 2): " virtio
if [ "$virtio" != "1" ] && [ "$virtio" != "2" ]; then
    echo "Invalid virtio option. Please select 1 or 2."
    exit 1
fi  

echo "checking FS_DEBUG option..."
read -p "Enable filesystem debug mode? (1 to enable, 0 to disable): " fs_debug
if [ "$fs_debug" != "0" ] && [ "$fs_debug" != "1" ]; then
    echo "Invalid FS_DEBUG option. Please select 0 or 1."
    exit 1
fi

echo "Starting the OS with virtio version $virtio and FS_DEBUG set to $fs_debug..."

make clean
make VIRTIO=$virtio FS_DEBUG=$fs_debug run
