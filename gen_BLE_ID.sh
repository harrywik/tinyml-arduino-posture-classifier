FILE="ble.h"
PATTERN="run .\/gen_BLE_ID\.sh"
UUID=$(python3 -c "from uuid import uuid4; print(str(uuid4()).upper())")

sed -i "s/$PATTERN/$UUID/" "$FILE"

if ! rg -q "$UUID" "$FILE"; then
  echo "WARNING: Replacement failed."
else
  echo "SUCCESS: UUID placeholder has been replaced."
fi
