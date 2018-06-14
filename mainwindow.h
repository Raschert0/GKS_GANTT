#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QTabWidget>
#include <QTextEdit>
#include "datastorage.h"
#include "casechart.h"

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(DataStorage *data, DataStorage *natmdata, QWidget *parent = 0);
    ~MainWidget();

private:
    void updateTextEdit();
    void createPetri(QVector<QString> &tab_names);

    QTabWidget *tabs;
    QTabWidget *noatms_tabs;
    QTabWidget *master_tabs;

    QTextEdit *textedit;

    DataStorage *d, *wad;

    int c_master{1}, c_slave{0};

public slots:
    void masterTabCurrentChanged(int index);
    void slaveTabCurrentChanged(int index);

};

#endif // MAINWINDOW_H
