### Priorities

- [x] Hyperparameter tuning:
    - [x] WINDOW_SIZE
    - [x] BATCH_SIZE
    - [x] EMA_ALPHA
    - [x] LEARNING_RATE
    - [x] RESERVOIR_SIZE
- [ ] Persist the number of batches that have been processed
- [ ] Weight sharing / Federated learning
    - [ ] CMD_SHARE_WEIGHTS to both devices
    - [ ] One receives the MAC-address of the counterparty
          this triggers a detachment from any ongoing BLE peripheral service
          the other device will not receive a MAC-address but will
          after a timeout also detach any ongoing BLE session.
    - [ ] The device that got a MAC-address is now the central device.
          It establishes using the provided MAC a connection to the peripheral device.
          On connection, the central sends its number of processed batches.
          The peripheral responds with its number of processed batches.
          Then the central device will send its output layer weights
          and as a response get the peripheral's.
          Both parties now have the necessary information 
          and will update the output weights according to:
          $$
            W_{out}^{(a)} = \frac{n^{(a)} W_{out}^{(a)} + n^{(b)} W_{out}^{(b)}}{n^{(a)} + n^{(b)}}
          $$

    - [ ] Persist the new weights 
    - [ ] Persist the new number of batches having been processed to be $n^{(a)} + n^{(b)}$
    - [ ] Communicate to the user as usual by turning the LED green
