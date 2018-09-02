#include "gray_world.h"

#define RED 0
#define GREEN 1
#define BLUE 2

Gray_world::Gray_world()
{
}

const char * Gray_world::stringType()
{
    return "Gray world.";
}

void Gray_world::operation(Image & srcImage)
{
    double ave_red = 0, ave_green = 0, ave_blue = 0;

    // сложим яркости по каналам
    for (uint i = 0; i < srcImage.n_rows; i++) {
        for (uint j = 0; j < srcImage.n_cols; j++) {
            ave_red += std::get<RED>(srcImage(i, j));
            ave_green += std::get<GREEN>(srcImage(i, j));
            ave_blue += std::get<BLUE>(srcImage(i,j));
        }
    }

    // нормируем по площади - средние яркости
    uint square = srcImage.n_rows * srcImage.n_cols;
    ave_red /= square;
    ave_green /= square;
    ave_blue /= square;

    double ave = (ave_red + ave_green + ave_blue) / 3; // среднее средних

    // множители для пикселей
    ave_red = ave / ave_red;
    ave_green = ave / ave_green;
    ave_blue = ave / ave_blue;

    // собственно, фильтр
    for (uint i = 0; i < srcImage.n_rows; i++) {
        for (uint j = 0; j < srcImage.n_cols; j++) {
            uint new_red = static_cast<uint>(static_cast<double>(std::get<RED>(srcImage(i, j))) * ave_red),
                 new_green = static_cast<uint>(static_cast<double>(std::get<GREEN>(srcImage(i, j))) * ave_green),
                 new_blue = static_cast<uint>(static_cast<double>(std::get<BLUE>(srcImage(i,j))) * ave_blue);
            if (new_red > 255) { new_red = 255; }
            if (new_green > 255) { new_green = 255; }
            if (new_blue > 255) { new_blue = 255; }

            srcImage(i, j) = std::make_tuple(new_red, new_green, new_blue);
        }
    }
}
