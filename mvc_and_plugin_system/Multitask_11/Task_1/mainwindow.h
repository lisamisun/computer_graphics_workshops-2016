#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "align_project/include/observer.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    Ui::MainWindow *ui;

private slots:
    void on_action_2_triggered();

    void on_action_5_triggered();

    void on_action_6_triggered();

    void on_action_11_triggered();
};

class View: public Observer // эта штука получает уведомления от модели
{
    class MainWindow *main_win;
public:
    View(class MainWindow *new_win)
    {
        main_win = new_win;
    }
    virtual void have_image_for_present(const class Observable & ref);
    virtual void new_state(const class Observable & ref);
};

#endif // MAINWINDOW_H
