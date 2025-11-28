# TinyML Arduino Posture Classifier

Architecture:

PostureProject.ino // main file 

engine.cpp/.h // operation mode controller
|
|---COLLECTION: Use python helper during training data collection
|        |---imu_features.cpp/.h // IMU feature extractor & pre-processing
|        |---? data_store.cpp/.h // write processed data to flash memory
|---TRAINNING: 
|        |---esn.cpp/.h // define model architecture ? choose MLP or CNN
|        |---? trainer.cpp/.h // need to define a trainer?
|---VALIDATION: 
|        |---eval.cpp/.h // print evaluation results on test dataset, eg: confusion matrix, accuracy, F1 score
|---WEIGHT_SHARING:
|        |---? ble.cpp/h // enable BLE communication between devices
|        |---? federated.cpp/.h // implement federated learning
|---INFERENCE: 
|        |---?infer.cpp/.h // an inference pipeline that automatically detects input data, loads the trained model to predict
|---serial_protocol.cpp/.h // UI



