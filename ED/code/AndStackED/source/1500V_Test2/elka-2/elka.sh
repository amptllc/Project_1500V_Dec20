#!/bin/bash
i=0
#for mac in 1509K000681  1509K000724 1509K000797 1509K000860 1509K000919 1509K000716 1509K000750 1509K000830 1509K000910  1509K000922 ; do 
#    "/cygdrive/z/projects/Anatoli_MidString/Ver8d - Final/a-linker.exe"  -boot "/cygdrive/z/projects/Anatoli_MidString/Ver8d - Final/hex/ny-boot.hex" -low "/cygdrive/z/projects/Anatoli_MidString/Ver8d - Final/hex/ampt-ms-i-full-low+ret+crc.hex" -high "/cygdrive/z/projects/Anatoli_MidString/Ver8d - Final/hex/ampt-ms-i-uhigh.hex" -bunch 0 -ch 253 -no-search -v-ch255 0 -stay-in-rx -addr 83F  -sn $mac -netId $i
#    i=$((i+1)); 
#done

for mac in 1612P000343 1612P000356 1612P000383 1612P000391 1612P000405 1612P000406 1612P000410 1612P000412 1612P000413 1612P000438; do 
    "../utils/a-linker.exe"  -boot "../ED/hex/ny-boot.hex" -low "../ED/hex/ampt-ms-i-full-low+ret+crc.hex" -high "../ED/hex/ampt-ms-i-uhigh.hex" -bunch 0 -ch 253 -no-search -v-ch255 0 -stay-in-rx -addr 83F  -sn $mac -netId $i
    i=$((i+1)); 
done

