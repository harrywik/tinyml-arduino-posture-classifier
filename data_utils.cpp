#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <cmath>

 /*
  * Usage:
  * std::vector<size_t> train_idxs, val_idxs;
  * train_val_split_indices_deterministic(N, RATIO, REPRODUCIBLE_SEED, train_idxs, val_idxs);
  * for (size_t i : train_idxs) {
  *    doStuff(train[i])
  * }
  */
void train_val_split_indices_deterministic(
    size_t array_size, 
    double train_ratio, 
    unsigned fixed_seed,
    std::vector<size_t>& train_indices_out,
    std::vector<size_t>& val_indices_out) 
{
    // Clear previous contents
    train_indices_out.clear();
    val_indices_out.clear();

    if (array_size == 0) {
        return;
    }

    // Generate initial indices (0 to array_size - 1)
    std::vector<size_t> indices(array_size);
    std::iota(indices.begin(), indices.end(), 0);

    // Initialize Random Engine with the SEED (deterministic)
    std::default_random_engine generator(fixed_seed); 

    // Shuffle
    std::shuffle(indices.begin(), indices.end(), generator);

    // Determine the split point
    size_t split_point = std::lround(array_size * train_ratio);

    // Copy the training part (from the start up to the split point)
    train_indices_out.assign(indices.begin(), indices.begin() + split_point);

    // Copy the validation part (from the split point to the end)
    val_indices_out.assign(indices.begin() + split_point, indices.end());
}
