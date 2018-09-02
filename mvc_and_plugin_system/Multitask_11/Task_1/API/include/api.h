#pragma once

#ifndef API
#define API

#include "../../align_project/include/io.h"

class IUnaryPlugin // базовый класс для плагинов с одним параметром
{
public:
    virtual const char* stringType() = 0; // имя плагина
    virtual void operation(Image &) = 0; // производит операцию и возвращает результат в первый параметр
};

template<typename Interface> class IFactory // абстрактный класс для factory
{
public:
    virtual Interface* Create() = 0;
};

typedef IFactory<IUnaryPlugin> IUnaryFactory; // factory для плагинов с одним параметром

class IPluginManager // абстрактный класс для менеджера плагинов с одним параметром
{
public:
    virtual void RegisterUnaryPlugin(IUnaryFactory* factory) = 0; // функция-регистратор плагинов
};

typedef void (*regFuncType)(IPluginManager *); // указатель на функцию, которая возвращает void и принимает IPluginManager *
                                               // формально эта штука задает точку входа в функцию

#endif

