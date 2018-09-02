#include "../include/lennatest.h"
#include "../externals/googletest/googletest/include/gtest/gtest.h"

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

TEST(LennaTestSSE, SSE) {

    BMP * image = new BMP(); // изображение, которое мучаем

    image->ReadFromFile("../../data/Lenna/Lenna.bmp");

    FImage gs_image = grayscale(*image); // преобразуем в оттенки серого
        
    gs_image = gs_image.extra_borders(1, 1); // дополняем границы

    FImage image_vx = sobel_x(gs_image); // горизонтальная составляющая вектора градиента
    FImage image_vx_sse = sobel_x_sse(gs_image); // горизонтальная составляющая вектора градиента c SSE

    FImage image_vy = sobel_y(gs_image); // вертикальная составляющая вектора градиента
    FImage image_vy_sse = sobel_y_sse(gs_image); // горизонтальная составляющая вектора градиента с SSE

    FImage v_abs = grad_abs(image_vx, image_vy); // модуль вектора градиента
    FImage v_abs_sse = grad_abs_sse(image_vx, image_vy); // модуль вектора градиента с SSE

    ASSERT_EQ(v_abs.n_rows, v_abs_sse.n_rows);
    ASSERT_EQ(v_abs.n_cols, v_abs_sse.n_cols);
  
    for (uint i = 0; i < v_abs.n_rows; i++) {
        for (uint j = 0; j < v_abs.n_cols; j++) {
            EXPECT_NEAR(v_abs(i, j), v_abs_sse(i, j), 0.0001);
        }
    }

    delete image;
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
