#pragma once

#include <QString>

class Observable;

class Observer // абстрактный класс для уведомлений
{
public:
    virtual void have_image_for_present(const Observable &) = 0; // есть новое изображение для представления
    virtual void new_state(const Observable &) = 0; // новое состояние модели для отображения
};

class Observable // класс для добавления, удаления и оповещения наблюдателей
{
private:

    QString note; // сообщения
    int type; // тип сообщения: 0 - have_image_for_present, 1 - new_state
    Observer * present_observer; // представление-наблюдатель

    void notify() // рассылка сообщений
    {
        switch (type) {
        case 0:
            present_observer->have_image_for_present(*this);
            break;
        case 1:
            present_observer->new_state(*this);
            break;
        }
    }

public:

    void add_present(Observer & ref)
    {
       present_observer = &ref;
    }
    /*void remove(Observer & ref) // не очень нужен
    {
        observers.remove(&ref);
    }*/
    const QString & get() const
    {
        return note;
    }
    void reset(QString new_note, int new_type)
    {
        note = new_note;
        type = new_type;
        notify();
    }

};
