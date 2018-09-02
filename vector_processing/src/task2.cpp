//! \file

/*!
\mainpage 

Данная документация содержит описание функций файла \b task_2.cpp второго задания машграфа.

Функция <b>int main()</b> по сравнению с заданием машграфа переделана под нужды пратикума следующим образом:
<ul> 
    <li> удалены все команды предыдущей версии этой функции;
    <li> позаимствованы команды функции <b>void TrainClassifier(const string& data_file, const string& model_file)</b> для формирования структуры изображений;
    <li> присутствует команда запуска функции void ExtractFeatures(const TDataSet& data_set, TFeatures* features), в этой функции (помимо её основного начначения, указанного в документации) также происходит подсчёт времени работы нужных нам функций.
</ul>

Единственный требуемый аргумент для запуска программы - путь до файла, содержащего пути до всех изображений и "ярлыки" к ним (типа test_labels.txt и train_labels.txt из машграфа). Проверки на правильность входных данных не проводятся, лучше не экспериментировать.

*/

#include <string>
#include <vector>
#include <fstream>
#include <cassert>
#include <iostream>
#include <cmath>

#include "classifier.h"
#include "EasyBMP.h"
#include "linear.h"
#include "argvparser.h"
#include "matrix.h"
#include "Timer.h"

#include "xmmintrin.h" // для SSE

using std::string;
using std::vector;
using std::ifstream;
using std::ofstream;
using std::pair;
using std::make_pair;
using std::cout;
using std::cerr;
using std::endl;

using CommandLineProcessing::ArgvParser;

typedef vector<pair<BMP*, int> > TDataSet;
typedef vector<pair<string, int> > TFileList;
typedef vector<pair<vector<float>, int> > TFeatures;

typedef Matrix<float> FImage; // "слепок" изображения; матрица для модулей и углов
typedef Matrix<vector<float> > HistoMatrix; // матрица для гистограмм

#define CELLS 5 // делим изображение CELLS x CELLS блоков
#define SEGMENTS 20 // делим область изменения направления градиента на сегменты
#define COLOR_CELLS 6
#define LBP_CELLS 6

// Load list of files and its labels from 'data_file' and
// stores it in 'file_list'
/*!
Загружает имена файлов и их "ярлыки" из директории с данными.
\param data_file Директория с данными
\param file_list Сформированный список имен файлов с "ярлыками"
*/
void LoadFileList(const string& data_file, TFileList* file_list) {
    ifstream stream(data_file.c_str());

    string filename;
    int label;
    
    int char_idx = data_file.size() - 1;
    for (; char_idx >= 0; --char_idx)
        if (data_file[char_idx] == '/' || data_file[char_idx] == '\\')
            break;
    string data_path = data_file.substr(0,char_idx+1);
    
    while(!stream.eof() && !stream.fail()) {
        stream >> filename >> label;
        if (filename.size())
            file_list->push_back(make_pair(data_path + filename, label));
    }

    stream.close();
}

// Load images by list of files 'file_list' and store them in 'data_set'
/*!
Загружает изображения согласно списку имен файлов.
\param file_list Список доступных файлов с "ярлыками"
\param data_set Загруженные изображения с "ярлыками"
*/
void LoadImages(const TFileList& file_list, TDataSet* data_set) {
    for (size_t img_idx = 0; img_idx < file_list.size(); ++img_idx) {
            // Create image
        BMP* image = new BMP();
            // Read image from file
        image->ReadFromFile(file_list[img_idx].first.c_str());
            // Add image and it's label to dataset
        data_set->push_back(make_pair(image, file_list[img_idx].second));
    }
}

// Save result of prediction to file
/*!
Сохраняет результаты классификации изображений в файл
\param file_list Список доступных файлов с "ярлыками"
\param labels "Ярлыки" для файлов после классификации
\param prediction_file Имя редактируемого файла
*/
void SavePredictions(const TFileList& file_list,
                     const TLabels& labels, 
                     const string& prediction_file) {
        // Check that list of files and list of labels has equal size 
    assert(file_list.size() == labels.size());
        // Open 'prediction_file' for writing
    ofstream stream(prediction_file.c_str());

        // Write file names and labels to stream
    for (size_t image_idx = 0; image_idx < file_list.size(); ++image_idx)
        stream << file_list[image_idx].first << " " << labels[image_idx] << endl;
    stream.close();
}

