#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <initializer_list>
#include <limits>

using std::string;
using std::stringstream;
using std::cout;
using std::cerr;
using std::endl;
using std::numeric_limits;

#include "../include/align.h"

#include "mainwindow.h"
#include <QApplication>

class Model main_model; // подключаем модель

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    class View present(&w);
    main_model.model_obs.add_present(present); // подпишись на наши обновления!

    return a.exec();
}
