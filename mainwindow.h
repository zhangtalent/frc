#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class FrcApp;
}

class FrcApp : public QMainWindow
{
    Q_OBJECT

public:
    explicit FrcApp(QWidget *parent = nullptr);
    ~FrcApp();

private:
    Ui::FrcApp *ui;
};

#endif // MAINWINDOW_H