// Преобразование изображения в оттенки серого и изображения в формат MImage заодно
/*!
Преобразование цветного изображения в оттенки серого
\param image Обрабатываемое изображение
\return Изображение в оттенках серого
*/
FImage grayscale(BMP & image) 
{
    FImage result(image.TellHeight(), image.TellWidth());

    // Y = 0.299R + 0.587G + 0.114B - яркость пикселя изображения
    for (uint i = 0; i < result.n_rows; i++) {
        for (uint j = 0; j < result.n_cols; j++) {
            RGBApixel *pixel = image(j, i);
            float gs_pix = 0.299 * pixel->Red + 0.587 * pixel->Green + 0.114 * pixel->Blue; // яркость пикселя

            result(i, j) = gs_pix; // как бы выставляем все каналы одинаковыми
        }
    }

    return result;
}

// горизонтальный фильтр Собеля
/*!
Вычисляет горизонтальную составляющую вектора градиента для каждого пискселя изображения с помощью фильтра Собеля без использования SSE
\param image Изображение в оттенках серого
\return Горизонтальную составляющую вектора градиента для каждого пискселя изображения
*/
FImage sobel_x(const FImage & image)
{
    FImage result(image.n_rows, image.n_cols); // по краям остаются нули, но теперь это легко обрезать

    for (uint i = 1; i < image.n_rows - 1; i++) {
    	for (uint j = 1; j < image.n_cols - 1; j++) {

            float pixel_left_up = image(i - 1, j - 1);
            float pixel_left = image(i, j - 1);
            float pixel_left_down = image(i + 1, j - 1);

            float pixel_right_up = image(i - 1, j + 1);
            float pixel_right = image(i, j + 1);
            float pixel_right_down = image(i + 1, j + 1);

            result(i, j) = (-1) * pixel_left_up + (-2) * pixel_left + (-1) * pixel_left_down +
                           1 * pixel_right_up + 2 * pixel_right + 1 * pixel_right_down;
	}
    }

    return result.submatrix(1, 1, result.n_rows - 2, result.n_cols - 2); // обрезали
}

// горизонтальный фильтр Собеля с SSE
/*!
Вычисляет горизонтальную составляющую вектора градиента для каждого пискселя изображения с помощью фильтра Собеля с использованием SSE
\param image Изображение в оттенках серого
\return Горизонтальную составляющую вектора градиента для каждого пискселя изображения
*/
FImage sobel_x_sse(const FImage & image)
{
// наверное, логично отрабатывать сразу по четыре пикселя в строке
// сделать переменную для каждого пикселя из окружения и засунуть в неё по четыре значения float
// это же логично, ВЕДЬ ДА?

    FImage result(image.n_rows, image.n_cols); // по краям остаются нули, но теперь это легко обрезать

    // для строки возможно обрабатывать только кратное 4 число элементов, по остальным проходимся обычным способом
    uint proc_size = 4; // число одновременно обрабатываемых пикселей
    uint cant_be_sse = (image.n_cols - 2) % proc_size; // сколько пикселей в строке останутся необработанными
    uint cols_for_sse = image.n_cols - 1 - cant_be_sse; // сколько пикселей в строке могут быть обработаны

    for (uint i = 1; i < image.n_rows - 1; i++) {
        for (uint j = 1; j < cols_for_sse; j += proc_size) {
            // левый блок пикселей для параллельной обработки
            __m128 pixel_left_up = _mm_setr_ps(image(i - 1, j - 1), image(i - 1, j), image(i - 1, j + 1), image(i - 1, j + 2));
            __m128 pixel_left = _mm_setr_ps(image(i, j - 1), image(i, j), image(i, j + 1), image(i, j + 2));
            __m128 pixel_left_down = _mm_setr_ps(image(i + 1, j - 1), image(i + 1, j), image(i + 1, j + 1), image(i + 1, j + 2));

            // правый блок пикселей для параллельной обработки
            __m128 pixel_right_up = _mm_setr_ps(image(i - 1, j + 1), image(i - 1, j + 2), image(i - 1, j + 3), image(i - 1, j + 4));
            __m128 pixel_right = _mm_setr_ps(image(i, j + 1), image(i, j + 2), image(i, j + 3), image(i, j + 4));
            __m128 pixel_right_down = _mm_setr_ps(image(i + 1, j + 1), image(i + 1, j + 2), image(i + 1, j + 3), image(i + 1, j + 4));

            // теперь нам вроде нужны константы
            const __m128 const_p_one = _mm_set1_ps(+1.0f);
            const __m128 const_p_two = _mm_set1_ps(+2.0f);
            const __m128 const_n_one = _mm_set1_ps(-1.0f);
            const __m128 const_n_two = _mm_set1_ps(-2.0f);
            // ура константам!

            // теперь надо всё умножить и сложить
            __m128 res = _mm_add_ps(_mm_mul_ps(const_n_one, pixel_left_up), 
                         _mm_add_ps(_mm_mul_ps(const_n_two, pixel_left),
                         _mm_add_ps(_mm_mul_ps(const_n_one, pixel_left_down),
                         _mm_add_ps(_mm_mul_ps(const_p_one, pixel_right_up),
                         _mm_add_ps(_mm_mul_ps(const_p_two, pixel_right),
                                    _mm_mul_ps(const_p_one, pixel_right_down))))));

            // а потом вытащить значения в result
            float help_take_res[4]; // сюда выгрузим готовые значения
            _mm_storeu_ps(help_take_res, res);
            result(i, j) = help_take_res[0]; // теперь перепишем в FImage
            result(i, j + 1) = help_take_res[1];
            result(i, j + 2) = help_take_res[2];
            result(i, j + 3) = help_take_res[3];
        }

        for (uint j = cols_for_sse; j < image.n_cols - 1; j++) {

            float pixel_left_up = image(i - 1, j - 1);
            float pixel_left = image(i, j - 1);
            float pixel_left_down = image(i + 1, j - 1);

            float pixel_right_up = image(i - 1, j + 1);
            float pixel_right = image(i, j + 1);
            float pixel_right_down = image(i + 1, j + 1);

            result(i, j) = (-1) * pixel_left_up + (-2) * pixel_left + (-1) * pixel_left_down +
                           1 * pixel_right_up + 2 * pixel_right + 1 * pixel_right_down;
        }
    }

    return result.submatrix(1, 1, result.n_rows - 2, result.n_cols - 2); // обрезали
}

