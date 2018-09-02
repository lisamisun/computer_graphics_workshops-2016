#pragma once

#include <vector>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <cassert>
#include <iostream>

#include "../externals/EasyBMP/include/EasyBMP.h"
#include "../include/matrix.h"

#include "xmmintrin.h" // для SSE


typedef Matrix<float> FImage; // "слепок" изображения; матрица для модулей

FImage grayscale(BMP &);
FImage sobel_x(const FImage &);
FImage sobel_x_sse(const FImage &);
FImage sobel_y(const FImage &);
FImage sobel_y_sse(const FImage &);
FImage grad_abs(const FImage &, const FImage &);
FImage grad_abs_sse(const FImage &, const FImage &);

