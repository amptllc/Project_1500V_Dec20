./returner.exe  -i "../ED/hex/ampt-ms-i-full-low.hex" -o "../ED/hex/ampt-ms-i-full-low+ret.hex" -startAddr 5800 -endAddr 5BE1 -retAddr 0 -rc
./returner.exe  -i "../ED/hex/ampt-ms-i-full-low+ret.hex" -o "../ED/hex/ampt-ms-i-full-low+ret.hex" -startAddr 5BF0 -endAddr 5BF1 -retAddr 0
./add-crc16-to-hex-file.exe "../ED/hex/ampt-ms-i-full-low+ret.hex" "../ED/hex/ampt-ms-i-full-low+ret+crc.hex"                                                                               
./a-linker.exe  -boot "../ED/hex/ny-boot.hex" -low "../ED/hex/ampt-ms-i-full-low+ret+crc.hex" -high "../ED/hex/ampt-ms-i-uhigh.hex" -bunch 0 -ch 253 -no-search -v-ch255 0 -addr 83F  -sn $1 -netId 10 -vshutup 0
