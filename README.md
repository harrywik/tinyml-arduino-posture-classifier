# TinyML Arduino Posture Classifier

## Features

- [x] Sample from the IMU's gyro and acc in x,y,z
- [x] Normalize input features via an exponential moving average
- [x] Receive labels from user over USB or BLE
- [x] Train or validate on collected data
- [x] Infer labels based on reservoir state and output layer weights
- [x] Persist weights and EMA values using MbedOS KVStore
- [ ] Share and aggregate output layer weights between boards

## How to use

Please note that the code has only been tested for this fully qualified board: `--fqbn arduino:mbed_nano:nano33ble`.
With library versions: 

- `ArduinoBLE@1.4.1`
- `Arduino_LSM9DS1@1.1.1`

and platform `arduino:mbed_nano@4.4.1`

Update and upgrade core:

```bash
arduino-cli core update-index && arduino-cli core upgrade arduino:mbed_nano
```

Install libraries:
```bash
arduino-cli lib install "ArduinoBLE"
arduino-cli lib install "Arduino_LSM9DS1"
```

Take note of the relevant device

```bash
arduino-cli board list
```

Example output with `/dev/ttyACM0`

```tsv
Port         Protocol Type              Board Name          FQBN                        Core
/dev/ttyACM0 serial   Serial Port (USB) Arduino Nano 33 BLE arduino:mbed_nano:nano33ble arduino:mbed_nano
```

Compile the code:

```bash
arduino-cli compile --fqbn arduino:mbed_nano:nano33ble  . --verbose --clean
```

Upload binaries:
```bash
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:mbed_nano:nano33ble . --verbose
```



### File structure
```
PostureProject.ino // main Arduino file 
|--- engine.{cpp,h} // post init operation controller
|
|---COLLECTION: 
|        |---imu_features.{cpp,h} // IMU feature extractor, pre-processing and normalization
|        |---persistance.{cpp,h} // write processed weights + EMAs to KV store
|---MODEL: 
|        |---esn.{cpp,h} // define model architecture, train, predict, load weights
|---IO:
|        |---io.{cpp,h} // unified interface for all IO to either BLE or USB
|        |---ble.{cpp,h} // BLE IO
|        |---serial_protocol.{cpp,h} // USB IO
|        |---button.{cpp,h} // button functions
|        |---led.{cpp,h} // LED functions
```

