// функции для плагинов

#include <string>
#include <vector>
#include <iostream>
#include <dirent.h>
#include <QLibrary>

using namespace std;

#include "../include/api.h"
#include "../../align_project/include/align.h"

class PluginManager: public IPluginManager
{
    std::vector <IUnaryPlugin*> unPlugins; // коллекционируем плагины с однм параметром
public:
    // функция-регистратор плагинов
    virtual void RegisterUnaryPlugin(IUnaryFactory* factory)
    {
        IUnaryPlugin* plugin = factory->Create();
        if (plugin)
            unPlugins.push_back(plugin);
    }

    // отдает накопленный список плагинов
    std::vector<IUnaryPlugin*> &GetUnaryPlugins()
    {
        return unPlugins;
    }
};

extern class Model main_model;

extern class PluginManager manager; // объявляем менеджер плагинов - в main

int findsos(const char *in_pluginsDir, std::vector<string> &out_file_names) // ищет плагины как so-файлы
{
    // in_pluginsDir - директория с плагинами
    // out_file_names - вектор найденных файлов
    string buffer; // полный путь до файла

    // просматриваем всю директорию
    DIR *dir = opendir(in_pluginsDir);
    struct dirent *dd;
    while (dir && (dd = readdir(dir))) { // директория открыта и просматривается
        string next_file = dd->d_name; // string удобнее сравнивать

        if ((next_file != ".") && (next_file != "..") &&
            (next_file.length() >= 3) &&
            (next_file.substr(next_file.length() - 3) == ".so")) {

            buffer = string(in_pluginsDir) + string("/") + next_file; // полный путь до файла
            out_file_names.push_back(buffer);
        }
    }

    closedir(dir);

    return out_file_names.size(); // сколько файлов в директории удовлетворяют условию
}

int loadsos(const vector<string> &in_file_names) // загружаем плагины - получаем указатели на точки входа
{
    // in_pluginsDir - директория с плагинами
    // in_file_names - вектор с именами файлов

    auto files_num = in_file_names.size();
    int k = 0; // количество открывшихся библитек, которые отдали свои указатели на функции
    for (uint i = 0; i < files_num; i++) {
        // QLibrary заботится о том, чтобы загруженная библиотека оставалась в памяти на протяжении всего времени использования
        QLibrary new_lib(in_file_names[i].c_str());
        if (new_lib.load()) { // если успешно загружена
            regFuncType new_func = (regFuncType)new_lib.resolve("registerPlugins"); // адрес найденной функции; символьная сигнатура, по которой будет производиться поиск нужной функции
            if (new_func) { // функция найдена?
                new_func(&manager); // эта штука идёт и регистрирует полученный плагин
                k++;
            }
        } // иначе эта библиотека не загрузилась
    }

    return k;
}

void UserUnaryOperation(vector<IUnaryPlugin*> &plugList)
{
    if (plugList.size() > 0) {
        cout << "Выберите функцию постобработки:" << endl;

        for (size_t i = 0; i < plugList.size(); i++)
            cout << '[' << i << ']' <<  (plugList[i])->stringType() << endl;

        int pluginIndex = -1;
        while ((pluginIndex >= int(plugList.size())) || pluginIndex < 0) {
             cin >> pluginIndex;
             if ((pluginIndex >= int(plugList.size())) || pluginIndex < 0) {
                 cerr << "Invalid input: out of range " << "0-" << plugList.size() << endl;
           }
        }

        ((IUnaryPlugin*)plugList[pluginIndex])->operation(main_model.get_image()); // обращается к плагинам c параметром
    }
}
