#ifndef UNSHARP_H
#define UNSHARP_H

#include <vector>
#include "unsharp_global.h"
#include "../../Task_1/API/include/api.h"
#include "../../Task_1/align_project/include/matrix.h"
#include "../../Task_1/align_project/include/io.h"

class UNSHARPSHARED_EXPORT Unsharp: public IUnaryPlugin
{
public:
    virtual void operation(Image &src_image);
    virtual const char* stringType();
    Unsharp();
};

// factory для серого мира
class UnsharpFactory: public IUnaryFactory
{
    std::vector<void*> pInstances; // указатели на конкретно то, что представляет собой плагин
public:
    virtual IUnaryPlugin* Create()
    {
        IUnaryPlugin* ret = new Unsharp;
        pInstances.push_back((void*)ret);
        return ret;
    }

    ~UnsharpFactory()
    {
        for (int i = 0; i < int(pInstances.size()); i++) {
            delete pInstances[i];
        }
    }
};

// изготовление плагинов
UnsharpFactory FactoryU;

// регистрация плагинов с .so
extern "C" Q_DECL_EXPORT
    void registerPlugins(IPluginManager * pluginManager)
{
    if (pluginManager != 0)
    {
        pluginManager->RegisterUnaryPlugin(&FactoryU);
    }
}

#endif // UNSHARP_H
