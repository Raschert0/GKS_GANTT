#include <QFile>
#include <QTextStream>
#include <QStringList>

#include <QDebug>
#include <QApplication>

#include <algorithm>

#include "datastorage.h"
#include "errorshandler.h"

#include "xlsxdocument.h"

bool cmp_opps(const ATMSel &a, const ATMSel &b){
    return a.time_for_all_moves < b.time_for_all_moves;
}

bool cmp_opps_outer(const QVector<ATMSel> &a, const QVector<ATMSel> &b){
    return std::min_element(a.constBegin(), a.constEnd(), cmp_opps)->time_for_all_moves < std::min_element(b.constBegin(), b.constEnd(), cmp_opps)->time_for_all_moves;
}

bool operator ==(const ATMSel &a, const ATMSel &b){
    return a.time_for_all_moves == b.time_for_all_moves
            && a.used_atms == b.used_atms
            && a.used_ts == b.used_ts;
}

DataStorage::DataStorage()
{
    current_rule = QSharedPointer<SupRule>(new FirstRule());
}

DataStorage::~DataStorage()
{
    stored_archives.clear();
    archive.clear();
}

void DataStorage::clear()
{
    Item::resetIdCounter();
    AS::resetIdCounter();
    ATM::resetIdCounter();
    FPM::resetIdCounter();
}

