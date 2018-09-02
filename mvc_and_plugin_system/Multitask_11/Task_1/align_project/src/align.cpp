#include "../include/align.h"
#include <string>
#include <cfloat>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <vector>
#include <array>

using std::string;
using std::cout;
using std::endl;

#define RED 0
#define GREEN 1
#define BLUE 2

void Model::load(QString & path)
{
    srcImage = load_image(path.toStdString().c_str());
    model_obs.reset(path, 0); // для перерсовки представления
    model_obs.reset("Изображение загружено.", 1); // оповещаем представление
}

void Model::save(QString & path)
{
    save_image(srcImage, path.toStdString().c_str());
    model_obs.reset("Изображение сохранено.", 1);
}

Image & Model::get_image()
{
    return srcImage;
}

void Model::align()//, bool isPostprocessing, std::string postprocessingType, double fraction, bool isMirror,
//bool isInterp, bool isSubpixel, double subScale)
{
    // srcImage уже загружено
    uint width = srcImage.n_cols, height = srcImage.n_rows / 3;

    // это максимальный сдвиг; возьмем его как 5% от высоты и 5% от ширины
    int shift_h = height * 5 / 100, shift_w = width * 5 / 100;

    // делим srcImage на три различных изображения по каналам
    // сразу отступаем на 10% от границ для улучшения метрики
    uint ind_h = height * 10 / 100, ind_w = width * 10 / 100; // отступы по высоте и ширине
    uint height_wi = height - 2 * ind_h, width_wi = width - 2 * ind_w; // высота и ширина с отступами
    Image blueImage = srcImage.submatrix(ind_h, ind_w, height_wi, width_wi), // конструктор копирования
            greenImage = srcImage.submatrix(height + ind_h, ind_w, height_wi, width_wi),
            redImage = srcImage.submatrix(2 * height + ind_h, ind_w, height_wi, width_wi);

    model_obs.reset("Произведено разделение изображения на цветовые каналы.", 1); // уведомляем представление

    // среднеквадратичное отклонение

    // сначала берем минимум по green и red
    double min_mse = DBL_MAX; // минимум по среднеквадратичному отклонению
    int shift_imin_rg = 0, shift_jmin_rg = 0; // соответствующие сдвиги

    for (int shift_i = -shift_h; shift_i <= shift_h; shift_i++) { // эти два цикла - сдвиг одного изображения относительно другого
        for (int shift_j = -shift_w; shift_j <= shift_w; shift_j++) {
            unsigned long long sum_pix = 0; // сумма по всем перекрывающимся пикселям

            // тут считаем сумму для перекрывающейся области
            for (uint i = std::max(0, shift_i); i < height_wi + std::min(0, shift_i); i++) {
                for (uint j = std::max(0, shift_j); j < width_wi + std::min(0, shift_j); j++) { // i и j - для blueImage
                    long long int tmp = static_cast<int>(std::get<GREEN>(greenImage(i, j))) -
                            static_cast<int>(std::get<RED>(redImage(i - shift_i, j - shift_j)));
                    sum_pix += static_cast<unsigned long long>(tmp * tmp);
                }
            }

            double mse; // среднеквадратичное отклонение
            mse = sum_pix / static_cast<double>((height_wi - std::abs(shift_i)) * (width_wi - std::abs(shift_j)));

            if (mse < min_mse) { // новый минимум и его сдвиги
                min_mse = mse;
                shift_imin_rg = shift_i;
                shift_jmin_rg = shift_j;
            }
        }
    }

    // теперь берем минимум по green и blue
    min_mse = DBL_MAX; // минимум по среднеквадратичному отклонению
    int shift_imin_bg = 0, shift_jmin_bg = 0; // соответствующие сдвиги

    for (int shift_i = -shift_h; shift_i <= shift_h; shift_i++) { // эти два цикла - сдвиг одного изображения относительно другого
        for (int shift_j = -shift_w; shift_j <= shift_w; shift_j++) {
            unsigned long long sum_pix = 0; // сумма по всем перекрывающимся пикселям

            // тут считаем сумму для перекрывающейся области
            for (uint i = std::max(0, shift_i); i < height_wi + std::min(0, shift_i); i++) {
                for (uint j = std::max(0, shift_j); j < width_wi + std::min(0, shift_j); j++) { // i и j - для blueImage
                    long long int tmp = static_cast<int>(std::get<GREEN>(greenImage(i, j))) -
                            static_cast<int>(std::get<BLUE>(blueImage(i - shift_i, j - shift_j)));
                    sum_pix += static_cast<unsigned long long>(tmp * tmp);
                }
            }

            double mse; // среднеквадратичное отклонение
            mse = sum_pix / static_cast<double>((height_wi - std::abs(shift_i)) * (width_wi - std::abs(shift_j)));

            if (mse < min_mse) { // новый минимум и его сдвиги
                min_mse = mse;
                shift_imin_bg = shift_i;
                shift_jmin_bg = shift_j;
            }
        }
    }

    // теперь лепим все воедино

    // возвращаем отдельным цветам края
    blueImage = srcImage.submatrix(0, 0, height, width);
    greenImage = srcImage.submatrix(height, 0, height, width);
    redImage = srcImage.submatrix(2 * height, 0, height, width);

    // изображение-результат; остальные изображеня двигаем относительно него
    Image resImage(height + std::min(std::min(0, shift_imin_rg), shift_imin_bg) - std::max(std::max(0, shift_imin_rg), shift_imin_bg),
                   width + std::min(std::min(0, shift_jmin_rg), shift_jmin_bg) - std::max(std::max(0, shift_jmin_rg), shift_jmin_bg));

    uint shift_bi = std::max(std::max(0, shift_imin_rg), shift_imin_bg), // сдвиг для зеленого цвета
            shift_bj = std::max(std::max(0, shift_jmin_rg), shift_jmin_bg);
    // для других цветов смотрим сдвиги относительно зеленого

    for (uint i = 0; i < resImage.n_rows; i++) {
        for (uint j = 0; j < resImage.n_cols; j++) {
            resImage(i, j) = std::make_tuple(std::get<RED>(redImage(i + shift_bi - std::min(0, shift_imin_rg) - std::max(0, shift_imin_rg),
                                                                    j + shift_bj - std::min(0, shift_jmin_rg) - std::max(0, shift_jmin_rg))),
                                             std::get<GREEN>(greenImage(i + shift_bi, j + shift_bj)),
                                             std::get<BLUE>(blueImage(i + shift_bi - std::min(0, shift_imin_bg) - std::max(0, shift_imin_bg),
                                                                      j + shift_bj - std::min(0, shift_jmin_bg) - std::max(0, shift_jmin_bg))));
        }
    }

    save_image(resImage, "tmp.bmp"); // сохраняем временную копию

    model_obs.reset("tmp.bmp", 0); // для перерисовки представления
    model_obs.reset("Произведено совмещение каналов.", 1); // оповещаем представление

    srcImage = resImage;
}
