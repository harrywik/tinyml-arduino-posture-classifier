# TinyML Arduino Posture Classifier

Architecture:

**TODO parts are marked with ?**
PostureProject.ino // main file 

engine.cpp/.h // operation mode controller
|
|---COLLECTION: 
|        |---imu_features.cpp/.h // IMU feature extractor & pre-processing **? load stored data and normalize, split datasets into train, vallidation and test**
|        |---**? data_store.cpp/.h // write processed data to flash memory**
|
|---TRAINNING: 
|        |---esn.cpp/.h // define model architecture 
         |--- **? trainer.cpp/.h  // load stored processed data, train the model**
|  
|---VALIDATION: 
|        |---**? eval.cpp/.h // print evaluation results on test dataset, eg: confusion matrix, accuracy, F1 score**
|
|---WEIGHT_SHARING:
|        
|        |---**? federated.cpp/.h // implement federated learning**
|
|---INFERENCE: 
|        |---**?infer.cpp/.h // an inference pipeline that automatically detects input data, loads the trained model to predict**

**? io.cpp/.h: // set operation mode, either serial or ble**
|       |---serial_protocol.cpp/.h // serial communication
|       |---**? ble.cpp/h // enable BLE communication between devices**