bool DataStorage::load(QString filename)
{
    clear();
    QFile f;
    f.setFileName(filename);
    if(!f.exists() || !f.open(QIODevice::ReadOnly)){
        return false;
    }

    QTextStream in(&f);

    if(in.atEnd()){
        ErrorsHandler::outErr(ErrorCode::UNEXPECTED_END_OF_FILE, __LINE__);
        return false;
    }

    if(!readNumber(in, items_count, __LINE__))    return false;
    if(!readNumber(in, fpms_count, __LINE__))     return false;
    if(!readNumber(in, ts_count, __LINE__))       return false;
    if(!readNumber(in, atms_count, __LINE__))     return false;
    if(!readNumber(in, as_count, __LINE__))     return false;
    in.readLine();

    archive.reset(new CaseChart(fpms_count));

    if(no_atm){
        atms_count = 0;
        ts_count = 0;
    }else if(ts_count < 1 || ts_count > 2){
        ErrorsHandler::outErr(ErrorCode::INCORRECT_NUMBER_OF_TS);
        return false;
    }

    if(ts_count == 2){
        reloader = QSharedPointer<BaseFactory>(new BaseFactory());
    }

    //Створення "об'єктів" деталей і ГВМ
    for(int i{0}; i < items_count; ++i){
        original_items.push_back(QSharedPointer<Item>(new Item()));
        items.push_back(QSharedPointer<Item>(new Item(*original_items.last().data())));
    }
    for(int i{0}; i < fpms_count; ++i){
        original_fpms.push_back(QSharedPointer<FPM>(new FPM()));
        fpms.push_back(QSharedPointer<FPM>(new FPM(*original_fpms.last().data())));
    }

    //Для випадку лінійно-кільцевої мережі
    if(ts_count == 1){
        for(int i{0}; i < atms_count; ++i){
            original_atms.push_back(QSharedPointer<ATM>(new ATM()));
            atms.push_back(QSharedPointer<ATM>(new ATM(*original_atms.last().data())));
        }
    }else if(ts_count == 2){
        int circle_atms_count = atms_count - 1; //Розбиття АТМ на частини
        int line_atms_count = 1;
        for(int i{0}; i < circle_atms_count; ++i){
            original_atms.push_back(QSharedPointer<ATM>(new ATM()));
            atms.push_back(QSharedPointer<ATM>(new ATM(*original_atms.last().data())));

        }
        for(int i{0}; i < line_atms_count; ++i){
            original_atms.push_back(QSharedPointer<ATM>(new ATM(1)));
            atms.push_back(QSharedPointer<ATM>(new ATM(*original_atms.last().data())));
        }
    }

    //Сторвення об'єктів для завантаження/розвантаження з АС
    for(int i{0}; i < as_count; ++i){
        original_as.push_back(QSharedPointer<AS>(new AS()));
        as.push_back(QSharedPointer<AS>(new AS(*original_as.last().data())));
    }

    //Оскільки послідовність ГВМ і час обробки на ГВМ - дві різні матриці, записуємо послідовність в тимчасовий веткор
    QVector<QVector<int>> temp_operations_vec;
    temp_operations_vec.resize(items_count);
    for(int i{0}; i < items_count; ++i){
        QVector<QString> c_line_split = in.readLine().split(QRegExp("\\s+"), QString::SkipEmptyParts).toVector();
        for(int j{0}; j < c_line_split.size(); ++j){
            bool status;
            int c_op = c_line_split[j].toInt(&status);
            if(!status || c_op > fpms.size()){
                ErrorsHandler::outErr(ErrorCode::PARSE_ERROR, __LINE__);
                return false;
            }
            c_op = c_op - 1 + as_count; //Нормалізація? Типу того
            temp_operations_vec[i].push_back(c_op);
        }
    }

    for(int i{0}; i < items_count; ++i){
        QVector<QString> c_line_split = in.readLine().split(QRegExp("\\s+"), QString::SkipEmptyParts).toVector();
        if(c_line_split.size() != temp_operations_vec[i].size()){
            ErrorsHandler::outErr(ErrorCode::PARSE_ERROR, __LINE__);
            return false;
        }
        for(int j{0}; j < c_line_split.size(); ++j){
            bool status;
            double time = c_line_split[j].toDouble(&status);
            if(!status){
                ErrorsHandler::outErr(ErrorCode::PARSE_ERROR, __LINE__);
                return false;
            }
            original_items[i].data()->addOperation(temp_operations_vec[i][j], time);
            items[i].data()->addOperation(temp_operations_vec[i][j], time);
        }
    }

    {
        QVector<QString> c_line_split = in.readLine().split(QRegExp("\\s+"), QString::SkipEmptyParts).toVector();
        for(int j{0}; j < c_line_split.size(); ++j){
            bool status;
            int bool_rep = c_line_split[j].toInt(&status);
            if(!status || bool_rep > fpms.size()){
                ErrorsHandler::outErr(ErrorCode::PARSE_ERROR, __LINE__);
                return false;
            }
            fpm_has_two_buffers.push_back(bool_rep);
        }
    }

    if(!no_atm){
        //Завантаження часу
        double move_speed;
        double move_dist;

        if(!readNumber(in, time_to_take_place, __LINE__)) return false;
        if(!readNumber(in, time_to_load, __LINE__))  return false;
        if(!readNumber(in, time_to_unload, __LINE__))  return false;
        if(!readNumber(in, move_speed, __LINE__)) return false;
        if(!readNumber(in, move_dist, __LINE__))  return false;
        in.readLine();

        time_to_move = move_dist / move_speed;

        int pois_count{fpms_count + as_count};

        if(ts_count == 1){
            time_to_move_to_from.resize(1);
        }else{
            time_to_move_to_from.resize(4);
            move_intersections.resize(2);
            for(int i{0}; i < 2; ++i){
                move_intersections[i].resize(pois_count);
                for(int j{0}; j < pois_count; ++j){
                    move_intersections[i][j].resize(pois_count);
                }
            }
        }

        for(int i{0}; i < time_to_move_to_from.size(); ++i){
            time_to_move_to_from[i].resize(pois_count);
            for(int j{0}; j < pois_count; ++j){
                time_to_move_to_from[i][j].resize(pois_count);
            }
        }
        time_to_pos_to_from.resize(ts_count);
        for(int i{0}; i < ts_count; ++i){
            time_to_pos_to_from[i].resize(pois_count);
            for(int j{0}; j < pois_count; ++j){
                time_to_pos_to_from[i][j].resize(pois_count);
            }
        }


        QVector<QVector<int>> pois_sentences;
        pois_sentences.resize(ts_count);
        for(int cts{0}; cts < ts_count; ++cts){
            QVector<QString> c_line_split = in.readLine().split(QRegExp("\\s+"), QString::SkipEmptyParts).toVector();
            for(int i{0}; i < c_line_split.size(); ++i){
                bool status;
                double cid = c_line_split[i].toInt(&status);
                if(!status){
                    ErrorsHandler::outErr(ErrorCode::PARSE_ERROR, __LINE__);
                    return false;
                }
                pois_sentences[cts].push_back(cid);
            }

            for(int i{-as_count}; i <= fpms_count; ++i){
                int pos1 = pois_sentences[cts].indexOf(i);
                if(pos1 < 0){
                    continue;
                }
                int zero_corrector_i = i >= 0 ? 1 : 0;
                for(int j{-as_count}; j <= fpms_count; ++j){
                    if(i == j){
                        continue;
                    }
                    int pos2 = pois_sentences[cts].indexOf(j);
                    if(pos2 < 0){
                        continue;
                    }
                    int zero_corrector_j = j >= 0 ? 1 : 0;

                    int dist{0};
                    if(cts == 0){
                        if(pos1 < pos2){
                            dist = pos2 - pos1;
                        }else{
                            dist = pos2 + pois_sentences[cts].size() - pos1;
                        }
                    }else{
                        dist = abs(pos2 - pos1);
                    }

                    time_to_pos_to_from[cts][j + as_count - zero_corrector_j][i + as_count - zero_corrector_i] = dist * time_to_move;
                }
            }
        }

        for(int cts{0}; cts < ts_count; ++cts){
            for(int from{0}; from < pois_count; ++from){
                for(int to{0}; to < pois_count; ++to){
                    if(!time_to_pos_to_from[cts][to][from]){
                        continue;
                    }
                    double time{time_to_pos_to_from[cts][to][from] + time_to_take_place * 2};
                    if(from > as_count - 1){
                        time += time_to_unload;
                    }
                    if(to > as_count - 1){
                        time += time_to_load;
                    }
                    time_to_move_to_from[cts][to][from] = time;
                }
            }
        }

        if(ts_count == 2){ //MAGIC NUMBERS ZONE. BE CAREFULL
            QVector<QString> c_line_split = in.readLine().split(QRegExp("\\s+"), QString::SkipEmptyParts).toVector();
            QVector<int> intersections;
            for(int i{0}; i < c_line_split.size(); ++i){
                bool status;
                double cid = c_line_split[i].toInt(&status);
                if(!status){
                    ErrorsHandler::outErr(ErrorCode::PARSE_ERROR, __LINE__);
                    return false;
                }
                intersections.push_back(cid - 1 + as_count);
            }
            for(auto &it : atms){
                if(it.data()->tId() == 1){
                    it.data()->moveTo(intersections.first());
                }
            }
            //Знаходження найкоротшого часу для переміщення деталі між ТС і відповідної точки перетину
            for(int from{0}; from < pois_count; ++from){
                for(int to{0}; to < pois_count; ++to){
                    for(int cross{0}; cross < intersections.size(); ++cross){
                        int cross_pos = intersections[cross];

                        //from line to circle (remember: 1 - line, 0 - circle)
                        if(time_to_pos_to_from[1][cross_pos][from] && time_to_pos_to_from[0][to][cross_pos]){
                            double cross_time = time_to_pos_to_from[1][cross_pos][from] + time_to_pos_to_from[0][to][cross_pos] + time_to_take_place * 4;
                            if(from > as_count - 1){
                                cross_time += time_to_unload;
                            }
                            if(to > as_count - 1){
                                cross_time += time_to_load;
                            }
                            if(!time_to_move_to_from[2][to][from] || time_to_move_to_from[2][to][from] > cross_time){
                                time_to_move_to_from[2][to][from] = cross_time;
                                move_intersections[0][to][from] = cross_pos;
                            }
                        }

                        //from circle to line (remember: 1 - line, 0 - circle)
                        if(time_to_pos_to_from[0][cross_pos][from] && time_to_pos_to_from[1][to][cross_pos]){
                            double cross_time = time_to_pos_to_from[0][cross_pos][from] + time_to_pos_to_from[1][to][cross_pos] + time_to_take_place * 4;
                            if(from > as_count - 1){
                                cross_time += time_to_unload;
                            }
                            if(to > as_count - 1){
                                cross_time += time_to_load;
                            }
                            if(!time_to_move_to_from[3][to][from] || time_to_move_to_from[3][to][from] > cross_time){
                                time_to_move_to_from[3][to][from] = cross_time;
                                move_intersections[1][to][from] = cross_pos;
                            }
                        }
                    }
                }
            }
        }

    }//end of no_atm if

    f.close();

    return true;
}