// вертикальный фильтр Собеля
// переполнения быть не должно
/*!
Вычисляет вертикальную составляющую вектора градиента для каждого пискселя изображения с помощью фильтра Собеля без использования SSE
\param image Изображение в оттенках серого
\return Вертикальную составляющую вектора градиента для каждого пискселя изображения
*/
FImage sobel_y(const FImage & image)
{
    FImage result(image.n_rows, image.n_cols); // по краям остаются нули, но теперь это легко обрезать

    for (uint i = 1; i < image.n_rows - 1; i++) {
    	for (uint j = 1; j < image.n_cols - 1; j++) {
            float pixel_up_left = image(i - 1, j - 1);
            float pixel_up = image(i - 1, j);
            float pixel_up_right = image(i - 1, j + 1);

            float pixel_down_left = image(i + 1, j - 1);
            float pixel_down = image(i + 1, j);
            float pixel_down_right = image(i + 1, j + 1);

            result(i, j) = (-1) * pixel_up_left + (-2) * pixel_up + (-1) * pixel_up_right +
                           1 * pixel_down_left + 2 * pixel_down + 1 * pixel_down_right; 
	}
    }

    return result.submatrix(1, 1, result.n_rows - 2, result. n_cols - 2); // обрезали
}

// вертикальный фильтр Собеля с SSE
/*!
Вычисляет вертикальную составляющую вектора градиента для каждого пискселя изображения с помощью фильтра Собеля с использованием SSE
\param image Изображение в оттенках серого
\return Вертикальную составляющую вектора градиента для каждого пискселя изображения
*/
FImage sobel_y_sse(const FImage & image)
{
    FImage result(image.n_rows, image.n_cols); // по краям остаются нули, но теперь это легко обрезать

    // для строки возможно обрабатывать только кратное 4 число элементов, по остальным проходимся обычным способом
    uint proc_size = 4; // число одновременно обрабатываемых пикселей
    uint cant_be_sse = (image.n_cols - 2) % proc_size; // сколько пикселей в строке останутся необработанными
    uint cols_for_sse = image.n_cols - 1 - cant_be_sse; // сколько пикселей в строке могут быть обработаны

    for (uint i = 1; i < image.n_rows - 1; i++) {
        for (uint j = 1; j < cols_for_sse; j += proc_size) {
            // верхний блок пикселей для параллельной обработки
            __m128 pixel_up_left = _mm_setr_ps(image(i - 1, j - 1), image(i - 1, j), image(i - 1, j + 1), image(i - 1, j + 2));
            __m128 pixel_up = _mm_setr_ps(image(i - 1, j), image(i - 1, j + 1), image(i - 1, j + 2), image(i - 1, j + 3));
            __m128 pixel_up_right = _mm_setr_ps(image(i - 1, j + 1), image(i - 1, j + 2), image(i - 1, j + 3), image(i - 1, j + 4));

            // нижний блок пикселей для параллельной обработки
            __m128 pixel_down_left = _mm_setr_ps(image(i + 1, j - 1), image(i + 1, j), image(i + 1, j + 1), image(i + 1, j + 2));
            __m128 pixel_down = _mm_setr_ps(image(i + 1, j), image(i + 1, j + 1), image(i + 1, j + 2), image(i + 1, j + 3));
            __m128 pixel_down_right = _mm_setr_ps(image(i + 1, j + 1), image(i + 1, j + 2), image(i + 1, j + 3), image(i + 1, j + 4));

            // теперь нам вроде нужны константы
            const __m128 const_p_one = _mm_set1_ps(+1.0f);
            const __m128 const_p_two = _mm_set1_ps(+2.0f);
            const __m128 const_n_one = _mm_set1_ps(-1.0f);
            const __m128 const_n_two = _mm_set1_ps(-2.0f);
            // ура константам!

            // теперь надо всё умножить и сложить
            __m128 res = _mm_add_ps(_mm_mul_ps(const_n_one, pixel_up_left), 
                         _mm_add_ps(_mm_mul_ps(const_n_two, pixel_up),
                         _mm_add_ps(_mm_mul_ps(const_n_one, pixel_up_right),
                         _mm_add_ps(_mm_mul_ps(const_p_one, pixel_down_left),
                         _mm_add_ps(_mm_mul_ps(const_p_two, pixel_down),
                                    _mm_mul_ps(const_p_one, pixel_down_right))))));

            // а потом вытащить значения в result
            float help_take_res[4]; // сюда выгрузим готовые значения
            _mm_storeu_ps(help_take_res, res);
            result(i, j) = help_take_res[0]; // теперь перепишем в FImage
            result(i, j + 1) = help_take_res[1];
            result(i, j + 2) = help_take_res[2];
            result(i, j + 3) = help_take_res[3];
        }

        for (uint j = cols_for_sse; j < image.n_cols - 1; j++) {

            float pixel_up_left = image(i - 1, j - 1);
            float pixel_up = image(i - 1, j);
            float pixel_up_right = image(i - 1, j + 1);

            float pixel_down_left = image(i + 1, j - 1);
            float pixel_down = image(i + 1, j);
            float pixel_down_right = image(i + 1, j + 1);

            result(i, j) = (-1) * pixel_up_left + (-2) * pixel_up + (-1) * pixel_up_right +
                           1 * pixel_down_left + 2 * pixel_down + 1 * pixel_down_right; 
        }
    }

    return result.submatrix(1, 1, result.n_rows - 2, result.n_cols - 2); // обрезали
}

