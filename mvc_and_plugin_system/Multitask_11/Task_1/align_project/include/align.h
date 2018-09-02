#pragma once

#include "io.h"
#include "matrix.h"
#include "observer.h"
#include "../../API/include/api.h"

class Model // вся начинка
{
    Image srcImage; // тут всегда хранится последняя актуальная версия
public:
    Observable model_obs; // наблюдатели за моделью

    void load(QString & path);
    void save(QString & path);
    void align();
    Image & get_image();
};
