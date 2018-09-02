#include "unsharp.h"
#include <tuple>

#define RED 0
#define GREEN 1
#define BLUE 2

Unsharp::Unsharp()
{
}

const char * Unsharp::stringType()
{
    return "Unsharp.";
}

class ForCustom
{
public:
    std::tuple<uint, uint, uint> operator () (const Image &m) const
    {
        uint size = 2 * radius + 1;
        uint red, green, blue;
        double sum_red = 0, sum_green = 0, sum_blue = 0;
        for (uint i = 0; i < size; i++) {
            for (uint j = 0; j < size; j++) {
                std::tie(red, green, blue) = m(i, j);
                sum_red += static_cast<double>(red) * weight(i, j);
                sum_green += static_cast<double>(green) * weight(i, j);
                sum_blue += static_cast<double>(blue) * weight(i, j);
            }
        }
        if (sum_red < 0) { sum_red = 0; }
        if (sum_red > 255) { sum_red = 255; }

        if (sum_green < 0) { sum_green = 0; }
        if (sum_green > 255) { sum_green = 255; }

        if (sum_blue < 0) { sum_blue = 0; }
        if (sum_blue > 255) { sum_blue = 255; }

        return std::make_tuple(static_cast<uint>(sum_red), static_cast<uint>(sum_green), static_cast<uint>(sum_blue));
    }

    static uint radius;
    static Matrix<double> weight;
};

uint ForCustom::radius;
Matrix<double> ForCustom::weight;

Image custom(Image srcImage, Matrix<double> kernel) {
    // Function custom is useful for making concrete linear filtrations
    // like gaussian or sobel. So, we assume that you implement customapply
    // and then implement other filtrations using this function.
    // sobel_x and sobel_y are given as an example.

    ForCustom::weight = kernel;

    srcImage = srcImage.unary_map(ForCustom());

    return srcImage;
}

void Unsharp::operation(Image & srcImage)
{
    Matrix<double> kernel = {{-1 / 6.0, -2 / 3.0, -1 / 6.0},
                             { -2 / 3.0, 13 / 3.0, -2 / 3.0},
                             {-1 / 6.0, -2 / 3.0, -1 / 6.0}};

    ForCustom::radius = 1;
    srcImage = custom(srcImage, kernel);
}