/*!
Вычисляет модуль вектора градиента для каждого пискселя изображения без использования SSE
\param x Горизонтальные составляющие вектора градиента для каждого пикселя изображения
\param y Вертикальные составляющие вектора градиента для каждого пикселя изображения
\return Модуль градиента для каждого пикселя изображения
*/
FImage grad_abs(const FImage & x, const FImage & y)
{
    FImage result(x.n_rows, x.n_cols); 

    for (uint i = 0; i < x.n_rows; i++) {
        for (uint j = 0; j < x.n_cols; j++) {
            result(i, j) = sqrt(x(i, j) * x(i, j) + y(i, j) * y(i, j));
        }
    }

    return result;
}

// модуль градиента с SSE
/*!
Вычисляет модуль вектора градиента для каждого пискселя изображения с использованием SSE
\param x Горизонтальные составляющие вектора градиента для каждого пикселя изображения
\param y Вертикальные составляющие вектора градиента для каждого пикселя изображения
\return Модуль градиента для каждого пикселя изображения
*/
FImage grad_abs_sse(const FImage & x, const FImage & y)
{
    FImage result(x.n_rows, x.n_cols); 

    // для строки возможно обрабатывать только кратное 4 число элементов, по остальным проходимся обычным способом
    uint proc_size = 4; // число одновременно обрабатываемых пикселей
    uint cant_be_sse = x.n_cols % proc_size; // сколько пикселей в строке останутся необработанными
    uint cols_for_sse = x.n_cols - cant_be_sse; // сколько пикселей в строке могут быть обработаны

    for (uint i = 0; i < x.n_rows; i++) {
        for (uint j = 0; j < cols_for_sse; j += proc_size) {
            // пиксели для параллельной обработки
            __m128 pixel_x = _mm_setr_ps(x(i, j), x(i, j + 1), x(i, j + 2), x(i, j + 3));
            __m128 pixel_y = _mm_setr_ps(y(i, j), y(i, j + 1), y(i, j + 2), y(i, j + 3));

            // считаем модуль
            __m128 res = _mm_sqrt_ps(_mm_add_ps(_mm_mul_ps(pixel_x, pixel_x), _mm_mul_ps(pixel_y, pixel_y)));

            // а потом вытащить значения в result
            float help_take_res[4]; // сюда выгрузим готовые значения
            _mm_storeu_ps(help_take_res, res);
            result(i, j) = help_take_res[0]; // теперь перепишем в FImage
            result(i, j + 1) = help_take_res[1];
            result(i, j + 2) = help_take_res[2];
            result(i, j + 3) = help_take_res[3];
        }

        for (uint j = cols_for_sse; j < x.n_cols; j++) {
            result(i, j) = sqrt(x(i, j) * x(i, j) + y(i, j) * y(i, j));
        }
    }

    return result;
}