void DataStorage::reset()
{
    stored_time.push_back(current_time);
    current_time = 0;

    if(!no_atm){
        stored_atms.push_back(atms);
        atms.clear();
    }

    stored_fpms.push_back(fpms);
    fpms.clear();

    items.clear();

    for(int i{0}; i < original_atms.size(); ++i){
        atms.push_back(QSharedPointer<ATM>(new ATM(*original_atms[i].data())));
    }

    for(int i{0}; i < original_fpms.size(); ++i){
        fpms.push_back(QSharedPointer<FPM>(new FPM(*original_fpms[i].data())));
    }

    for(int i{0}; i < original_items.size(); ++i){
        items.push_back(QSharedPointer<Item>(new Item(*original_items[i].data())));
    }

    if(reloader.data() != nullptr){
        stored_reloaders.push_back(reloader);
        reloader.reset(new BaseFactory());
    }

    if(archive.data() != nullptr){
        stored_archives.push_back(archive);
        archive.reset(new CaseChart(original_fpms.size()));
    }
}

void DataStorage::setNewRule(SupRule *ptr)
{
    current_rule.reset(ptr);
}

void DataStorage::firstRun()
{
    for(int i{0}; i < items.size(); ++i){
        int n_fpm_id = items[i].data()->nextFPMid();

        FPM* n_fpm = fpms[n_fpm_id - as_count].data();
        n_fpm->addToQueue(items[i]);

    }
    if(!no_atm){
        qDebug() << "Atm's first run";
    }
}

