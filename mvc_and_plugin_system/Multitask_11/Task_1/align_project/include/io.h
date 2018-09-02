#pragma once

#include "matrix.h"
#include "../externals/EasyBMP/include/EasyBMP.h"

#include <tuple>

typedef Matrix<std::tuple<uint, uint, uint>> Image;

Image load_image(const char*);
void save_image(const Image&, const char*);
