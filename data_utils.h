#pragma once

void train_val_split_indices_deterministic(
    size_t array_size, 
    double train_ratio, 
    unsigned fixed_seed,
    std::vector<uint16_t>& train_indices_out,
    std::vector<uint16_t>& val_indices_out
);
