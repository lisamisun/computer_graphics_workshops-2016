#include <list>
#include <QString>

#include "align_project/include/observer.h"

class Observable;

class Observer // абстрактный класс для уведомлений
{
public:
    virtual void have_image_for_model(const Observable &) = 0; // есть новое изображение для модели, и для представления
    virtual void have_image_for_present(const Observable &) = 0; // есть новое изображение для представления
    virtual void save_last_image(const Observable &) = 0; // запрос представления на сохранение изображения
    virtual void new_state(const Observable &) = 0; // новое состояние модели для отображения
    virtual void wanna_action(const Observable &) = 0; // запрос действий от модели
};

class Observable // класс для добавления, удаления и оповещения наблюдателей
{
private:

    QString note; // сообщения
    int type; // тип сообщения: 0 - have_image_for_model, 1 - have_image_for_present, 2 - save_image, 3 - new_state
    Observer * model_observer; // модель-наблюдатель
    Observer * present_observer; // представление-наблюдатель

    void notify() // рассылка сообщений
    {
        switch (type) {
        case 0:
            model_observer->have_image_for_model(*this);
            break;
        case 1:
            present_observer->have_image_for_present(*this);
            break;
        case 2:
            model_observer->save_last_image(*this);
            break;
        case 3:
            present_observer->new_state(*this);
            break;
        case 4:
            model_observer->wanna_action(*this);
        }
    }

public:

    void add_model(Observer & ref)
    {
       model_observer = &ref;
    }
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