void DataStorage::calculateThemAll()
{
    static const QVector<QString> XlsxNames = {"Правило найкоротшої операції",
                                              "Правило максимальної залишкової трудомісткості",
                                              "Правило вирівнювання завантаження верстатів",
                                              "Правило мінімальної залишкової трудомісткості",
                                              "Правило найдовшої операції"
                                              };

    QXlsx::Document atm_cases;
    if(!no_atm){
        atm_cases.write(1, 1, QString("Час"));
        atm_cases.write(1, 2, QString("Деталь"));
        int i{0};
        for(; i < atms_count; ++i){
            atm_cases.write(1, i + 3, QString("АТМ") + QString::number(i + 1));
        }
        if(ts_count > 1){
            for(int j{0}; j < atms_count - 1; ++j){
                atm_cases.write(1, i + 3, QString("АТМ") + QString::number(atms_count) + QString(" + АТМ") + QString::number(j + 1));
                atm_cases.setColumnWidth(i+3, i+3, 13.0);
                ++i;
            }
            for(int j{0}; j < atms_count - 1; ++j){
                atm_cases.write(1, i + 3, QString("АТМ") + QString::number(j + 1) + QString(" + АТМ") + QString::number(atms_count));
                atm_cases.setColumnWidth(i+3, i+3, 13.0);
                ++i;
            }
        }
    }

    int crow{2};

    bool working;
    do{
        working = false;

        //ATMs processing
        QVector<QSharedPointer<ATM>> free_atms;
        QVector<QSharedPointer<ATM>> busy_atms;

        if(!no_atm){
            for(int i{0}; i < atms.size(); ++i){
                if(atms[i].data()->status() != FStatus::Idle){
                    working = true;
                    busy_atms.push_back(atms[i]);
                    continue;
                }
                free_atms.push_back(atms[i]);
            }

            for(int i{busy_atms.size() - 1}; i >= 0; --i){
                QVector<LogEntry> &c_log = busy_atms[i].data()->getLog();
                for(int j{0}; j < c_log.size(); ++j){
                    if(!c_log[j].isDone()){
                        if(c_log[j].end() < current_time){
                            QSharedPointer<Item> transf_item = items[c_log[j].itemId()];
                            //int iid = c_log[j].itemId();

                            if(transf_item.data()->factory() == busy_atms[i]){
                                int next_fpm = c_log[j].dest();
                                if(next_fpm > as_count - 1){
                                    QSharedPointer<FPM> recpt_fpm = fpms[next_fpm - as_count];
                                    transf_item.data()->setFactory(recpt_fpm);
                                    recpt_fpm.data()->setStatus(FStatus::Busy);
                                    recpt_fpm.data()->getLog().push_back(LogEntry(current_time, transf_item.data()->nextFPMtime(), transf_item.data()->id(), transf_item.data()->currentOp()));
                                    recpt_fpm.data()->selectFromQueueById(transf_item.data()->id());

                                    Case tcase;
                                    tcase.string_rep += "[" + QString::number(recpt_fpm.data()->last_best.absolute_item_id + 1) + "] ";
                                    tcase.times_strs.push_back(QString::number(recpt_fpm.data()->last_best.best_time, 'f', 2));
                                    if(recpt_fpm.data()->last_best.alt_calculated){
                                        tcase.times_strs.last().push_back(" - " + QString::number(recpt_fpm.data()->last_best.alt_time, 'f', 2));
                                    }
                                    for(int i{0}; i < recpt_fpm.data()->last_best.relative_candidates.size(); ++i){
                                        if(recpt_fpm.data()->last_best.relative_candidates[i] == recpt_fpm.data()->last_best.relative_item_id){
                                            continue;
                                        }
                                        tcase.string_rep += QString::number(recpt_fpm.data()->last_best.absolute_candidates[i] + 1) + " ";
                                        tcase.times_strs.push_back(QString::number(recpt_fpm.data()->last_best.candidates_times[i], 'f', 2));
                                        if(recpt_fpm.data()->last_best.alt_calculated && recpt_fpm.data()->last_best.best_time == recpt_fpm.data()->last_best.candidates_times[i]){
                                            tcase.times_strs.last().push_back(" - " + QString::number(recpt_fpm.data()->last_best.alt_times[i], 'f', 2));
                                        }
                                    }

                                    archive.data()->saveCase(tcase, recpt_fpm.data()->Id(), current_time);

                                    transf_item.data()->setNextOperation();
                                }else{
                                    transf_item.data()->send_me_to_as = 0;
                                    qDebug() << transf_item.data()->id() + 1 << "stored at" << c_log[j].dest() << "(time:" << current_time << ")";
                                }

                                if(c_log[j].dest() < as_count){
                                    transf_item.data()->current_pos = 0;
                                }else{
                                    transf_item.data()->current_pos = c_log[j].dest();
                                }

                                transf_item.data()->on_the_way = false;
                            }else{
                                qDebug() << "Bingo?" << transf_item.data()->id();
                            }
                            c_log[j].finish();
                        }else{
                            break;
                        }
                    }
                }
                if(c_log.last().isDone()){
                    busy_atms[i].data()->setStatus(FStatus::Idle);
                    free_atms.push_back(busy_atms[i]);
                    busy_atms.removeAt(i);
                }
            }

            if(!free_atms.size()){
                current_time += discrete;
                continue;
            }

        //FPMs preprocessing (for version with ATM's only)
            bool time_printed{false};

            QVector<QSharedPointer<FPM>> pfpms_to_handle;
            for(int i{0}; i < fpms.size(); ++i){
                if(fpms[i].data()->status() == FStatus::InHopeForFreedom){
                    int iiid = fpms[i].data()->currentItem().data()->nextFPMid() - as_count;
                    if(iiid >= 0){
                        if(fpms[iiid].data()->status() != FStatus::Idle){
                            fpms[i].data()->currentItem().data()->send_me_to_as = as_count;
                        }
                    }
                    pfpms_to_handle.push_back(fpms[i]);
                    working = true;
                }
            }

            QVector<QSharedPointer<Item>> pbest_items;
            QVector<TSelectionResult> psels;
            for(int i{0}; i < pfpms_to_handle.size(); ++i){
                QSharedPointer<FPM> fake_fpm = QSharedPointer<FPM>(new FPM(*pfpms_to_handle[i].data()));
                fake_fpm.data()->addToQueue(pfpms_to_handle[i].data()->currentItem());
                TSelectionResult csel = current_rule.data()->selectItem(this, fake_fpm);
                pbest_items.push_back(pfpms_to_handle[i].data()->currentItem());
                psels.push_back(csel);
            }

            QVector<QVector<ATMSel>> patms_opps;
            for(int i{0}; i < pbest_items.size(); ++i){
                Item* c_item = pbest_items[i].data();
                int c_pos = c_item->current_pos;
                int n_pos = c_item->nextFPMid();
                QVector<ATMSel> c_opp = findAllAtmsTimesToMoveFromToInList(c_pos, n_pos, free_atms);
                if(!c_opp.size()){
                    patms_opps.push_back(QVector<ATMSel>());
                    continue;
                }
                patms_opps.push_back(c_opp);
            }

            bool absolutely_empty{true};
            for(int i{0}; i < patms_opps.size(); ++i){
                if(patms_opps[i].size()){
                    absolutely_empty = false;
                    break;
                }
            }

            //Запис "верхньої частини" портфелю АТМів
            if(!absolutely_empty){
                for(int i{0}; i < patms_opps.size(); ++i){
                    if(!time_printed){
                        atm_cases.write(crow, 1, QString::number(current_time));
                        time_printed = true;
                    }
                    atm_cases.write(crow + i, 2, QString::number(pbest_items[i].data()->id()));
                    for(int j{0}; j < patms_opps[i].size(); ++j){
                        int c_ts = patms_opps[i][j].used_ts;
                        if(c_ts < 2){
                            atm_cases.write(crow + i, 3 + patms_opps[i][j].used_atms.first().data()->Id(), QString::number(patms_opps[i][j].time_for_all_moves));
                        }else{
                            int f_id = patms_opps[i][j].used_atms.first().data()->Id();
                            int s_id = patms_opps[i][j].used_atms.last().data()->Id();
                            if(c_ts == 3){ //З кільця на лінію
                                atm_cases.write(crow + i, 3 + atms_count + f_id, QString::number(patms_opps[i][j].time_for_all_moves));
                            }else{ //З лінії на кільце
                                atm_cases.write(crow + i, 3 + atms_count * 2 - 1 + s_id, QString::number(patms_opps[i][j].time_for_all_moves));
                            }
                        }
                    }
                }
                crow += patms_opps.size();
            }

            QVector<QVector<ATMSel>> patms_opps_copy = patms_opps;
            while(patms_opps.size()){
                if(!free_atms.size()){
                    break;
                }
                if(absolutely_empty){
                    break;
                }
                auto min_o = std::min_element(patms_opps.constBegin(), patms_opps.constEnd(), cmp_opps_outer);
                const ATMSel *min_i = std::min_element(min_o->constBegin(), min_o->constEnd(), cmp_opps);

                {
                    int row_d = patms_opps_copy.indexOf(*min_o) - patms_opps_copy.size();
                    int c_ts = min_i->used_ts;
                    if(c_ts < 2){
                        QString ostr = atm_cases.read(crow + row_d, 3 + min_i->used_atms.first().data()->Id()).toString();
                        QXlsx::Format bold;
                        bold.setFontBold(true);
                        atm_cases.write(crow + row_d, 3 + min_i->used_atms.first().data()->Id(), ostr, bold);
                    }else{
                        int f_id = min_i->used_atms.first().data()->Id();
                        int s_id = min_i->used_atms.last().data()->Id();
                        QXlsx::Format bold;
                        bold.setFontBold(true);
                        if(c_ts == 3){ //З кільця на лінію
                            QString ostr = atm_cases.read(crow + row_d,  3 + atms_count + f_id).toString();
                            atm_cases.write(crow + row_d, 3 + atms_count + f_id, ostr, bold);
                        }else{ //З лінії на кільце
                            QString ostr = atm_cases.read(crow + row_d,  3 + atms_count * 2 - 1 + s_id).toString();
                            atm_cases.write(crow + row_d, 3 + atms_count * 2 - 1 + s_id, ostr, bold);
                        }
                    }
                }

                QSharedPointer<Item> t_item = pbest_items[patms_opps.indexOf(*min_o)];
                int c_pos = t_item.data()->current_pos;
                int n_pos = t_item.data()->nextFPMid();
                int c_ts = min_i->used_ts;

                QVector<QSharedPointer<ATM>> to_del = min_i->used_atms;

                t_item.data()->setFactory(min_i->used_atms.last());
                double move_start_time = current_time;//- time_to_move_to_from[min_i->used_atms.first().data()->tId()][n_pos][c_pos];
                //move_start_time += pfpms_to_handle[patms_opps.indexOf(*min_o)].data()->last_delay;
                //double move_start_time = current_time + 0.1 - time_to_move_to_from[transports.first().data()->tId()][n_pos][c_pos];
                if(move_start_time < 0){
                    move_start_time = 0;
                }

                if(min_i->used_atms.first().data()->getLog().size()){
                    pfpms_to_handle[patms_opps.indexOf(*min_o)].data()->last_delay = std::max(0.0, min_i->used_atms.first().data()->getLog().last().end() - current_time);
                }else{
                    pfpms_to_handle[patms_opps.indexOf(*min_o)].data()->last_delay = 0;
                }

                pfpms_to_handle[patms_opps.indexOf(*min_o)].data()->last_delay += time_to_pos_to_from[min_i->used_atms.first().data()->tId()][c_pos][min_i->used_atms.first().data()->cPos()] + time_to_unload + time_to_take_place;


                if(c_ts < 2){
                    min_i->used_atms.first().data()->transportToNextPos(t_item, this, move_start_time, c_ts);
                }else{
                    int intersect = move_intersections[c_ts - 2][n_pos][c_pos];
                    double rollover_time = min_i->used_atms.first().data()->haulFromTo(t_item.data()->id(), c_pos, intersect, this, move_start_time, t_item.data()->currentOp());
                    reloader.data()->getLog().push_back(LogEntry(rollover_time, 1, t_item.data()->id(), t_item.data()->currentOp()));
                    double second_atm_start_time = rollover_time;// - time_to_pos_to_from[transports.last().data()->tId()][intersect][transports.last().data()->cPos()];
                    min_i->used_atms.last().data()->deliverFromTo(t_item.data()->id(), intersect, n_pos, this, second_atm_start_time, t_item.data()->currentOp());

                    min_i->used_atms.first().data()->getLog().last().cross_move = 1;
                    min_i->used_atms.last().data()->getLog().last().cross_move = 2;
                }

                t_item.data()->on_the_way = true;
                //c_fpm.data()->addToQueue(c_item);
                int ind = patms_opps.indexOf(*min_o);
                pfpms_to_handle[ind].data()->setStatus(FStatus::Idle);

                if(t_item.data()->nextFPMid() - as_count >= 0){
                    fpms[t_item.data()->nextFPMid() - as_count].data()->setStatus(FStatus::WaitingForSupply);
                    fpms[t_item.data()->nextFPMid() - as_count].data()->last_wait_for_supply = current_time;
                    fpms[t_item.data()->nextFPMid() - as_count].data()->last_best = psels[ind];
                }

                //видалення "відправлених" деталей із черги на обслуговування та видалення АТМів, що стали зайнятими
                pbest_items.removeAt(ind);
                pfpms_to_handle.removeAt(ind);
                patms_opps.removeOne(*min_o);
                for(auto &it : to_del){
                    free_atms.removeOne(it);
                    for(int i{patms_opps.size() - 1}; i >= 0; --i){
                        for(int j{patms_opps[i].size() - 1}; j >= 0; --j){
                            if(patms_opps[i][j].used_atms.contains(it)){
                                patms_opps[i].removeAt(j);
                            }
                        }
                        if(!patms_opps[i].size()){
                            patms_opps.removeAt(i);
                        }
                    }
                }
            }

            //JUST A COPY
            if(!free_atms.size()){
                current_time += discrete;
                continue;
            }

        //FPMs preprocessing (for version with ATM's only)
            QVector<QSharedPointer<FPM>> fpms_to_handle;
            for(int i{0}; i < fpms.size(); ++i){
                if(fpms[i].data()->status() == FStatus::Idle && fpms[i].data()->queueSize()){
                    fpms_to_handle.push_back(fpms[i]);
                    working = true;
                }
            }

            QVector<QSharedPointer<Item>> best_items;
            QVector<TSelectionResult> sels;
            for(int i{fpms_to_handle.size() - 1}; i >= 0; --i){
                TSelectionResult csel = current_rule.data()->selectItem(this, fpms_to_handle[i]);
                if(csel.relative_item_id == -1){
                    fpms_to_handle.removeAt(i);
                    continue;
                }
                best_items.push_front((*fpms_to_handle[i].data())[csel.relative_item_id]);
                sels.push_front(csel);
            }

            QVector<QVector<ATMSel>> atms_opps;
            for(int i{0}; i < best_items.size(); ++i){
                Item* c_item = best_items[i].data();
                int c_pos = c_item->current_pos;
                int n_pos = c_item->nextFPMid();
                QVector<ATMSel> c_opp = findAllAtmsTimesToMoveFromToInList(c_pos, n_pos, free_atms);
                if(!c_opp.size()){
                    atms_opps.push_back(QVector<ATMSel>());
                    continue;
                }
                atms_opps.push_back(c_opp);
            }

            absolutely_empty = true;
            for(int i{0}; i < atms_opps.size(); ++i){
                if(atms_opps[i].size()){
                    absolutely_empty = false;
                    break;
                }
            }



            //Запис "нижньої частини" портфелю АТМів
            if(!absolutely_empty){
                if(time_printed){
                    int mmmax;
                    if(ts_count == 2){
                        mmmax = atms_count * 3;
                    }else{
                        mmmax = atms_count + 2;
                    }
                    for(int i{1}; i <= mmmax; ++i){
                        atm_cases.write(crow, i, QString("---"));
                    }
                    atm_cases.setRowHeight(crow, crow, 5.0);
                    ++crow;
                }

                for(int i{0}; i < atms_opps.size(); ++i){
                    if(!time_printed){
                        atm_cases.write(crow, 1, QString::number(current_time));
                        time_printed = true;
                    }
                    atm_cases.write(crow + i, 2, QString::number(best_items[i].data()->id()));
                    atm_cases.write(crow + i, 2, QString::number(best_items[i].data()->id()));
                    for(int j{0}; j < atms_opps[i].size(); ++j){
                        int c_ts = atms_opps[i][j].used_ts;
                        if(c_ts < 2){
                            atm_cases.write(crow + i, 3 + atms_opps[i][j].used_atms.first().data()->Id(), QString::number(atms_opps[i][j].time_for_all_moves));
                        }else{
                            int f_id = atms_opps[i][j].used_atms.first().data()->Id();
                            int s_id = atms_opps[i][j].used_atms.last().data()->Id();
                            if(c_ts == 3){ //З кільця на лінію
                                atm_cases.write(crow + i, 3 + atms_count + f_id, QString::number(atms_opps[i][j].time_for_all_moves));
                            }else{ //З лінії на кільце
                                atm_cases.write(crow + i, 3 + atms_count * 2 - 1 + s_id, QString::number(atms_opps[i][j].time_for_all_moves));
                            }
                        }
                    }
                }
                crow += atms_opps.size();
            }

            QVector<QVector<ATMSel>> atms_opps_copy = atms_opps;
            while(atms_opps.size()){
                if(!free_atms.size()){
                    break;
                }
                if(absolutely_empty){
                    break;
                }

                auto min_o = std::min_element(atms_opps.constBegin(), atms_opps.constEnd(), cmp_opps_outer);
                const ATMSel *min_i = std::min_element(min_o->constBegin(), min_o->constEnd(), cmp_opps);

                {
                    int row_d = atms_opps_copy.indexOf(*min_o) - atms_opps_copy.size();
                    int c_ts = min_i->used_ts;
                    if(c_ts < 2){
                        QString ostr = atm_cases.read(crow + row_d, 3 + min_i->used_atms.first().data()->Id()).toString();
                        QXlsx::Format bold;
                        bold.setFontBold(true);
                        atm_cases.write(crow + row_d, 3 + min_i->used_atms.first().data()->Id(), ostr, bold);
                    }else{
                        int f_id = min_i->used_atms.first().data()->Id();
                        int s_id = min_i->used_atms.last().data()->Id();
                        QXlsx::Format bold;
                        bold.setFontBold(true);
                        if(c_ts == 3){ //З кільця на лінію
                            QString ostr = atm_cases.read(crow + row_d,  3 + atms_count + f_id).toString();
                            atm_cases.write(crow + row_d, 3 + atms_count + f_id, ostr, bold);
                        }else{ //З лінії на кільце
                            QString ostr = atm_cases.read(crow + row_d,  3 + atms_count * 2 - 1 + s_id).toString();
                            atm_cases.write(crow + row_d, 3 + atms_count * 2 - 1 + s_id, ostr, bold);
                        }
                    }
                }

                QSharedPointer<Item> t_item = best_items[atms_opps.indexOf(*min_o)];
                t_item.data()->send_me_to_as = 0;
                int c_pos = t_item.data()->current_pos;
                int n_pos = t_item.data()->nextFPMid();
                int c_ts = min_i->used_ts;

                QVector<QSharedPointer<ATM>> to_del = min_i->used_atms;

                t_item.data()->setFactory(min_i->used_atms.last());
                double move_start_time = current_time - time_to_move_to_from[min_i->used_atms.first().data()->tId()][n_pos][c_pos];
                move_start_time += fpms_to_handle[atms_opps.indexOf(*min_o)].data()->last_delay;
                //double move_start_time = current_time + 0.1 - time_to_move_to_from[transports.first().data()->tId()][n_pos][c_pos];
                if(move_start_time < 0){
                    move_start_time = 0;
                }

                QSharedPointer<FPM> prev_fpm;
                if(c_pos > as_count - 1){
                    prev_fpm = fpms[c_pos - as_count];
                }

                if(prev_fpm.data() != nullptr){
                    prev_fpm.data()->last_delay = std::max(0.0, min_i->used_atms.first().data()->getLog().last().end() - current_time);
                    prev_fpm.data()->last_delay += time_to_pos_to_from[min_i->used_atms.first().data()->tId()][c_pos][min_i->used_atms.first().data()->cPos()] + time_to_unload + time_to_take_place;
                }

                if(c_ts < 2){
                    min_i->used_atms.first().data()->transportToNextPos(t_item, this, move_start_time, c_ts);
                }else{
                    int intersect = move_intersections[c_ts - 2][n_pos][c_pos];
                    double rollover_time = min_i->used_atms.first().data()->haulFromTo(t_item.data()->id(), c_pos, intersect, this, move_start_time, t_item.data()->currentOp());
                    reloader.data()->getLog().push_back(LogEntry(rollover_time, 1, t_item.data()->id(), t_item.data()->currentOp()));
                    double second_atm_start_time = rollover_time;// - time_to_pos_to_from[transports.last().data()->tId()][intersect][transports.last().data()->cPos()];
                    min_i->used_atms.last().data()->deliverFromTo(t_item.data()->id(), intersect, n_pos, this, second_atm_start_time, t_item.data()->currentOp());

                    min_i->used_atms.first().data()->getLog().last().cross_move = 1;
                    min_i->used_atms.last().data()->getLog().last().cross_move = 2;
                }


                t_item.data()->on_the_way = true;
                //c_fpm.data()->addToQueue(c_item);
                int ind = atms_opps.indexOf(*min_o);
                fpms_to_handle[ind].data()->setStatus(FStatus::WaitingForSupply);
                fpms_to_handle[ind].data()->last_wait_for_supply = current_time;
                fpms_to_handle[ind].data()->last_best = sels[ind];
                fpms_to_handle.removeAt(ind);
                best_items.removeAt(ind);

                //видалення "відправлених" деталей із черги на обслуговування та видалення АТМів, що стали зайнятими
                atms_opps.removeOne(*min_o);
                for(auto &it : to_del){
                    free_atms.removeOne(it);
                    for(int i{atms_opps.size() - 1}; i >= 0; --i){
                        for(int j{atms_opps[i].size() - 1}; j >= 0; --j){
                            if(atms_opps[i][j].used_atms.contains(it)){
                                atms_opps[i].removeAt(j);
                            }
                        }
                        if(!atms_opps[i].size()){
                            atms_opps.removeAt(i);
                        }
                    }
                    for(int i{atms_opps_copy.size() - 1}; i >= 0; --i){
                        for(int j{atms_opps_copy[i].size() - 1}; j >= 0; --j){
                            if(atms_opps_copy[i][j].used_atms.contains(it)){
                                atms_opps_copy[i].removeAt(j);
                            }
                        }
                    }
                }
            }

            if(!free_atms.size()){
                current_time += discrete;
                continue;
            }
        }

        //FPMs processing
        for(int i{0}; i < fpms.size(); ++i){
            QSharedPointer<FPM> c_fpm = fpms[i];
            switch(c_fpm.data()->status()){
            case FStatus::Idle:
            {
                if(!no_atm || !c_fpm.data()->queueSize()){
                    continue;
                }
                working = true;

                TSelectionResult sel = current_rule.data()->selectItem(this, c_fpm);
                int best_item = sel.relative_item_id;

                QSharedPointer<Item> c_item = (*c_fpm.data())[best_item];
                c_fpm.data()->selectFromQueue(best_item);
                c_fpm.data()->setStatus(FStatus::Busy);
                c_fpm.data()->getLog().push_back(LogEntry(current_time, c_item.data()->nextFPMtime(), c_item.data()->id(), c_item.data()->currentOp()));

                Case tcase;
                tcase.string_rep += "[" + QString::number(sel.absolute_item_id + 1) + "] ";
                tcase.times_strs.push_back(QString::number(sel.best_time, 'f', 2));
                if(sel.alt_calculated){
                    tcase.times_strs.last().push_back(" - " + QString::number(sel.alt_time, 'f', 2));
                }
                for(int i{0}; i < sel.relative_candidates.size(); ++i){
                    if(sel.relative_candidates[i] == sel.relative_item_id){
                        continue;
                    }
                    tcase.string_rep += QString::number(sel.absolute_candidates[i] + 1) + " ";
                    tcase.times_strs.push_back(QString::number(sel.candidates_times[i], 'f', 2));
                    if(sel.alt_calculated && sel.best_time == sel.candidates_times[i]){
                        tcase.times_strs.last().push_back(" - " + QString::number(sel.alt_times[i], 'f', 2));
                    }
                }

                archive.data()->saveCase(tcase, c_fpm.data()->Id(), current_time);

                c_item.data()->setFactory(c_fpm);
                c_item.data()->current_pos = c_item.data()->nextFPMid();
                c_item.data()->setNextOperation();
            }
            break;

            case FStatus::Busy:
            {
                working = true;
                if(current_time < c_fpm.data()->getLog().last().end()){
                    continue;
                }

                QSharedPointer<Item> &c_item = c_fpm.data()->currentItem();
                if(c_item.data()->nextFPMid() > as_count - 1){
                    if(!no_atm){
                        QSharedPointer<FPM> n_fpm = fpms[c_item.data()->nextFPMid() - as_count];
                        if(n_fpm.data()->status() == FStatus::Idle){
                            n_fpm.data()->addToQueue(c_item);

                            c_fpm.data()->setStatus(FStatus::InHopeForFreedom);

                        }else{
                            goto nah;
                        }
                    }else{
                        QSharedPointer<FPM> n_fpm = fpms[c_item.data()->nextFPMid() - as_count];
                        n_fpm.data()->addToQueue(c_item);
                    }
                }else{
                    if(!no_atm){
                        nah:
                        if(c_item.data()->nextFPMid()){
                            fpms[c_item.data()->nextFPMid() - as_count].data()->addToQueue(c_item);
                        }

                        c_item.data()->send_me_to_as = as_count;
                        c_fpm.data()->setStatus(FStatus::InHopeForFreedom);

                        if(c_item.data()->nextFPMid() > as_count - 1){
                            fpms[c_item.data()->nextFPMid() - as_count].data()->addToQueue(c_item);
                        }
                    }else{
                        QSharedPointer<BaseFactory> nptr;
                        c_item.data()->setFactory(nptr);
                    }
                }

                c_fpm.data()->getLog().last().finish();

                if(c_fpm.data()->status() == FStatus::Busy){
                    QSharedPointer<Item> nptr;
                    c_fpm.data()->setCurrent(nptr);
                    c_fpm.data()->setStatus(FStatus::Idle);
                }
                if(!c_fpm.data()->queueSize()){
                    const Case null_case = {{}, {}, "\0"};
                    archive.data()->saveCase(null_case, c_fpm.data()->Id(), current_time);
                    continue;
                }
            }
            break;

            case FStatus::WaitingForSupply:
            {
                working = true;
                //do nothing
            }
            break;

            case FStatus::InHopeForFreedom:
            {
                working = true;
                //Also do nothing, just wait
            }
            break;

            default:
            qDebug() << "Eeeh? Forget about status?";
            break;
            }
        }

        current_time += discrete;
    }while(working);
    atm_cases.saveAs(XlsxNames[cur_file_id] + QString("_atms_case.xlsx"));
    ++cur_file_id;
}

