// это контроллер, насколько я понимаю всю эту систему

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>

#include "align_project/include/align.h"

extern class Model main_model;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    remove("tmp.bmp");
    delete ui;
}

void MainWindow::on_action_2_triggered()
{
    QString open_file = QFileDialog::getOpenFileName(0, "Выбрать изображение...", "", "*.bmp");
    main_model.load(open_file);
}

void MainWindow::on_action_5_triggered()
{
    QString save_file = QFileDialog::getSaveFileName(0, "Сохранить изображение...", "", "*.bmp");
    main_model.save(save_file);
}

void MainWindow::on_action_6_triggered()
{
    remove("tmp.bmp");
    exit(0); // завершение с кодом 0
}

void MainWindow::on_action_11_triggered() // совместить каналы
{
    main_model.align();
}

void View::have_image_for_present(const Observable & ref)
{
    // новое изображение! -> перерисовываем
    main_win->ui->label->setPixmap(QPixmap(ref.get()));
}

void View::new_state(const Observable & ref)
{
    // новое состояние! -> переписываем

    main_win->ui->textBrowser->insertPlainText(ref.get());
    main_win->ui->textBrowser->insertPlainText("\n");
}
