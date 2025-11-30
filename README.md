# TinyML Arduino Posture Classifier

File structure:
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