double DataStorage::timeToPosTF(int to, int from, int ts)
{
    return time_to_pos_to_from[ts][to][from];
}

double DataStorage::timeToMoveTF(int to, int from, int ts)
{
    return time_to_move_to_from[ts][to][from];
}

//Знаходження (пари) АТМ, для найшвидшшого пункту переміщення з пункту А в пункт Б
QVector<QSharedPointer<ATM> > DataStorage::findBestAtmsToMoveFromTo(int from, int to, double *ret_time, int *ret_ts)
{
    return findBestAtmsToMoveFromToInList(from, to, ret_time, ret_ts, atms);
}

QVector<QSharedPointer<ATM> > DataStorage::findBestAtmsToMoveFromToInList(int from, int to, double *ret_time, int *ret_ts, QVector<QSharedPointer<ATM> > &container)
{
    QVector<QSharedPointer<ATM> > ret;
    double min_move_time{INT_MAX};
    for(int cts{0}; cts < time_to_move_to_from.size(); ++cts){
        if(!time_to_move_to_from[cts][to][from]){
            continue;
        }
        double base_time = time_to_move_to_from[cts][to][from];
        if(cts < 2){
            QVector<QSharedPointer<ATM> > qualified_atms;
            for(QSharedPointer<ATM> &it : container){
                if(it.data()->tId() == cts && it.data()->status() == FStatus::Idle){
                    qualified_atms.push_back(it);
                }
            }
            if(!qualified_atms.size()){
                continue;
            }

            double min_pos_time{INT_MAX};
            int sel_atm{0};
            for(int a_num{0}; a_num < qualified_atms.size(); ++a_num){
                if(qualified_atms[a_num].data()->tId() != cts){
                    continue;
                }
                double c_time = time_to_pos_to_from[cts][from][qualified_atms[a_num].data()->cPos()];
                if(c_time < min_pos_time){
                    min_pos_time = c_time;
                    sel_atm = a_num;
                }
            }
            if(base_time + min_pos_time < min_move_time){
                min_move_time = base_time + min_pos_time;
                ret.clear();
                ret.push_back(qualified_atms[sel_atm]);
                *ret_ts = cts;
            }
        }else{
            QVector<QSharedPointer<ATM> > qualified_circle_atms;
            QVector<QSharedPointer<ATM> > qualified_line_atms;
            for(QSharedPointer<ATM> &it : container){
                if(it.data()->tId() == 0 && it.data()->status() == FStatus::Idle){
                    qualified_circle_atms.push_back(it);
                }else if(it.data()->tId() == 1 && it.data()->status() == FStatus::Idle){
                    qualified_line_atms.push_back(it);
                }
            }
            if(!qualified_circle_atms.size() || !qualified_line_atms.size()){
                continue;
            }
            double min_pos_time{INT_MAX};
            double min_atm_time_circle{INT_MAX};
            int atm_circle{0};
            double min_atm_time_line{INT_MAX};
            int atm_line{0};
            int intersect = move_intersections[cts-2][to][from];

            if(cts == 3){//З кільця на лінію
                for(int a_num{0}; a_num < qualified_circle_atms.size(); ++a_num){
                    double c_time = time_to_pos_to_from[0][from][qualified_circle_atms[a_num].data()->cPos()];
                    if(c_time && c_time < min_atm_time_circle){
                        min_atm_time_circle = c_time;
                        atm_circle = a_num;
                    }
                }
                for(int a_num{0}; a_num < qualified_line_atms.size(); ++a_num){
                    double c_time = time_to_pos_to_from[1][intersect][qualified_line_atms[a_num].data()->cPos()];
                    c_time -= time_to_move_to_from[0][intersect][from];
                    c_time -= time_to_take_place;
                    if(intersect > as_count - 1){
                        c_time += time_to_load;
                    }
                    if(c_time && c_time < min_atm_time_line){
                        min_atm_time_line = c_time;
                        atm_line = a_num;
                    }
                }

                min_pos_time = min_atm_time_circle < min_atm_time_line
                        ? min_atm_time_line
                        : min_atm_time_circle;

            }else{//2 - з лінії на кільце
                for(int a_num{0}; a_num < qualified_line_atms.size(); ++a_num){
                    double c_time = time_to_pos_to_from[1][from][qualified_line_atms[a_num].data()->cPos()];
                    if(c_time < min_atm_time_line){
                        min_atm_time_line = c_time;
                        atm_line = a_num;
                    }
                }
                for(int a_num{0}; a_num < qualified_circle_atms.size(); ++a_num){
                    double c_time = time_to_pos_to_from[0][intersect][qualified_circle_atms[a_num].data()->cPos()];
                    c_time -= time_to_move_to_from[1][intersect][from];
                    c_time -= time_to_take_place;
                    if(intersect > as_count - 1){
                        c_time += time_to_load;
                    }
                    if(c_time < 0){
                        c_time = 0;
                    }
                    if(c_time < min_atm_time_circle){
                        min_atm_time_circle = c_time;
                        atm_circle = a_num;
                    }
                }

                min_pos_time = min_atm_time_circle < min_atm_time_line
                        ? min_atm_time_line
                        : min_atm_time_circle;

            }

            if(base_time + min_pos_time < min_move_time){
                min_move_time = base_time + min_pos_time;
                ret.clear();
                if(cts == 3){
                    ret.push_back(qualified_circle_atms[atm_circle]);
                    ret.push_back(qualified_line_atms[atm_line]);
                }else{
                    ret.push_back(qualified_line_atms[atm_line]);
                    ret.push_back(qualified_circle_atms[atm_circle]);
                }
                *ret_ts = cts;
            }
        }
    }
    *ret_time = min_move_time;
    return ret;
}