// число пи - M_PI
/*!
Вычисляет направление вектора градиента для каждого пискселя изображения
\param x Горизонтальные составляющие вектора градиента для каждого пикселя изображения
\param y Вертикальные составляющие вектора градиента для каждого пикселя изображения
\return Направление градиента для каждого пикселя изображения
*/
FImage grad_dest(const FImage & x, const FImage & y)
{
    FImage result(x.n_rows, x.n_cols);

    for (uint i = 0; i < x.n_rows; i++) {
        for (uint j = 0; j < x.n_cols; j++) {
            result(i, j) = atan2(y(i, j), x(i, j)); // atan2 вроде контролирует нули  
        }
    }

    return result;
}

/*!
Вычисляет цветовые признаки для изображения
\param image Исследуемое изображение
\return Дескриптор цветовых признаков
*/
vector<float> color_features(BMP & image)
{
    vector<float> result; // вектор цветов

    FImage red(COLOR_CELLS, COLOR_CELLS), 
           green(COLOR_CELLS, COLOR_CELLS), 
           blue(COLOR_CELLS, COLOR_CELLS); // матрицы для средних цветов каждой клетки

    // обнуляем эти матрицы
    for (uint i = 0; i < red.n_rows; i++) {
        for (uint j = 0; j < red.n_cols; j++) {
            red(i, j) = green(i, j) = blue(i, j) = 0;
        }
    }

    int cell_h = image.TellHeight() / COLOR_CELLS, cell_w = image.TellWidth() / COLOR_CELLS; // ширина и высота одной клетки в пикселях
    for (int i = 0; i < image.TellHeight(); i++) {
        for (int j = 0; j < image.TellWidth(); j++) {
            // нужно определить в какую именно клетку по вертикали и горизонтали попадает пиксель
            uint place_i = i / cell_h, place_j = j / cell_w; // из какой пиксель клетки
            if (place_i >= COLOR_CELLS) { place_i = COLOR_CELLS - 1; } // боковые пиксели относятся к последней клетке
            if (place_j >= COLOR_CELLS) { place_j = COLOR_CELLS - 1; }

            RGBApixel *pixel = image(j, i); // вытаскиваем пиксель

            red(place_i, place_j) += pixel->Red; 
            green(place_i, place_j) += pixel->Green;
            blue(place_i, place_j) += pixel->Blue;
        }
    }

    // считаем средний цвет для каждой клетки как среднее арифметическое
    for (uint i = 0; i < red.n_rows; i++) {
        for (uint j = 0; j < red.n_cols; j++) {
            int num_pix; // число пикселей в клетке
            
            if (i == red.n_rows - 1) { // по краю снизу клетка может быть больше обычного
                num_pix = cell_h + image.TellHeight() % COLOR_CELLS; 
            } else {
                num_pix = cell_h;
            }

            if (j == red.n_cols - 1) { // по краю справа клетка может быть больше обычного
                num_pix *= cell_w + image.TellWidth() % COLOR_CELLS; 
            } else {
                num_pix *= cell_w;
            }

            red(i, j) /= num_pix;
            green(i, j) /= num_pix;
            blue(i, j) /= num_pix;

            // сразу нормируем и заносим в результирующий вектор
            result.push_back(red(i, j) / 255);
            result.push_back(green(i, j) / 255);
            result.push_back(blue(i, j) / 255);
        }
    }

    return result;
}

