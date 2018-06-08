#include <QFile>
#include <QTextStream>
#include <QStringList>

#include <QDebug>
#include <QApplication>

#include "datastorage.h"
#include "errorshandler.h"

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
                            double cross_time = time_to_pos_to_from[0][cross_pos][from] + time_to_pos_to_from[1][to][cross_pos] + time_to_take_place * 2;
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

}

void DataStorage::calculateThemAll()
{
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
                                    recpt_fpm.data()->getLog().push_back(LogEntry(current_time, transf_item.data()->nextFPMtime(), transf_item.data()->id()));
                                    recpt_fpm.data()->selectFromQueueById(transf_item.data()->id());

                                    QString tok;
                                    tok += "[" + QString::number(transf_item.data()->id() + 1) + "] ";
                                    for(int i{0}; i < recpt_fpm.data()->queueSize(); ++i){
                                        tok += QString::number(recpt_fpm.data()->queue[i].data()->id() + 1) + " ";
                                    }
                                    archive.data()->saveString(tok, recpt_fpm.data()->Id(), current_time);

                                    transf_item.data()->setNextOperation();
                                }else{
                                    qDebug() << transf_item.data()->id() + 1 << "stored at" << c_log[j].dest() << "(time:" << current_time << ")";
                                }
                                transf_item.data()->current_pos = c_log[j].dest();
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
        }

        //FPMs processing
        for(int i{0}; i < fpms.size(); ++i){
            QSharedPointer<FPM> c_fpm = fpms[i];
            switch(c_fpm.data()->status()){
            case FStatus::Idle:
            {
                if(!c_fpm.data()->queueSize()){
                    continue;
                }
                working = true;

                select_next_from_idle:
                c_fpm.data()->setStatus(FStatus::Idle);
                QVector<QSharedPointer<Item>> cqueue = c_fpm.data()->queue;
                bool fail{false};
                int best_item{0};
                do{
                    if(fail){
                        c_fpm.data()->queue.removeAt(best_item);
                        if(c_fpm.data()->queue.size() == 0){
                            break;
                        }
                        fail = false;
                    }

                    best_item = current_rule.data()->selectItem(this, c_fpm);
                    if(best_item == -1){
                        break;
                    }

                    QSharedPointer<Item> &c_item = (*c_fpm.data())[best_item];
                    if(!no_atm){
                        int c_pos = c_item.data()->current_pos;
                        int n_pos = c_item.data()->nextFPMid();
                        double dummy;
                        int c_ts;
                        QVector<QSharedPointer<ATM>> transports = findBestAtmsToMoveFromToInList(c_pos, n_pos, &dummy, &c_ts, free_atms);
                        if(!transports.size()){
                            fail = true;
                            continue;
                        }

                        c_item.data()->setFactory(transports.last());
                        //int iid = c_item.data()->id();
                        double move_start_time = current_time + 0.5 - time_to_move_to_from[transports.first().data()->tId()][n_pos][c_pos];
                        if(move_start_time < 0){
                            move_start_time = 0;
                        }

                        if(c_ts < 2){
                            transports.first().data()->transportToNextPos(c_item, this, move_start_time, c_ts);
                        }else{
                            int intersect = move_intersections[c_ts - 2][n_pos][c_pos];
                            double rollover_time = transports.first().data()->haulFromTo(c_item.data()->id(), c_pos, intersect, this, move_start_time);
                            reloader.data()->getLog().push_back(LogEntry(rollover_time, 1, c_item.data()->id()));
                            double second_atm_start_time = rollover_time;// - time_to_pos_to_from[transports.last().data()->tId()][intersect][transports.last().data()->cPos()];
                            transports.last().data()->deliverFromTo(c_item.data()->id(), intersect, n_pos, this, second_atm_start_time);

                            transports.first().data()->getLog().last().cross_move = 1;
                            transports.last().data()->getLog().last().cross_move = 2;
                        }

                        c_item.data()->on_the_way = true;
                        //c_fpm.data()->addToQueue(c_item);
                        c_fpm.data()->setStatus(FStatus::WaitingForSupply);
                    }else{
                        c_fpm.data()->selectFromQueue(best_item);
                        c_fpm.data()->setStatus(FStatus::Busy);
                        c_fpm.data()->getLog().push_back(LogEntry(current_time, c_item.data()->nextFPMtime(), c_item.data()->id()));

                        QString tok;
                        tok += "[" + QString::number(c_item.data()->id() + 1) + "] ";
                        for(int i{0}; i < c_fpm.data()->queueSize(); ++i){
                            tok += QString::number(c_fpm.data()->queue[i].data()->id() + 1) + " ";
                        }
                        archive.data()->saveString(tok, c_fpm.data()->Id(), current_time);

                        c_item.data()->setFactory(c_fpm);
                        c_item.data()->current_pos = c_item.data()->nextFPMid();
                        c_item.data()->setNextOperation();
                    }
                }while(fail);

                if(!no_atm){
                    c_fpm.data()->queue = cqueue;
                }
            }
            break;

            case FStatus::Busy:
            {
                working = true;
                if(current_time < c_fpm.data()->getLog().last().end()){
                    continue;
                }

                //int ctt = current_time;
                QSharedPointer<Item> &c_item = c_fpm.data()->currentItem();
                if(c_item.data()->nextFPMid()){
                    if(!no_atm){
                        int c_pos = c_item.data()->currentFPMid();
                        int n_pos = c_item.data()->nextFPMid();
                        QSharedPointer<FPM> n_fpm = fpms[n_pos - as_count];
                        if(n_fpm.data()->status() == FStatus::Idle){
                            double dummy;
                            int c_ts;
                            QVector<QSharedPointer<ATM>> transports = findBestAtmsToMoveFromToInList(c_pos, n_pos, &dummy, &c_ts, free_atms);
                            if(!transports.size()){
                                qDebug() << "DAMNIT! (calcthemall, FPMs processing #1.75)";
                                goto nah;
                            }else{
                                c_item.data()->setFactory(transports.last());
                                double move_start_time = current_time + 0.5;
                                if(move_start_time < 0){
                                    move_start_time = 0;
                                }

                                if(c_ts < 2){
                                    transports.first().data()->transportToNextPos(c_item, this, move_start_time, c_ts);
                                }else{
                                    int intersect = move_intersections[c_ts - 2][n_pos][c_pos];
                                    double rollover_time = transports.first().data()->haulFromTo(c_item.data()->id(), c_pos, intersect, this, move_start_time);
                                    reloader.data()->getLog().push_back(LogEntry(rollover_time, 1, c_item.data()->id()));
                                    double second_atm_start_time = rollover_time;// - time_to_pos_to_from[transports.last().data()->tId()][intersect][transports.last().data()->cPos()];
                                    transports.last().data()->deliverFromTo(c_item.data()->id(), intersect, n_pos, this, second_atm_start_time);

                                    transports.first().data()->getLog().last().cross_move = 1;
                                    transports.last().data()->getLog().last().cross_move = 2;
                                }

                                c_item.data()->on_the_way = true;
                                n_fpm->addToQueue(c_item);
                                n_fpm.data()->setStatus(FStatus::WaitingForSupply);
                            }
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
                        double best_time{INT_MAX};
                        QVector<QSharedPointer<ATM>> transports;
                        int c_pos = c_item.data()->currentFPMid();
                        int best_ts;
                        int best_dest;
                        for(int i{0}; i < as.size(); ++i){
                            double c_time;
                            int c_ts;
                            auto tr = findBestAtmsToMoveFromTo(c_pos, as[i].data()->id(), &c_time, &c_ts);
                            if(c_time < best_time && tr.size()){
                                best_time = c_time;
                                transports = tr;
                                best_ts = c_ts;
                                best_dest = i;
                            }
                        }
                        if(!transports.size()){
                            continue; //Good luck next time
                        }

                        c_item.data()->setFactory(transports.last());
                        double move_start_time = current_time + 0.5;
                        if(best_ts < 2){
                            transports.first().data()->haulFromTo(c_item.data()->id(), c_pos, as[best_dest].data()->id(), this, move_start_time);
                        }else{
                            int intersect = move_intersections[best_ts - 2][as[best_dest].data()->id()][c_pos];
                            double rollover_time = transports.first().data()->haulFromTo(c_item.data()->id(), c_pos, intersect, this, move_start_time);
                            reloader.data()->getLog().push_back(LogEntry(rollover_time, 1, c_item.data()->id()));
                            double second_atm_start_time = rollover_time;// - time_to_pos_to_from[transports.last().data()->tId()][intersect][transports.last().data()->cPos()];
                            transports.last().data()->deliverFromTo(c_item.data()->id(), intersect, as[best_dest].data()->id(), this, second_atm_start_time);

                            transports.first().data()->getLog().last().cross_move = 1;
                            transports.last().data()->getLog().last().cross_move = 2;
                        }

                        c_item.data()->on_the_way = true;
                        if(c_item.data()->nextFPMid()){
                            fpms[c_item.data()->nextFPMid()-as_count].data()->addToQueue(c_item);
                        }
                    }else{
                        QSharedPointer<BaseFactory> nptr;
                        c_item.data()->setFactory(nptr);
                    }
                }

                c_fpm.data()->getLog().last().finish();

                if(!c_fpm.data()->queueSize()){
                    QSharedPointer<Item> nptr;
                    c_fpm.data()->setCurrent(nptr);
                    c_fpm.data()->setStatus(FStatus::Idle);
                    archive.data()->saveString(QString(), c_fpm.data()->Id(), current_time);
                    continue;
                }else{
                    goto select_next_from_idle;
                }
            }
            break;

            case FStatus::WaitingForSupply:
            {
                //do nothing
            }
            break;

            default:
            qDebug() << "Eeeh? Forget about status?";
            break;
            }
        }

        current_time += discrete;
    }while(working);
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
