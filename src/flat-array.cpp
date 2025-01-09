#pragma once

FlatArray FlatArray::FlatArray(uint8_t input_width, uint8_t input_length);

~FlatArray::FlatArray();

// You should not be copying or moving this data structure.
FlatArray::FlatArray(FlatArray && other) = delete;

// You should not be copying or moving this data structure.
FlatArray::FlatArray(const FlatArray & other) = delete;
