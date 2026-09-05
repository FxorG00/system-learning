import numpy as np

original = np.arange(6, dtype=np.int32)
window = original[2:5]

window[0] = 99

print(original)  # [ 0  1 99  3  4  5]
print(window)    # [99  3  4]