/*!
Составляет дескриптор локальных бинарных шаблонов для изображения
\param image Исследуемое изображение
\return Дескриптор локальных бинарных шаблонов
*/
vector<float> local_binary_patterns(const FImage & image)
{
    vector<float> result;

    FImage bins(image.n_rows - 2, image.n_cols - 2);
    for (uint i = 1; i < image.n_rows - 1; i++) {
        for (uint j = 1; j < image.n_cols - 1; j++) {
            // смотрим на соседей каждого пикселя
            uint nei = 0; // какой по счёту сосед
            uint bin_code = 0; // бинарный код для (i, j)-го пикселя

            for (uint nei_i = i - 1; nei_i <= i + 1; nei_i++) {
                for (uint nei_j = j - 1; nei_j <= j + 1; nei_j++) {

                    if ((nei_i != i) || (nei_j != j)) { // если это не сам пиксель
                        if (image(i, j) <= image(nei_i, nei_j)) { bin_code = bin_code | (1 << nei); } 
                        
                        nei++; // следующий сосед; всего их 8
                    }
                    
                }
            }
           
            bins(i - 1, j - 1) = bin_code; // теперь каждый пиксель имеет свой бинарный код, который число 0..255
        }
    }

    HistoMatrix histo(LBP_CELLS, LBP_CELLS); // гистограммы каждой клетки
    for (uint i = 0; i < LBP_CELLS; i++) {
        for (uint j = 0; j < LBP_CELLS; j++) {
            for (uint k = 0; k < 256; k++) { // обнуляем каждую гистограмму
                histo(i, j).push_back(0);
            }
        }
    }

    uint cell_h = bins.n_rows / LBP_CELLS, cell_w = bins.n_cols / LBP_CELLS; // ширина и высота одной клетки в пикселях
    // если cell_h или cell_w получается меньше остатка от деления на LBP_CELLS, то клетки снизу и справа сильно больше остальных
    // ну и ничего страшного, исправления получаются недокостылями
    for (uint i = 0; i < bins.n_rows; i++) {
        for (uint j = 0; j < bins.n_cols; j++) {
            // нужно определить в какую именно клетку по вертикали и горизонтали попадает пиксель
            // а потом найти для него место в гистограмме
            uint place_i = i / cell_h, place_j = j / cell_w; // из какой пиксель клетки
            if (place_i >= LBP_CELLS) { place_i = LBP_CELLS - 1; } // боковые пиксели относятся к последней клетке
            if (place_j >= LBP_CELLS) { place_j = LBP_CELLS - 1; }  

            histo(place_i, place_j)[bins(i, j)]++; // формируем гистограмму
        }
    }

    FImage norms(LBP_CELLS, LBP_CELLS); // тут считаем евклидову норму каждой гистограммы
    for (uint i = 0; i < LBP_CELLS; i++) {
        for (uint j = 0; j < LBP_CELLS; j++) {

            norms(i, j) = 0;
            for (uint k = 0; k < 256; k++) {
                norms(i, j) += histo(i, j)[k] * histo(i, j)[k];
            }
            norms(i, j) = sqrt(norms(i, j));

            if (norms(i, j) > 0) {
                for (uint k = 0; k < 256; k++) {
                    histo(i, j)[k] /= norms(i, j); // нормализуем гистограммы
                }
            }
            
            result.insert(result.end(), histo(i, j).begin(), histo(i, j).end()); // конкатенируем все гистограммы
        }
    }

    return result;
}

