#ifndef GRAY_WORLD_H
#define GRAY_WORLD_H

#include <vector>
#include "gray_world_global.h"
#include "../../Task_1/API/include/api.h"
#include "../../Task_1/align_project/include/matrix.h"
#include "../../Task_1/align_project/include/io.h"

// конкретный плагин для gray_world
class GRAY_WORLDSHARED_EXPORT Gray_world: public IUnaryPlugin
{
public:
    virtual void operation(Image &src_image);
    virtual const char* stringType();
    Gray_world();
};

// factory для серого мира
class Gray_WorldFactory: public IUnaryFactory
{
    std::vector<void*> pInstances; // указатели на конкретно то, что представляет собой плагин
public:
    virtual IUnaryPlugin* Create()
    {
        IUnaryPlugin* ret = new Gray_world;
        pInstances.push_back((void*)ret);
        return ret;
    }

    ~Gray_WorldFactory()
    {
        for (int i = 0; i < int(pInstances.size()); i++) {
            delete pInstances[i];
        }
    }
};

// изготовление плагинов
Gray_WorldFactory FactoryGW;

// регистрация плагинов с .so
extern "C" Q_DECL_EXPORT
    void registerPlugins(IPluginManager * pluginManager)
{
    if (pluginManager != 0)
    {
        pluginManager->RegisterUnaryPlugin(&FactoryGW);
    }
}

#endif // GRAY_WORLD_H
