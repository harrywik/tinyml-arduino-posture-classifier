#!/bin/sh

echo "Replace the constants in ble.h:"
fst=$(python3 -c "from uuid import uuid4; print(str(uuid4()))")
snd=$(python3 -c "from uuid import uuid4; print(str(uuid4()))")
echo "$fst"
echo "$snd"