// Exatract features from dataset.
// You should implement this function by yourself =)
/*!
Извлекает признаки из каждого загруженного изображения
\param data_set Загруженные изображения
\param features Извлечённые признаки
*/
void ExtractFeatures(const TDataSet& data_set, TFeatures* features) {
    Timer ftime; // поможет замерять время работы функций
    // время считаем просто среднее на всем датасете

    vector<double> sob_x, sob_x_sse; // это векторы времени для каждой картинки
    vector<double> sob_y, sob_y_sse;
    vector<double> vabs, vabs_sse;

    for (size_t image_idx = 0; image_idx < data_set.size(); ++image_idx) {
        // PLACE YOUR CODE HERE
        // Remove this sample code and place your feature extraction code here

        BMP image = *(data_set[image_idx].first); // изображение, которое мучаем

        // цветовые признаки
        vector<float> color = color_features(image);
        // конец цветовых признаков

        // HOG
        FImage gs_image = grayscale(image); // преобразуем в оттенки серого
        
        gs_image = gs_image.extra_borders(1, 1); // дополняем границы

        // локальные бинарные шаблоны
        vector<float> locbinpat = local_binary_patterns(gs_image);
        // конец локальных бинарных шаблонов

        ftime.start();
        FImage image_vx = sobel_x(gs_image); // горизонтальная составляющая вектора градиента
        sob_x.push_back(ftime.elapsed_time());

        ftime.restart();
        image_vx = sobel_x_sse(gs_image); // горизонтальная составляющая вектора градиента c SSE
        sob_x_sse.push_back(ftime.elapsed_time());

        ftime.restart();
        FImage image_vy = sobel_y(gs_image); // вертикальная составляющая вектора градиента
        sob_y.push_back(ftime.elapsed_time());

        ftime.restart();
        image_vy = sobel_y_sse(gs_image); // горизонтальная составляющая вектора градиента с SSE
        sob_y_sse.push_back(ftime.elapsed_time());

        ftime.restart();
        FImage v_abs = grad_abs(image_vx, image_vy); // модуль вектора градиента
        vabs.push_back(ftime.elapsed_time());

        ftime.restart();
        v_abs = grad_abs_sse(image_vx, image_vy); // модуль вектора градиента с SSE
        vabs_sse.push_back(ftime.elapsed_time());

        ftime.stop(); // останавливаем таймер после каждой картинки

        FImage v_dest = grad_dest(image_vx, image_vy); // направление вектора градиента
	
        // ненормированное заполнение гистограмм
        FImage norms(CELLS, CELLS); // матрица из норм клеток
        HistoMatrix histo(CELLS, CELLS); // гистограммы каждой клетки

        for (uint i = 0; i < CELLS; i++) {
            for (uint j = 0; j < CELLS; j++) {
                norms(i, j) = 0; // обнуляем матрицу норм

                for (uint k = 0; k < SEGMENTS; k++) { // обнуляем каждую гистограмму
                    histo(i, j).push_back(0);
                }
            }
        }

        uint cell_h = v_abs.n_rows / CELLS, cell_w = v_abs.n_cols / CELLS; // ширина и высота одной клетки в пикселях
        double ang_seg = 2 * M_PI / SEGMENTS; // доля угла в сегменте
        for (uint i = 0; i < v_abs.n_rows; i++) {
            for (uint j = 0; j < v_abs.n_cols; j++) {
                // нужно определить в какую именно клетку по вертикали и горизонтали попадает пиксель
                // а потом найти для него место в гистограмме
                uint place_i = i / cell_h, place_j = j / cell_w; // из какой пиксель клетки
                if (place_i >= CELLS) { place_i = CELLS - 1; } // боковые пиксели относятся к последней клетке
                if (place_j >= CELLS) { place_j = CELLS - 1; }

                int place_ang = (v_dest(i, j) + M_PI) / ang_seg; // сегмент, в который попадает угол
                if (place_ang >= SEGMENTS) { place_ang = SEGMENTS - 1; } // на случай 2 * M_PI

                histo(place_i, place_j)[place_ang] += v_abs(i, j);
                norms(place_i, place_j) += v_abs(i, j) * v_abs(i, j); // копим норму
            }
        }

        for (uint i = 0; i < CELLS; i++) {
            for (uint j = 0; j < CELLS; j++) {
                norms(i, j) = sqrt(norms(i, j)); // евклидова норма клетки

                for (uint k = 0; k < SEGMENTS; k++) { // и сразу нормируем гистограммы
                    if (norms(i, j) > 0) {
                        histo(i, j)[k] /= norms(i, j);
                    }
                }
            }
        }

        // формируем дескриптор
        vector<float> desc;
        for (uint i = 0; i < CELLS; i++) {
            for (uint j = 0; j < CELLS; j++) {
                desc.insert(desc.end(), histo(i, j).begin(), histo(i, j).end());
            }
        }
        desc.insert(desc.end(), color.begin(), color.end()); // цветовые признаки
        desc.insert(desc.end(), locbinpat.begin(), locbinpat.end()); // локальные бинарные шаблоны

        (*features).push_back(make_pair(desc, data_set[image_idx].second));

        // End of sample code
    }

    // считаем среднее арфметическое время на всём датасете
    double tsob_x = 0, 
           tsob_x_sse = 0, 
           tsob_y = 0, 
           tsob_y_sse = 0, 
           tvabs = 0, 
           tvabs_sse = 0;
    int count_images = sob_x.size();
    for (int i = 0; i < count_images; i++) {
        tsob_x += sob_x[i];
        tsob_x_sse += sob_x_sse[i];
        tsob_y += sob_y[i];
        tsob_y_sse += sob_y_sse[i];
        tvabs += vabs[i];
        tvabs_sse += vabs_sse[i];        
    }

    // выводим все время
    std::cout << "Time of Sobel_x: " << std::setiosflags(std::ios::fixed) << std::setprecision(3) << tsob_x << " seconds" << std::endl;
    std::cout << "Time of Sobel_x_sse: " << std::setiosflags(std::ios::fixed) << std::setprecision(3) << tsob_x_sse << " seconds" << std::endl;
    std::cout << "Time of Sobel_y: " << std::setiosflags(std::ios::fixed) << std::setprecision(3) << tsob_y << " seconds" << std::endl;
    std::cout << "Time of Sobel_y_sse: " << std::setiosflags(std::ios::fixed) << std::setprecision(3) << tsob_y_sse << " seconds" << std::endl;
    std::cout << "Time of Absolute: " << std::setiosflags(std::ios::fixed) << std::setprecision(3) << tvabs << " seconds" << std::endl;
    std::cout << "Time of Absolute_sse: " << std::setiosflags(std::ios::fixed) << std::setprecision(3) << tvabs_sse << " seconds" << std::endl;
}

