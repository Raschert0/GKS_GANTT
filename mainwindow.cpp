#include "mainwindow.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSimpleTextItem>
#include <QGridLayout>
#include <QTabWidget>
#include <QTextEdit>

#include <QDebug>
#include <QImage>
#include <QPainter>

#include "xlsxdocument.h"

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

MainWidget::MainWidget(DataStorage *data, DataStorage *natmdata, QWidget *parent)
    : QWidget(parent), d{data}, wad{natmdata}
{
    QVector<QBrush> brushes = generateBrushes(data->items.size());
    QVector<QString> tab_names{"Правило найкоротшої операції",
                              "Правило максимальної залишкової трудомісткості",
                              "Правило вирівнювання завантаження верстатів",
                              "Правило мінімальної залишкової трудомісткості",
                              "Правило найдовшої операції"
                              };

    tabs = new QTabWidget();

    const int h_lines_step{10};
    const int h_lines_width{15};

    for(int z{0}; z < data->stored_atms.size(); ++z){
        qDebug() << tab_names[z];

        QGraphicsScene *scene = new QGraphicsScene();
        QXlsx::Document trans_xlsx;
        trans_xlsx.setColumnWidth(1, 1, 15.0);
        trans_xlsx.setColumnWidth(2, 2, 45.0);
        int tlsxsid{1};

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
            qDebug() << "ATM" + QString::number(c_atm->Id() + 1);;
            QVector<LogEntry> &log = c_atm->getLog();
            QString log_str;
            for(int j{0}; j < log.size(); ++j){
                log_str += QString::number(log[j].itemId() + 1) + " ";
                scene->addRect(log[j].start(),
                               i * (h_lines_step + h_lines_width),
                               log[j].duration(),
                               h_lines_width,
                               QPen(),
                               brushes[log[j].itemId()]);
                QString tsxlsentry;
                tsxlsentry += "АТМ" + QString::number(i + 1) + " транспортує Д" + QString::number(log[j].itemId() + 1) + " з ";
                if(log[j].cross_move != 2){
                    if(log[j].from() > data->asCount() - 1){
                        tsxlsentry += "ГВМ" + QString::number(log[j].from() - data->asCount() + 1);
                    }else{
                        tsxlsentry += "АС" + QString::number(log[j].from() + 1);
                    }
                }else{
                    tsxlsentry += "ПП";
                }
                tsxlsentry += " до ";

                if(log[j].cross_move){
                    if(log[j].cross_move == 1){
                        scene->addLine(log[j].end(), i * (h_lines_step + h_lines_width), log[j].end(), end_y_atms);
                        tsxlsentry += "ПП";
                    }else{
                        scene->addLine(log[j].start(), i * (h_lines_step + h_lines_width), log[j].start(), end_y_atms);
                        if(log[j].dest() > data->asCount() - 1){
                            tsxlsentry += "ГВМ" + QString::number(log[j].dest() - data->asCount() + 1);
                        }else{
                            tsxlsentry += "АС" + QString::number(log[j].dest() + 1);
                        }
                    }
                }else{
                    if(log[j].dest() > data->asCount() - 1){
                        tsxlsentry += "ГВМ" + QString::number(log[j].dest() - data->asCount() + 1);
                    }else{
                        tsxlsentry += "АС" + QString::number(log[j].dest() + 1);
                    }
                }
                trans_xlsx.write(tlsxsid, 1, QString("Т") + QString::number(tlsxsid));
                trans_xlsx.write(tlsxsid, 2, tsxlsentry);
                ++tlsxsid;
                /*QGraphicsSimpleTextItem *num = new QGraphicsSimpleTextItem(" " + QString::number(log[j].itemId() + 1));
                scene->addItem(num);
                num->setPos(log[j].start(), i * (h_lines_step + h_lines_width));
                */
            }
            qDebug() << log_str;
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
            scene->addLine(-50,
                           end_y_atms - 5 + h_lines_width,
                           log.last().end(),
                           end_y_atms - 5 + h_lines_width);
            QGraphicsSimpleTextItem *simple_text = new QGraphicsSimpleTextItem("ПП");
            scene->addItem(simple_text);
            simple_text->setPos(-50, end_y_atms - 5);
            end_y_atms += h_lines_step + h_lines_width * 2;
        }


        for(int i{0}; i < data->stored_fpms[z].size(); ++i){
            QString tsxlsentry;

            FPM* c_fpm = data->stored_fpms[z][i].data();
            QVector<LogEntry> &log = c_fpm->getLog();
            for(int j{0}; j < log.size(); ++j){
                scene->addRect(log[j].start(),
                               end_y_atms + i * (h_lines_step + h_lines_width),
                               log[j].duration(),
                               h_lines_width,
                               QPen(),
                               brushes[log[j].itemId()]);
                tsxlsentry = "ГВМ" + QString::number(i + 1) + " обробляє Д" + QString::number(log[j].itemId() + 1) + " на " + QString::number(log[j].op() + 2) + " операції";
                trans_xlsx.write(tlsxsid, 1, QString("Т") + QString::number(tlsxsid));
                trans_xlsx.write(tlsxsid, 2, tsxlsentry);
                ++tlsxsid;
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

        auto c_arch_det_data = data->stored_archives[z].data()->detailed_data;
        QXlsx::Document xlsx;
        QXlsx::Document s_xlsx;
        int i = 1;
        for(auto &it : c_arch_det_data){
            xlsx.write(1, i, QString("ГВМ №") + QString::number(i / 3 + 1));
            s_xlsx.write(i / 3 + 1, 1, QString("ГВМ №") + QString::number(i / 3 + 1));

            xlsx.setColumnWidth(i + 1, i + 1, 5.0);
            xlsx.setColumnWidth(i + 2, i + 2, 15.0);

            xlsx.mergeCells(QXlsx::CellRange(1, i, 1, i+2));
            int j = 2, jj = 2;
            for(auto &iit : it.keys()){
                xlsx.write(j, i, QString::number(iit, 'f', 2));
                //xlsx.write(j, i + 1, it[iit].string_rep);
                //j++;
                if(!it[iit].string_rep.isEmpty()){
                    QString str = it[iit].string_rep;
                    QVector<QString> splstr = str.split(QRegExp("\\s+"), QString::SkipEmptyParts).toVector();
                    if(splstr.first() != QString("-//-")){
                        s_xlsx.write(i / 3 + 1, jj, splstr.first());
                        ++jj;
                        for(int k{0}; k < splstr.size(); ++k){
                            xlsx.write(j, i + 1, splstr[k]);
                            xlsx.write(j, i + 2, it[iit].times_strs[k]);
                            ++j;
                        }
                    }
                }
            }
            i += 3;
        }
        s_xlsx.saveAs(tab_names[z] + "_seq.xlsx");
        xlsx.saveAs(tab_names[z] + ".xlsx");
        trans_xlsx.saveAs(tab_names[z] + "_uniq.xlsx");

    }

    createPetri(tab_names);

    //==============================================================================


    noatms_tabs = new QTabWidget();

    for(int z{0}; z < natmdata->stored_fpms.size(); ++z){
        QGraphicsScene *scene = new QGraphicsScene();

        int max_y{natmdata->stored_fpms[z].size() * (h_lines_step + h_lines_width) + h_lines_width};
        double max_x{natmdata->stored_time[z]};
        const int v_lines_step{50};
        for(int i{0}; i <= max_x/v_lines_step; ++i){
            scene->addLine(i * v_lines_step, max_y, i * v_lines_step, -15, QPen(QColor(Qt::gray)));
            QGraphicsSimpleTextItem *simple_text = new QGraphicsSimpleTextItem(QString::number(i * v_lines_step));
            scene->addItem(simple_text);
            simple_text->setPos(i * v_lines_step, max_y);
        }
        scene->addLine(-50, max_y, max_x, max_y, QPen(QColor(Qt::gray)));
        {
            QGraphicsSimpleTextItem *simple_text = new QGraphicsSimpleTextItem("Час закінчення обробки: " + QString::number(natmdata->stored_time[z]) + " хв");
            scene->addItem(simple_text);
            simple_text->setPos(-50, max_y + 15);
        }


        for(int i{0}; i < natmdata->stored_fpms[z].size(); ++i){
            FPM* c_fpm = natmdata->stored_fpms[z][i].data();
            QVector<LogEntry> &log = c_fpm->getLog();
            for(int j{0}; j < log.size(); ++j){
                scene->addRect(log[j].start(),
                               i * (h_lines_step + h_lines_width),
                               log[j].duration(),
                               h_lines_width,
                               QPen(),
                               brushes[log[j].itemId()]);
            }
            scene->addLine(-50,
                           i * (h_lines_step + h_lines_width) + h_lines_width,
                           log.last().end(),
                           i * (h_lines_step + h_lines_width) + h_lines_width);
            QGraphicsSimpleTextItem *simple_text = new QGraphicsSimpleTextItem("ГВМ №" + QString::number(i+1));
            scene->addItem(simple_text);
            simple_text->setPos(-50, i * (h_lines_step + h_lines_width));
        }

        QGraphicsView *view = new QGraphicsView(scene);
        noatms_tabs->addTab(view, tab_names[z]);
        QString filename = tab_names[z]+"_NOATM.png";
        QImage img(scene->sceneRect().width(), scene->sceneRect().height(), QImage::Format_RGB32);
        img.fill(Qt::white);
        QPainter painter(&img);
        painter.setRenderHint(QPainter::Antialiasing);
        scene->render(&painter);
        img.save(filename);

        auto c_arch_det_data = natmdata->stored_archives[z].data()->detailed_data;
        QXlsx::Document xlsx;
        QXlsx::Document s_xlsx;
        int i = 1;
        for(auto &it : c_arch_det_data){
            xlsx.write(1, i, QString("ГВМ №") + QString::number(i / 3 + 1));
            s_xlsx.write(i / 3 + 1, 1, QString("ГВМ №") + QString::number(i / 3 + 1));

            xlsx.setColumnWidth(i + 1, i + 1, 5.0);
            xlsx.setColumnWidth(i + 2, i + 2, 15.0);

            xlsx.mergeCells(QXlsx::CellRange(1, i, 1, i+2));
            int j = 2, jj = 2;
            for(auto &iit : it.keys()){
                xlsx.write(j, i, QString::number(iit, 'f', 2));
                //xlsx.write(j, i + 1, it[iit].string_rep);
                //j++;
                if(!it[iit].string_rep.isEmpty()){
                    QString str = it[iit].string_rep;
                    QVector<QString> splstr = str.split(QRegExp("\\s+"), QString::SkipEmptyParts).toVector();
                    if(splstr.first() != QString("-//-")){
                        s_xlsx.write(i / 3 + 1, jj, splstr.first());
                        ++jj;
                        for(int k{0}; k < splstr.size(); ++k){
                            xlsx.write(j, i + 1, splstr[k]);
                            xlsx.write(j, i + 2, it[iit].times_strs[k]);
                            ++j;
                        }
                    }
                }
            }
            i += 3;
        }
        s_xlsx.saveAs(tab_names[z] + "_seq_NOATM.xlsx");
        xlsx.saveAs(tab_names[z] + "_NOATM.xlsx");
    }


    //==============================================================================

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

    master_tabs = new QTabWidget();
    master_tabs->addTab(noatms_tabs, "Without ATMs");
    master_tabs->addTab(tabs, "With ATMs");

    textedit = new QTextEdit();

    c_master = master_tabs->currentIndex();
    if(c_master == 1){
        c_slave = tabs->currentIndex();
    }else{
        c_slave = noatms_tabs->currentIndex();
    }
    updateTextEdit();

    QGridLayout *layout = new QGridLayout();
    layout->addWidget(master_tabs, 0, 0, 1, 2);
    layout->addWidget(bview, 1, 1);
    layout->addWidget(textedit, 1, 0);

    layout->setRowStretch(0, 5);
    layout->setRowStretch(1, 2);
    layout->setColumnStretch(0, 3);
    setLayout(layout);
    qDebug() << bview->sceneRect().x() << bview->sceneRect().y() << bview->sceneRect().width() << bview->sceneRect().height();

    connect(master_tabs, &QTabWidget::currentChanged, this, &MainWidget::masterTabCurrentChanged);
    connect(tabs, &QTabWidget::currentChanged, this, &MainWidget::slaveTabCurrentChanged);
    connect(noatms_tabs, &QTabWidget::currentChanged, this, &MainWidget::slaveTabCurrentChanged);


}

MainWidget::~MainWidget()
{

}

void MainWidget::updateTextEdit()
{
    DataStorage *active_storage;
    if(c_master == 1){
        active_storage = d;
    }else{
        active_storage = wad;
    }
    QSharedPointer<CaseChart> &ptr = active_storage->stored_archives[c_slave];
    QMap<int, QVector<Case>> &data = ptr.data()->data;

    QVector<QString> strings;
    strings.resize(active_storage->original_fpms.size() + 1);
    for(int i{0}; i < active_storage->original_fpms.size(); ++i){
        strings[i + 1] = "<tr><td>ГВМ №" + QString::number(d->original_fpms[i].data()->Id() + 1) + "</td>";
    }

    strings.first() = "<tr bgcolor=\"lightgray\"><th></th>";
    for(auto &key : data.keys()){
        strings.first().push_back("<th>" + QString::number(key) + "</th>");
        for(int i{0}; i < data[key].size(); ++i){
            strings[i + 1] += "<td align=\"center\">" + data[key][i].string_rep + "</td>";
        }
    }
    for(int i{0}; i < strings.size(); ++i){
        strings[i].push_back("</tr>");
    }

    QString text{"<table border=\"1\">"};
    for(auto &it : strings){
        text += it;
    }
    text += "</table>";
    textedit->setLineWrapMode(QTextEdit::NoWrap);
    textedit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    textedit->setText(text);
}

void MainWidget::createPetri(QVector<QString> &tab_names)
{
    for(int z{0}; z < d->stored_fpms.size(); ++z){
        QMap<QString, QString> great_cache; //like great wall, but great cache
        QMap<QString, QString> prev_bufers_map;
        QXlsx::Document petri_xlsx;
        int t_id{1}, p_id{1};
        int crow{2};
        petri_xlsx.setColumnWidth(1, 1, 5.0);
        petri_xlsx.setColumnWidth(2, 2, 10.0);
        petri_xlsx.setColumnWidth(3, 3, 55.0);
        petri_xlsx.setColumnWidth(4, 4, 11.0);
        petri_xlsx.setColumnWidth(5, 5, 9.0);

        petri_xlsx.write(1, 1, QString("Перехід"));
        petri_xlsx.write(1, 2, QString("Позиція"));
        petri_xlsx.write(1, 3, QString("Опис"));
        petri_xlsx.write(1, 4, QString("Вхід/вихід"));
        petri_xlsx.write(1, 5, QString("Інгібітор"));


        for(int i{0}; i < d->stored_atms[z].size(); ++i){
            t_id += d->stored_atms[z][i].data()->getLog().size();
        }

        for(int i{0}; i < d->stored_fpms[z].size(); ++i){

            FPM* c_fpm = d->stored_fpms[z][i].data();
            QVector<LogEntry> &log = c_fpm->getLog();

            int s_row{crow};
            bool two_bufers = d->fpm_has_two_buffers[i];

            QString fpm_name = "ГВМ" + QString::number(c_fpm->Id() + 1);
            for(int j{0}; j < log.size(); ++j){
                QString p_1, p_2, p_3, p_4, p_999, p_1000;

                QString item_name = "Д" + QString::number(log[j].itemId() + 1);
                QString op_name = QString::number(log[j].op() + 2);
                QString after_of_before;
                after_of_before = "перед " + op_name + " операцією";
                /*if(log[j].op() == -1){
                    after_of_before = "перед " + op_name + " операцією";
                }else{
                    after_of_before = "після " + op_name + " операції";
                }*/

                QString next_item_name;
                QString prev_item_name;
                QString next_op_name;
                QString prev_op_name;
                if(j == log.size() - 1){
                    next_item_name = "Д" + QString::number(log[0].itemId() + 1);
                    if(log.size() == 1){
                        prev_item_name = "Д" + QString::number(log[0].itemId() + 1);
                    }else{
                        prev_item_name = "Д" + QString::number(log[j - 1].itemId() + 1);
                    }
                    next_op_name = QString::number(log[0].op() + 2);
                    if(log.size() == 1){
                        prev_op_name = QString::number(log[0].op() + 2);
                    }else{
                        prev_op_name = QString::number(log[j - 1].op() + 2);
                    }
                }else if(!j){
                    next_item_name = "Д" + QString::number(log[1].itemId() + 1);
                    prev_item_name = "Д" + QString::number(log[log.size() - 1].itemId() + 1);
                    next_op_name = QString::number(log[1].op() + 2);
                    prev_op_name = QString::number(log[log.size() - 1].op() + 2);
                }else{
                    next_item_name = "Д" + QString::number(log[j + 1].itemId() + 1);
                    prev_item_name = "Д" + QString::number(log[j - 1].itemId() + 1);
                    next_op_name = QString::number(log[j + 1].op() + 2);
                    prev_op_name = QString::number(log[j - 1].op() + 2);
                }

                p_1 = fpm_name + " готовий обробити " + item_name + " на " + op_name + " операції";
                if(two_bufers){
                    p_2 = item_name + " у вхідному накопичувачі " + fpm_name + " " + after_of_before;
                    p_3 = item_name + " у вихідному накопичувачі " + fpm_name + " після " + op_name + " операції";
                    p_999 = prev_item_name + " у вихідному накопичувачі " + fpm_name + " після " + prev_op_name + " операції";
                    p_1000 = prev_item_name + " у вхідному накопичувачі " + fpm_name + " перед " + prev_op_name + " операцією";
                }else{
                    p_2 = item_name + " у накопичувачі " + fpm_name + " " + after_of_before;
                    p_3 = item_name + " у накопичувачі " + fpm_name + " після " + op_name + " операції";
                    p_999 = prev_item_name + " у накопичувачі " + fpm_name + " після " + prev_op_name + " операції";
                    p_1000 = prev_item_name + " у накопичувачі " + fpm_name + " перед " + prev_op_name + " операцією";
        //---------------------------------
        //DO NOT FORGET TO CHECK THIS! ^^^
        //---------------------------------
                }
                p_4 = fpm_name + " готовий обробити " + next_item_name + " на " + next_op_name + " операції";

                prev_bufers_map[p_2] = p_1000;
                if(!two_bufers){
                    prev_bufers_map[p_3] = p_999;
                }

                petri_xlsx.write(crow, 1, QString("Т") + QString::number(t_id));

                petri_xlsx.write(crow, 3, p_1);
                petri_xlsx.write(crow + 1, 3, p_2);
                petri_xlsx.write(crow + 2, 3, p_3);
                petri_xlsx.write(crow + 3, 3, p_4);
                if(two_bufers){
                    petri_xlsx.write(crow + 4, 3, p_999);
                }else{
                    petri_xlsx.write(crow + 4, 3, p_1000);
                    petri_xlsx.write(crow + 5, 3, p_999);
                }

                petri_xlsx.write(crow, 2, QString("P") + QString::number(c_fpm->Id() + 1) + QString("_") + QString::number(p_id));

                great_cache[p_2] = QString("P") + QString::number(c_fpm->Id() + 1) + QString("_") + QString::number(p_id + 1);

                great_cache[p_3] = QString("P") + QString::number(c_fpm->Id() + 1) + QString("_") + QString::number(p_id + 2);
                p_id += 2;
                /*if(two_bufers){
                    great_cache[p_3] = QString("P") + QString::number(c_fpm->Id() + 1) + QString("_") + QString::number(p_id + 2);
                    p_id += 2;
                }else{
                    great_cache[p_3] = QString("P") + QString::number(c_fpm->Id() + 1) + QString("_") + QString::number(p_id + 1);
                    ++p_id;
                }*/

                petri_xlsx.write(crow + 1, 2, great_cache[p_2]);
                petri_xlsx.write(crow + 2, 2, great_cache[p_3]);

                if(j == log.size() - 1){
                    petri_xlsx.write(crow + 3, 2, QString("P") + QString::number(c_fpm->Id() + 1) + QString("_1"));
                }else{
                    ++p_id;
                    petri_xlsx.write(crow + 3, 2, QString("P") + QString::number(c_fpm->Id() + 1) + QString("_") + QString::number(p_id));
                }

                if(!j){
                    //nop
                }else if(j == log.size() - 1){
                    if(two_bufers){
                        petri_xlsx.write(crow + 4, 2, great_cache[p_999]);
                        petri_xlsx.write(s_row + 4, 2, great_cache[p_3]);
                    }else{
                        petri_xlsx.write(crow + 4, 2, great_cache[p_1000]);
                        petri_xlsx.write(crow + 5, 2, great_cache[p_999]);
                        petri_xlsx.write(s_row + 4, 2, great_cache[p_2]);
                        petri_xlsx.write(s_row + 5, 2, great_cache[p_3]);
                    }
                }else{
                    if(two_bufers){
                        petri_xlsx.write(crow + 4, 2, great_cache[p_999]);
                    }else{
                        petri_xlsx.write(crow + 4, 2, great_cache[p_1000]);
                        petri_xlsx.write(crow + 5, 2, great_cache[p_999]);
                    }
                }

                petri_xlsx.write(crow, 4, QString("вхід"));
                petri_xlsx.write(crow + 1, 4, QString("вхід"));
                petri_xlsx.write(crow + 2, 4, QString("вихід"));
                petri_xlsx.write(crow + 3, 4, QString("вихід"));
                petri_xlsx.write(crow + 4, 5, QString("інгибітор"));
                if(!two_bufers){
                    petri_xlsx.write(crow + 5, 5, QString("інгибітор"));
                }

                if(two_bufers){
                    crow += 5;
                }else{
                    crow += 6;
                }
                ++t_id;
            }
            p_id = 1;
        }

        if(d->ts_count == 2){
            BaseFactory *c_reloader = d->stored_reloaders[z].data();
            QVector<LogEntry> &log = c_reloader->getLog();

            for(int j{0}; j < log.size(); ++j){
                QString p_1, p_2, p_prev_1, p_prev_2;

                int ciop = log[j].op() + 1;
                QString item_name = "Д" + QString::number(log[j].itemId() + 1);

                p_1 = item_name + " в ПП після " + QString::number(ciop) + " операції";
                p_2 = item_name + " в ПП перед " + QString::number(ciop + 1) + " операцією";

                great_cache[p_1] = QString("R_") + QString::number(p_id);
                great_cache[p_2] = QString("R_") + QString::number(p_id);

                int prev_id = j - 1;
                if(prev_id < 0){
                    prev_id = log.size() - 1;
                }
                ciop = log[prev_id].op() + 1;

                QString prev_item_name = "Д" + QString::number(log[prev_id].itemId() + 1);

                p_prev_1 = prev_item_name + " в ПП після " + QString::number(ciop) + " операції";
                //p_prev_2 = prev_item_name + " в ПП перед " + QString::number(ciop + 1) + " операцією";

                prev_bufers_map[p_1] = p_prev_1;
                prev_bufers_map[p_2] = p_prev_1;

                ++p_id;
            }
            p_id = 1;
        }

        t_id = 1;

        for(int i{0}; i < d->stored_atms[z].size(); ++i){
            ATM* c_atm = d->stored_atms[z][i].data();
            QVector<LogEntry> &log = c_atm->getLog();

            //int s_row{crow};

            QString atm_name = "АТМ" + QString::number(c_atm->Id() + 1);
            for(int j{0}; j < log.size(); ++j){
                QString p_1, p_2, p_3, p_4, p_prev_1, p_prev_2;

                int ciop = log[j].op() + 2;
                QString item_name = "Д" + QString::number(log[j].itemId() + 1);
                QString op_name = QString::number(ciop);

                QString before_or_after;
                if(log[j].op() != -1){
                    --ciop;
                    op_name = QString::number(ciop);
                    before_or_after = "після " + op_name + " операції";
                }else{
                    before_or_after = "перед " + op_name + " операцією";
                }

                QString from_name;
                if(log[j].cross_move != 2){
                    if(log[j].from() < d->asCount()){
                        from_name = "АС" + QString::number(log[j].from() + 1);
                    }else{
                        from_name = "ГВМ" + QString::number(log[j].from() -  d->asCount() + 1);
                    }
                }else{
                    from_name = "ПП";
                }
                QString dest_name;
                if(log[j].cross_move != 1){
                    if(log[j].dest() < d->asCount()){
                        dest_name = "АС" + QString::number(log[j].dest() + 1);
                    }else{
                        dest_name = "ГВМ" + QString::number(log[j].dest() -  d->asCount() + 1);
                    }
                }else{
                    dest_name = "ПП";
                }


                int next_log_id, prev_log_id;
                if(j == log.size() - 1){
                    next_log_id = 0;
                    prev_log_id = j - 1;
                }else if(!j){
                    next_log_id = 1;
                    prev_log_id = log.size() - 1;
                }else{
                    next_log_id = j + 1;
                    prev_log_id = j - 1;
                }

                QString next_item_name = "Д" + QString::number(log[next_log_id].itemId() + 1);

                QString next_from_name;
                if(log[next_log_id].cross_move != 2){
                    if(log[next_log_id].from() < d->asCount()){
                        next_from_name = "АС" + QString::number(log[next_log_id].from() + 1);
                    }else{
                        next_from_name = "ГВМ" + QString::number(log[next_log_id].from() -  d->asCount() + 1);
                    }
                }else{
                    next_from_name = "ПП";
                }

                QString next_dest_name;
                if(log[next_log_id].cross_move != 1){
                    if(log[next_log_id].dest() < d->asCount()){
                        next_dest_name = "АС" + QString::number(log[next_log_id].dest() + 1);
                    }else{
                        next_dest_name = "ГВМ" + QString::number(log[next_log_id].dest() -  d->asCount() + 1);
                    }
                }else{
                    next_dest_name = "ПП";
                }

                p_1 = atm_name + " готовий транспортувати " + item_name + " з " + from_name + " до " + dest_name;
                if(from_name.contains(QString("АС"))){
                    p_2 = item_name + " на " + from_name + " " + before_or_after;
                }else if(from_name.contains(QString("ПП"))){
                    p_2 = item_name + " в " + from_name + " " + before_or_after;
                }else{
                    if(d->fpm_has_two_buffers[log[j].from() - d->asCount()]){
                        p_2 = item_name + " у вихідному накопичувачі " + from_name + " " + before_or_after;
                    }else{
                        p_2 = item_name + " у накопичувачі " + from_name + " " + before_or_after;
                    }
                }
                if(dest_name.contains(QString("АС"))){
                    p_3 = item_name + " на " + dest_name + " перед ";// + next_op_name + " операцією";
                }else if(dest_name.contains(QString("ПП"))){
                    p_3 = item_name + " в " + dest_name + " перед ";// + next_op_name + " операцією";
                }else{
                    if(d->fpm_has_two_buffers[log[j].dest() - d->asCount()]){
                        p_3 = item_name + " у вхідному накопичувачі " + dest_name + " перед ";// + next_op_name + " операцією";
                    }else{
                        p_3 = item_name + " у накопичувачі " + dest_name + " перед ";// + next_op_name + " операцією";
                    }
                }
                if(log[j].op() == -1){
                    p_3 += "1 операцією";
                }else{
                    p_3 += QString::number(ciop + 1) + " операцією";
                }
                p_4 = atm_name + " готовий транспортувати " + next_item_name + " з " + next_from_name + " до " + next_dest_name;

                if(p_3.contains(QString("ГВМ"))){
                    if(d->fpm_has_two_buffers[log[j].dest() - d->asCount()]){
                        p_prev_1 = prev_bufers_map[p_3];
                        if(p_prev_1.isEmpty()){
                            qDebug() << p_3 << "not found 1" << tab_names[z];
                        }
                    }else{
                        p_prev_1 = prev_bufers_map[p_3];
                        QString tmp = item_name + " у накопичувачі " + dest_name + " після ";
                        if(log[j].op() == -1){
                            tmp += "1 операції";
                        }else{
                            tmp += QString::number(ciop + 1) + " операції";
                        }

                        p_prev_2 = prev_bufers_map[tmp];
                        if(p_prev_1.isEmpty()){
                            qDebug() << p_3 << "not found 2" << tab_names[z];
                        }
                        if(p_prev_2.isEmpty()){
                            qDebug() << tmp << "not found 3" << tab_names[z];
                        }
                    }
                }else if(p_3.contains(QString("ПП"))){
                    p_prev_1 = prev_bufers_map[p_3];
                }

                petri_xlsx.write(crow, 1, QString("Т") + QString::number(t_id));

                petri_xlsx.write(crow, 3, p_1);
                petri_xlsx.write(crow + 1, 3, p_2);
                petri_xlsx.write(crow + 2, 3, p_3);
                petri_xlsx.write(crow + 3, 3, p_4);
                if(!p_prev_1.isEmpty()){
                    petri_xlsx.write(crow + 4, 3, p_prev_1);
                }
                if(!p_prev_2.isEmpty()){
                    petri_xlsx.write(crow + 5, 3, p_prev_2);
                }

                if(!j){
                    petri_xlsx.write(crow, 2, QString("А") + QString::number(c_atm->Id() + 1) + QString("_") + QString::number(p_id));
                    if(great_cache.count(p_2)){
                        petri_xlsx.write(crow + 1, 2, great_cache[p_2]);
                    }else{
                        ++p_id;
                        petri_xlsx.write(crow + 1, 2, QString("А") + QString::number(c_atm->Id() + 1) + QString("_") + QString::number(p_id));
                    }
                    if(great_cache.count(p_3)){
                        petri_xlsx.write(crow + 2, 2, great_cache[p_3]);
                    }else{
                        ++p_id;
                        petri_xlsx.write(crow + 2, 2, QString("А") + QString::number(c_atm->Id() + 1) + QString("_") + QString::number(p_id));
                    }
                    ++p_id;
                    petri_xlsx.write(crow + 3, 2, QString("А") + QString::number(c_atm->Id() + 1) + QString("_") + QString::number(p_id));
                }else if(j == log.size() - 1){
                    petri_xlsx.write(crow, 2, QString("А") + QString::number(c_atm->Id() + 1) + QString("_") + QString::number(p_id));
                    if(great_cache.count(p_2)){
                        petri_xlsx.write(crow + 1, 2, great_cache[p_2]);
                    }else{
                        ++p_id;
                        petri_xlsx.write(crow + 1, 2, QString("А") + QString::number(c_atm->Id() + 1) + QString("_") + QString::number(p_id));
                    }
                    if(great_cache.count(p_3)){
                        petri_xlsx.write(crow + 2, 2, great_cache[p_3]);
                    }else{
                        ++p_id;
                        petri_xlsx.write(crow + 2, 2, QString("А") + QString::number(c_atm->Id() + 1) + QString("_") + QString::number(p_id));
                    }
                    petri_xlsx.write(crow + 3, 2, QString("А") + QString::number(c_atm->Id() + 1) + QString("_1"));
                    //petri_xlsx.write(crow + 4, 2, QString("А") + QString::number(c_atm->Id() + 1) + QString("_") + QString::number(p_id - 1));
                    //petri_xlsx.write(s_row + 4, 2, QString("А") + QString::number(c_atm->Id() + 1) + QString("_") + QString::number(p_id + 2));
                }else{
                    petri_xlsx.write(crow, 2, QString("А") + QString::number(c_atm->Id() + 1) + QString("_") + QString::number(p_id));
                    if(great_cache.count(p_2)){
                        petri_xlsx.write(crow + 1, 2, great_cache[p_2]);
                    }else{
                        ++p_id;
                        petri_xlsx.write(crow + 1, 2, QString("А") + QString::number(c_atm->Id() + 1) + QString("_") + QString::number(p_id));
                    }
                    if(great_cache.count(p_3)){
                        petri_xlsx.write(crow + 2, 2, great_cache[p_3]);
                    }else{
                        ++p_id;
                        petri_xlsx.write(crow + 2, 2, QString("А") + QString::number(c_atm->Id() + 1) + QString("_") + QString::number(p_id));
                    }
                    ++p_id;
                    petri_xlsx.write(crow + 3, 2, QString("А") + QString::number(c_atm->Id() + 1) + QString("_") + QString::number(p_id));
                    //petri_xlsx.write(crow + 4, 2, QString("А") + QString::number(c_atm->Id() + 1) + QString("_") + QString::number(p_id - 1));
                }

                if(!p_prev_1.isEmpty()){
                    petri_xlsx.write(crow + 4, 2, great_cache[p_prev_1]);
                }
                if(!p_prev_2.isEmpty()){
                    petri_xlsx.write(crow + 5, 2, great_cache[p_prev_2]);
                }

                petri_xlsx.write(crow, 4, QString("вхід"));
                petri_xlsx.write(crow + 1, 4, QString("вхід"));
                petri_xlsx.write(crow + 2, 4, QString("вихід"));
                petri_xlsx.write(crow + 3, 4, QString("вихід"));
                if(!p_prev_1.isEmpty()){
                    petri_xlsx.write(crow + 4, 5, QString("інгибітор"));
                }
                if(!p_prev_2.isEmpty()){
                    petri_xlsx.write(crow + 5, 5, QString("інгибітор"));
                }
                //petri_xlsx.write(crow + 4, 5, QString("інгибітор"));

                //p_id += 3;
                //crow += 5;
                crow += 4;
                if(!p_prev_1.isEmpty()){
                    ++crow;
                }
                if(!p_prev_2.isEmpty()){
                    ++crow;
                }
                ++t_id;
            }
            p_id = 1;
        }

        petri_xlsx.saveAs(tab_names[z] + "_petri.xlsx");
    }
}

void MainWidget::masterTabCurrentChanged(int index)
{
    c_master = index;
    if(c_master == 1){
        c_slave = tabs->currentIndex();
    }else{
        c_slave = noatms_tabs->currentIndex();
    }
    updateTextEdit();
}

void MainWidget::slaveTabCurrentChanged(int index)
{
    c_slave = index;
    updateTextEdit();
}
