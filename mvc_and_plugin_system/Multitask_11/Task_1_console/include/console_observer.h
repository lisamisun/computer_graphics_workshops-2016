#pragma once

#ifndef CONSOLE_OBSERVER
#define CONSOLE_OBSERVER

#include "../../Task_1/align_project/include/observer.h"

class ConsoleObserver: public Observer // эта штука получает уведомления от модели
{
public:
    virtual void have_image_for_present(const class Observable &);
    virtual void new_state(const class Observable & ref);
};

#endif // CONSOLE_OBSERVER

