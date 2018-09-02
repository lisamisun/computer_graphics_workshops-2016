#include "../include/console_observer.h"
#include <iostream>

void ConsoleObserver::have_image_for_present(const Observable &)
{
    // новое изображение! -> перерисовываем
    // для консоли оно бесполезно
}

void ConsoleObserver::new_state(const Observable & ref)
{
    // новое состояние! -> переписываем
    std::cout << ref.get().toStdString() << std::endl;
}