// Clear dataset structure
/*!
Правильно освобождает память структуры, хранящей загруженные изображения
\param Загруженные изображения
*/
void ClearDataset(TDataSet* data_set) {
        // Delete all images from dataset
    for (size_t image_idx = 0; image_idx < data_set->size(); ++image_idx)
        delete (*data_set)[image_idx].first;
        // Clear dataset
    data_set->clear();
}

// Train SVM classifier using data from 'data_file' and save trained model
// to 'model_file'
/*!
Обучает классификатор, используя уже классифицированные изображения (т.е. те, что с "ярлыками") и сохраняет результат в файл
\param data_file Путь до изображений с "ярлыками"
\param model_file Файл для сохранения результатов
*/
void TrainClassifier(const string& data_file, const string& model_file) {
        // List of image file names and its labels
    TFileList file_list;
        // Structure of images and its labels
    TDataSet data_set;
        // Structure of features of images and its labels
    TFeatures features;
        // Model which would be trained
    TModel model;
        // Parameters of classifier
    TClassifierParams params;
    
        // Load list of image file names and its labels
    LoadFileList(data_file, &file_list);
        // Load images
    LoadImages(file_list, &data_set);
        // Extract features from images
    ExtractFeatures(data_set, &features);

        // PLACE YOUR CODE HERE
        // You can change parameters of classifier here
    params.C = 0.01;
    TClassifier classifier(params);
        // Train classifier
    classifier.Train(features, &model);
        // Save model to file
    model.Save(model_file);
        // Clear dataset structure
    ClearDataset(&data_set);
}

// Predict data from 'data_file' using model from 'model_file' and
// save predictions to 'prediction_file'
/*!
Классифицирует изображения, используя обученную модель, и сохраняет результаты в файл
\param data_file Путь до требуемых изображений
\param model_file Файл с обученной моделью
\param prediction_file Файл для сохранения результатов
*/
void PredictData(const string& data_file,
                 const string& model_file,
                 const string& prediction_file) {
        // List of image file names and its labels
    TFileList file_list;
        // Structure of images and its labels
    TDataSet data_set;
        // Structure of features of images and its labels
    TFeatures features;
        // List of image labels
    TLabels labels;

        // Load list of image file names and its labels
    LoadFileList(data_file, &file_list);
        // Load images
    LoadImages(file_list, &data_set);
        // Extract features from images
    ExtractFeatures(data_set, &features);

        // Classifier 
    TClassifier classifier = TClassifier(TClassifierParams());
        // Trained model
    TModel model;
        // Load model from file
    model.Load(model_file);
        // Predict images by its features using 'model' and store predictions
        // to 'labels'
    classifier.Predict(features, model, &labels);

        // Save predictions
    SavePredictions(file_list, labels, prediction_file);
        // Clear dataset structure
    ClearDataset(&data_set);
}

int main(int argc, char** argv) {
    // List of image file names and its labels
    TFileList file_list;
    // Structure of images and its labels
    TDataSet data_set;
    // Structure of features of images and its labels
    TFeatures features;
    
    string data_file;
    if (argc == 2) {
    	data_file = argv[1];
    } else {
        std::cout << "Error count of arguments" << std::endl;
        exit(1);
    }

    // Load list of image file names and its labels
    LoadFileList(data_file, &file_list);
    // Load images
    LoadImages(file_list, &data_set);
    // Extract features from images
    ExtractFeatures(data_set, &features);

    return 0;
}
