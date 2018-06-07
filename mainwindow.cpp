#include "mainwindow.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSimpleTextItem>
#include <QGridLayout>
#include <QTabWidget>
#include <QPlainTextEdit>

#include <QDebug>
#include <QImage>
#include <QPainter>

inline QVector<QColor> rndColors(int count){
    QVector<QColor> colors;
    float currentHue = 0.0;
    double currentSat = 1.0;
    for (int i = 0; i < count; i++){
        //0.618033988749895f
        colors.push_back( QColor::fromHslF(currentHue, currentSat, 0.5) );
        currentHue += 0.618033988749895f;
        currentHue = std::fmod(currentHue, 1.0f);
        if(i % 2){
            currentSat = 1.0;
        }else{
            currentSat = 0.6;
        }
    }
    return colors;
}

inline QVector<QBrush> generateBrushes(int count){
    QVector<QColor> colors = rndColors(count);
    QVector<QBrush> ret;
    for(int i{0}; i < count / 2; ++i){
        ret.push_back(QBrush(colors[i], Qt::SolidPattern));
    }
    for(int i{0}; i < count / 2; ++i){
        ret.push_back(QBrush(colors[i], Qt::DiagCrossPattern));
    }
    return ret;
}

MainWidget::MainWidget(DataStorage *data, QWidget *parent)
    : QWidget(parent)
{
    QVector<QBrush> brushes = generateBrushes(data->items.size());
    QVector<QString> tab_names{"Правило найкоротшої операції",
                              "Правило максимальної залишкової трудомісткості",
                              "Правило вирівнювання завантаження верстатів",
                              "Правило мінімальної залишкової трудомісткості",
                              "Правило найдовшої операції",
                              "FIFO",
                              "LIFO"};

    QTabWidget *tabs = new QTabWidget();

    const int h_lines_step{10};
    const int h_lines_width{15};

    for(int z{0}; z < data->stored_atms.size(); ++z){
        QGraphicsScene *scene = new QGraphicsScene();

        int max_y{(data->stored_atms[z].size() + data->stored_fpms[z].size()) * (h_lines_step + h_lines_width) + h_lines_width * 2};
        if(data->ts_count == 2){
            max_y += h_lines_step + 2 * h_lines_width;
        }
        double max_x{data->stored_time[z]};
        const int v_lines_step{50};
        for(int i{0}; i <= max_x/v_lines_step; ++i){
            scene->addLine(i * v_lines_step, max_y, i * v_lines_step, -15, QPen(QColor(Qt::gray)));
            QGraphicsSimpleTextItem *simple_text = new QGraphicsSimpleTextItem(QString::number(i * v_lines_step));
            scene->addItem(simple_text);
            simple_text->setPos(i * v_lines_step, max_y);
        }
        scene->addLine(-50, max_y, max_x, max_y, QPen(QColor(Qt::gray)));
        {
            QGraphicsSimpleTextItem *simple_text = new QGraphicsSimpleTextItem("Час закінчення обробки: " + QString::number(data->stored_time[z]) + " хв");
            scene->addItem(simple_text);
            simple_text->setPos(-50, max_y + 15);
        }

        int end_y_atms{data->stored_atms[z].size() * (h_lines_step + h_lines_width) + h_lines_width + 10};
        for(int i{0}; i < data->stored_atms[z].size(); ++i){
            ATM* c_atm = data->stored_atms[z][i].data();
            QVector<LogEntry> &log = c_atm->getLog();
            for(int j{0}; j < log.size(); ++j){
                scene->addRect(log[j].start(),
                               i * (h_lines_step + h_lines_width),
                               log[j].duration(),
                               h_lines_width,
                               QPen(),
                               brushes[log[j].itemId()]);
                if(log[j].cross_move){
                    if(log[j].cross_move == 1){
                        scene->addLine(log[j].end(), i * (h_lines_step + h_lines_width), log[j].end(), end_y_atms);
                    }else{
                        scene->addLine(log[j].start(), i * (h_lines_step + h_lines_width), log[j].start(), end_y_atms);
                    }

                }
                /*QGraphicsSimpleTextItem *num = new QGraphicsSimpleTextItem(" " + QString::number(log[j].itemId() + 1));
                scene->addItem(num);
                num->setPos(log[j].start(), i * (h_lines_step + h_lines_width));
                */
            }
            scene->addLine(-50,
                           i * (h_lines_step + h_lines_width) + h_lines_width,
                           log.last().end(),
                           i * (h_lines_step + h_lines_width) + h_lines_width);
            QGraphicsSimpleTextItem *simple_text = new QGraphicsSimpleTextItem("АТМ №" + QString::number(i+1));
            scene->addItem(simple_text);
            simple_text->setPos(-50, i * (h_lines_step + h_lines_width));
        }


        if(data->ts_count == 2){
            QVector<LogEntry> &log = data->stored_reloaders[z].data()->getLog();
            for(int i{0}; i < log.size(); ++i){
                scene->addRect(log[i].start(),
                               end_y_atms - 5,
                               1,
                               h_lines_width,
                               QPen(),
                               brushes[log[i].itemId()]);
            }
            end_y_atms += h_lines_step + h_lines_width * 2;
        }

        for(int i{0}; i < data->stored_fpms[z].size(); ++i){
            FPM* c_fpm = data->stored_fpms[z][i].data();
            QVector<LogEntry> &log = c_fpm->getLog();
            for(int j{0}; j < log.size(); ++j){
                scene->addRect(log[j].start(),
                               end_y_atms + i * (h_lines_step + h_lines_width),
                               log[j].duration(),
                               h_lines_width,
                               QPen(),
                               brushes[log[j].itemId()]);
            }
            scene->addLine(-50,
                           end_y_atms + i * (h_lines_step + h_lines_width) + h_lines_width,
                           log.last().end(),
                           end_y_atms + i * (h_lines_step + h_lines_width) + h_lines_width);
            QGraphicsSimpleTextItem *simple_text = new QGraphicsSimpleTextItem("ГВМ №" + QString::number(i+1));
            scene->addItem(simple_text);
            simple_text->setPos(-50, end_y_atms + i * (h_lines_step + h_lines_width));
        }

        QGraphicsView *view = new QGraphicsView(scene);
        tabs->addTab(view, tab_names[z]);
        QString filename = tab_names[z]+".png";
        QImage img(scene->sceneRect().width(), scene->sceneRect().height(), QImage::Format_RGB32);
        img.fill(Qt::white);
        QPainter painter(&img);
        painter.setRenderHint(QPainter::Antialiasing);
        scene->render(&painter);
        img.save(filename);
    }

    QGraphicsScene *bottom_scene = new QGraphicsScene();


    for(int i{0}; i < data->original_items.size() / 2; ++i){ //Сподіваюся, воно завжди буде парним
        bottom_scene->addRect(0, i * (h_lines_width + h_lines_step), 15, 15, QPen(), brushes[i]);
        QGraphicsSimpleTextItem *simple_text = new QGraphicsSimpleTextItem("Деталь №" + QString::number(i+1));
        bottom_scene->addItem(simple_text);
        simple_text->setPos(25, i * (h_lines_step + h_lines_width));

        bottom_scene->addRect(170, i * (h_lines_width + h_lines_step), 15, 15, QPen(), brushes[i+7]);
        simple_text = new QGraphicsSimpleTextItem("Деталь №" + QString::number(i+8));
        bottom_scene->addItem(simple_text);
        simple_text->setPos(195, i * (h_lines_step + h_lines_width));

    }

    QGraphicsView *bview = new QGraphicsView(bottom_scene);
    bottom_scene->addLine(-15, -10, -10, -10, QPen(QColor(Qt::white)));
    bottom_scene->addLine(bview->sceneRect().right() + 10, bview->sceneRect().bottom() + 10, bview->sceneRect().right() + 15, bview->sceneRect().bottom() + 10, QPen(QColor(Qt::white)));

    QString filename = "Деталі.png";
    QImage img(bottom_scene->sceneRect().width(), bottom_scene->sceneRect().height(), QImage::Format_RGB32);
    img.fill(Qt::white);
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);
    bottom_scene->render(&painter);
    painter.end();
    img.save(filename);

    QPlainTextEdit *textedit = new QPlainTextEdit();

    QGridLayout *layout = new QGridLayout();
    layout->addWidget(tabs, 0, 0, 1, 2);
    layout->addWidget(bview, 1, 1);
    layout->addWidget(textedit, 1, 0);

    layout->setRowStretch(0, 5);
    layout->setRowStretch(1, 2);
    layout->setColumnStretch(0, 3);
    setLayout(layout);
    qDebug() << bview->sceneRect().x() << bview->sceneRect().y() << bview->sceneRect().width() << bview->sceneRect().height();

}

MainWidget::~MainWidget()
{

}
