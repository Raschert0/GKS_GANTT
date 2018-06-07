#include "mainwindow.h"
#include <QApplication>
#include <QFileDialog>
#include <QStandardPaths>
#include "datastorage.h"
#include "suprule.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    DataStorage gdata;
    DataStorage gnoatmdata;
    gnoatmdata.disable_atms();


    QString fname = QFileDialog::getOpenFileName(nullptr,
                                                 QObject::tr("Open file with data"),
                                                 QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                 "*.txt");
    if(fname.isEmpty() || !gdata.load(fname) || !gnoatmdata.load(fname)){
        exit(1);
    }
    gdata.firstRun();
    gdata.calculateThemAll();
    gdata.reset();

    gdata.setNewRule(new SecondRule());
    gdata.firstRun();
    gdata.calculateThemAll();
    gdata.reset();

    gdata.setNewRule(new ThirdReich());
    gdata.firstRun();
    gdata.calculateThemAll();
    gdata.reset();

    gdata.setNewRule(new FourthRule());
    gdata.firstRun();
    gdata.calculateThemAll();
    gdata.reset();

    gdata.setNewRule(new FifthRule());
    gdata.firstRun();
    gdata.calculateThemAll();
    gdata.reset();

    gdata.setNewRule(new SixthRule());
    gdata.firstRun();
    gdata.calculateThemAll();
    gdata.reset();

    gdata.setNewRule(new SeventhRule());
    gdata.firstRun();
    gdata.calculateThemAll();
    gdata.reset();

    MainWidget w(&gdata);
    w.show();

    return a.exec();
}
