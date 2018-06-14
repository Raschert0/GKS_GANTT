#include "mainwindow.h"
#include <QApplication>
#include <QFileDialog>
#include <QProgressDialog>
#include <QStandardPaths>
#include <QInputDialog>
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

    bool itsok;
    double pre = QInputDialog::getDouble(nullptr, QString("User input"), QString("Уведіть крок дискрети часу"), 0.01, 0.001, 0.5, 3, &itsok);
    if(itsok){
        gdata.discrete = pre;
        gnoatmdata.discrete = pre;
    }

    QProgressDialog progress("Calculating...", QString(), 0, 15);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);

    gnoatmdata.firstRun();
    gnoatmdata.calculateThemAll();
    gnoatmdata.reset();
    progress.setValue(5);

    gnoatmdata.setNewRule(new SecondRule());
    gnoatmdata.firstRun();
    gnoatmdata.calculateThemAll();
    gnoatmdata.reset();
    progress.setValue(6);

    gnoatmdata.setNewRule(new ThirdReich());
    gnoatmdata.firstRun();
    gnoatmdata.calculateThemAll();
    gnoatmdata.reset();
    progress.setValue(7);

    gnoatmdata.setNewRule(new FourthRule());
    gnoatmdata.firstRun();
    gnoatmdata.calculateThemAll();
    gnoatmdata.reset();
    progress.setValue(8);

    gnoatmdata.setNewRule(new FifthRule());
    gnoatmdata.firstRun();
    gnoatmdata.calculateThemAll();
    gnoatmdata.reset();
    progress.setValue(9);

    gdata.firstRun();
    gdata.calculateThemAll();
    gdata.reset();
    progress.setValue(10);

    gdata.setNewRule(new SecondRule());
    gdata.firstRun();
    gdata.calculateThemAll();
    gdata.reset();
    progress.setValue(11);

    gdata.setNewRule(new ThirdReich());
    gdata.firstRun();
    gdata.calculateThemAll();
    gdata.reset();
    progress.setValue(12);

    gdata.setNewRule(new FourthRule());
    gdata.firstRun();
    gdata.calculateThemAll();
    gdata.reset();
    progress.setValue(13);

    gdata.setNewRule(new FifthRule());
    gdata.firstRun();
    gdata.calculateThemAll();
    gdata.reset();
    progress.setLabelText("Creating fancy diagram...");
    progress.setValue(14);


    MainWidget w(&gdata, &gnoatmdata);
    progress.setValue(15);
    w.show();

    return a.exec();
}