QVector<ATMSel> DataStorage::findAllAtmsTimesToMoveFromToInList(int from, int to, QVector<QSharedPointer<ATM> > &container)
{
    QVector<ATMSel> ret;

    for(int i{0}; i < container.size(); ++i){
        QSharedPointer<ATM> c_atm = container[i];
        int c_ts = c_atm.data()->tId();
        double c_time = time_to_move_to_from[c_ts][to][from];
        if(!c_time){
            continue;
        }

        c_time += time_to_pos_to_from[c_ts][from][c_atm.data()->cPos()];

        ATMSel c_sel;;
        c_sel.used_atms.push_back(c_atm);
        c_sel.used_ts = c_ts;
        c_sel.time_for_all_moves = c_time;
        ret.push_back(c_sel);
    }

    if(ts_count > 1){
        QVector<QSharedPointer<ATM> > qualified_circle_atms;
        QVector<QSharedPointer<ATM> > qualified_line_atms;
        for(QSharedPointer<ATM> &it : container){
            if(it.data()->tId() == 0 && it.data()->status() == FStatus::Idle){
                qualified_circle_atms.push_back(it);
            }else if(it.data()->tId() == 1 && it.data()->status() == FStatus::Idle){
                qualified_line_atms.push_back(it);
            }
        }

        if(!qualified_circle_atms.size() || !qualified_line_atms.size()){
            return ret;
        }

        for(int c_ts{2}; c_ts < time_to_move_to_from.size(); ++c_ts){
            double c_time = time_to_move_to_from[c_ts][to][from];
            if(!c_time){
                continue;
            }

            int intersect = move_intersections[c_ts-2][to][from];

            if(c_ts == 3){//З кільця на лінію
                for(int a_num{0}; a_num < qualified_circle_atms.size(); ++a_num){
                    double circle_time = time_to_pos_to_from[0][from][qualified_circle_atms[a_num].data()->cPos()];
                    for(int b_num{0}; b_num < qualified_line_atms.size(); ++b_num){
                        double line_time = time_to_pos_to_from[1][intersect][qualified_line_atms[b_num].data()->cPos()];
                        line_time -= time_to_move_to_from[0][intersect][from];
                        line_time -= time_to_take_place;
                        if(intersect > as_count - 1){
                            line_time += time_to_load;
                        }
                        if(line_time < 0){
                            line_time = 0;
                        }
                        double all_move_time = circle_time < line_time
                                ? line_time
                                : circle_time;

                        all_move_time += c_time;

                        ATMSel c_sel;;
                        c_sel.used_atms.push_back(qualified_circle_atms[a_num]);
                        c_sel.used_atms.push_back(qualified_line_atms[b_num]);
                        c_sel.used_ts = c_ts;
                        c_sel.time_for_all_moves = all_move_time;
                        ret.push_back(c_sel);
                    }
                }
            }else{//2 - з лінії на кільце
                for(int a_num{0}; a_num < qualified_line_atms.size(); ++a_num){
                    double line_time = time_to_pos_to_from[1][from][qualified_line_atms[a_num].data()->cPos()];
                    for(int b_num{0}; b_num < qualified_circle_atms.size(); ++b_num){
                        double circle_time = time_to_pos_to_from[0][intersect][qualified_circle_atms[b_num].data()->cPos()];
                        circle_time -= time_to_move_to_from[1][intersect][from];
                        circle_time -= time_to_take_place;
                        if(intersect > as_count - 1){
                            circle_time += time_to_load;
                        }
                        if(circle_time < 0){
                            circle_time = 0;
                        }

                        double all_move_time = circle_time < line_time
                                ? line_time
                                : circle_time;

                        all_move_time += c_time;

                        ATMSel c_sel;;
                        c_sel.used_atms.push_back(qualified_line_atms[a_num]);
                        c_sel.used_atms.push_back(qualified_circle_atms[b_num]);
                        c_sel.used_ts = c_ts;
                        c_sel.time_for_all_moves = all_move_time;
                        ret.push_back(c_sel);
                    }
                }
            }
        }
    }

    return ret;
}

double DataStorage::loadTime()
{
    return time_to_load;
}

double DataStorage::unloadTime()
{
    return time_to_unload;
}

int DataStorage::asCount()
{
    return as_count;
}

void DataStorage::disable_atms()
{
    no_atm = true;
}

template<typename T>
bool DataStorage::readNumber(QTextStream &stream, T &numvar, int line, bool zero_is_valid)
{
    stream >> numvar;
    if(stream.atEnd()){
        ErrorsHandler::outErr(ErrorCode::UNEXPECTED_END_OF_FILE, line);
        return false;
    }
    if(!zero_is_valid && !numvar){
        ErrorsHandler::outErr(ErrorCode::PARSE_ERROR, line);
        return false;
    }
    return true;
}
