#!/bin/sh

echo "Replace the constants in ble.h:"
fst=$(python3 -c "from uuid import uuid4; print(str(uuid4()).upper())")
snd=$(python3 -c "from uuid import uuid4; print(str(uuid4()).upper())")
echo "$fst"
echo "$snd"
