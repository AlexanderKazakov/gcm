#!/usr/bin/env bash

usage() { echo "Usage: $0
    [-n] number_of_processes (currently ignored)
    [-t] task_name
    [-o] folder to write snapshots into relative to ./snapshots/
    [-s] do not show output to stdout
    [-p] (to open Paraview after calculation)"
    1>&2; exit 1; }

np=1 #`cat /proc/cpuinfo | grep processor | wc -l`
task="cgal2d"
outDir=""
run_paraview=0
silent=0

while getopts ":n:t:o:sp" o; do
    case "${o}" in
        n)
#            np=${OPTARG}
            ;;
        t)
            task=${OPTARG}
            ;;
        o)
            outDir=${OPTARG}
            ;;
        s)
            silent=1
            ;;
        p)
            run_paraview=1
            ;;
        *)
            usage
            ;;
    esac
done

snaps="snapshots"
if [ -n "$outDir" ]; then
	snaps="$snaps/$outDir"
fi
echo "Write snapshots to $snaps"

rm -rf $snaps/vtk $snaps/detector $snaps/zaxis
mkdir -p $snaps/vtk $snaps/detector $snaps/zaxis
rm -f *.log
echo "Start ./build/gcm_exe with $np processes ..."
if ((silent)); then
	./build/gcm_exe --task ${task} --out $outDir > /dev/null &
else
	./build/gcm_exe --task ${task} --out $outDir
fi

if ((run_paraview)); then
    echo "Run Paraview ..."
    if (($np == 1)); then
        if (ls snapshots/vtk | grep vts); then
            paraview --data=snapshots/vtk/mesh0core00snap..vts
        else
            paraview --data=snapshots/vtk/mesh0core00snap..vtu
        fi
    else
        paraview
    fi
fi
