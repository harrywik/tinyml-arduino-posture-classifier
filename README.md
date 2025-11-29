# TinyML Arduino Posture Classifier

Architecture:

```
PostureProject.ino // main Arduino file 
|--- engine.{cpp,h} // post init operation controller
|
|---COLLECTION: 
|        |---imu_features.{cpp,h} // IMU feature extractor, pre-processing and normalization
|        |---persist_weights.{cpp,h} // write processed weights to flash memory
|---MODEL: 
|        |---esn.{cpp,h} // define model architecture, train, predict, load weights
|---IO:
|        |---io.{cpp,h} // unified interface for all IO to either BLE or USB
|        |---ble.{cpp,h} // BLE IO
|        |---serial_protocol.{cpp,h} // USB IO
|        |---button.{cpp,h} // button functions
```

