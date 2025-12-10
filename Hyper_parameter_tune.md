## Records of tuning

**RESERVOIR_SIZE**

    default: 20; to test: 10, 40, 15

* 20: train_acc 100%(2 epochs); val_idation _acc 66.67%; test_acc 25% 
    * The results appeared to be very random. Tried more epochs, but val_acc dropped to 18.9% with 4 epochs, indicating overfitting.

* 10: train_acc 90%(3 epochs, an additional iter for label 0); val_acc 64.58%, test_acc 60.4% 
    * The performance was very unstable when I trained the model over 4 epochs. It still did not achieve high acc, either train or validation. The reservoir might be too small.

* 40: train_acc: 100%; val_acc: 43.75%; 
    * Trained with 2 epochs at first, overfitted. Trid 1 epoch insted, but val_acc was still very low.

* 15: train_acc 100% (2 epochs); val_acc 37.5%

* Set RESERVOIR_SIZE = 20, which gives the most stable performance.

**LEARNING_RATE** 

    default: 0.01; to test: 0.001, 0.1

* 0.001: train acc 100%(2 epochs); val_acc 66.67%; test_acc 66.67%

* 0.1: val_acc 56.25% (1 epoch).
    * The performance was very unstable, and hard to get improvement during training. When set Re_Size to 30, better, val_acc 66.67%. 

* After further testing, 0.01 is a balanced setting of LEARNING_RATE.

**Findings so far**

The model memorizes the previous states, leading to inaccurate predictions during validation and inference stage. Foe example, after validated on label 2, the model tends to predict label 2 during the first inference round no matter what the current input is. Thus, the window_size might not be set too large, otherwise the update of reservoir would be slow. 

**WINDOW_SIZE**
    
    default: 128; to test: 64, 256

* 128: Combined with batch_size 32, the memeory lagging problem went even worse.

* 64: Very hard to train. The model could not learn (very low train acc after many epochs).

* 256: Got stucked (no response when "TRAIN"). Maybe out of momery?

* Thus, keep window_size at 128, and try reducing batch_size instead.

**BATCH_SIZE** 
    
    default: 16; to test: 8, 32

* 8: train acc: 88.24%, val acc: 75%, test acc very low( so the val acc might just be of lucky).      
    * Need to train more epochs. The model is trained with 5 epochs, and 2 more iters for weak classes. 
    * The model appeared to be hard to jump out of local minimum. Once the model got confused with two classes, it was difficult to adjust. Had to increase lr to 0.1 and got train_acc: 100%(2 epochs); val_acc: 72.92%; test_acc: 60.42%.

* 32: The memeory lagging problem occurs again. Failed to achieve val_acc higher than 60%.

* Optimum choice of BATCH_SIZE = 8.

**EMA_ALPHA**
    
    default: 0.01; to test: 0.005, 0.1

* 0.1: 62.5% val_acc after 5~7 epochs
    * The model appeared to be sensitive and unstable (eg: for two infer rounds in a row when tested at the same input label, the model gives two different predictions)

* 0.005: **THE BEST!** train_acc: 100%(2 epochs); val_acc: 100%; test_acc: 91.67% 
    * This is achieved when I skip updateEMA(state) during continuous reservoir updating. This might be unexpected, but the model performs perfectly after removing updateEMA(state).

* 0.01: 
    * Also achieved almost 100% val_acc and test_acc, but 3 epochs required, and the model was less stable compared to the previous setting.
    
* Optimum choice of EMA_ALPHA = 0.005.

## Conclusions:

1. Best to define three classes as **inactive(still)**, **walking(gently waving arms)**, **active(doing puches or shaking arms rapidly)**, ortherwise the model might fail to distinguish similar classes.

2. **Skip updateEMA(state) during continuous reservoir updating**. Otherwise, the normalization would be massed up by the noises. This improves the performance significantly!

3. **Bset parameter setting**: RESERVOIR_SIZE = 20, LEARNING_RATE = 0.01, WINDOW_SIZE = 128, BATCH_SIZE = 8, EMA_ALPHA = 0.005.
