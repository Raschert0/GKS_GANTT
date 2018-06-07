#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include "datastorage.h"

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(DataStorage *data, QWidget *parent = 0);
    ~MainWidget();
};

#endif // MAINWINDOW_H
