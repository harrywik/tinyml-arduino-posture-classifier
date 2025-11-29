# TinyML Arduino Posture Classifier

Architecture:

PostureProject.ino // main file 

```
|--- engine.cpp/.h // operation mode controller
|
|---COLLECTION: Use python helper during training data collection
|        |---imu_features.cpp/.h // IMU feature extractor, pre-processing and normalization
|        |---persist_weights.cpp/.h // write processed weights to flash memory
|---MODEL: 
|        |---esn.cpp/.h // define model architecture, train, predict, load weights
|---IO:
|        |---io.{cpp,h} //
|        |---ble.cpp/h // enable BLE communication between devices
|        |---serial_protocol.cpp/.h // IO
```

