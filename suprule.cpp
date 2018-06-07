#include <QVector>
#include <climits>
#include "suprule.h"
#include "datastorage.h"

#include <QDebug>

SupRule::SupRule()
{

}

SupRule::~SupRule()
{

}

int FirstRule::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr)
{
    int ret{-1};
    int min{INT_MAX};
    if(ptr.data() != nullptr){
        for(int i{0}; i < ptr.data()->queueSize(); ++i){
            QSharedPointer<Item> c_item = (*ptr.data())[i];
            if(c_item.data()->on_the_way){
                continue;
            }
            if(c_item.data()->nextFPMtime() < min){
                min = c_item.data()->nextFPMtime();
                ret = i;
            }
        }
    }else{
        for(int i{0}; i < data->items.size(); ++i){
            if(data->items[i].data()->factory() != nullptr){
                continue;
            }
            double min_move_time;
            int to = data->items[i].data()->nextFPMid();
            if(!to){
                qDebug() << "DAMNIT! (firstrule, selectitem)";
            }
            int from = 0;
            int dumb;
            data->findBestAtmsToMoveFromTo(from, to, &min_move_time, &dumb);
            double total_time = data->items[i].data()->nextFPMtime() + min_move_time;
            if(total_time < min){
                min = total_time;
                ret = i;
            }
        }
    }
    return ret;
}

int SecondRule::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr)
{
    int ret{-1};
    int max{0};
    if(ptr.data() != nullptr){
        for(int i{0}; i < ptr.data()->queueSize(); ++i){
            QSharedPointer<Item> c_item = (*ptr.data())[i];
            if(c_item.data()->on_the_way){
                continue;
            }
            double time = c_item.data()->sumOfFPMtimes(c_item.data()->currentOp() + 1);
            if(time > max){
                max = time;
                ret = i;
            }
        }
    }else{
        for(int i{0}; i < data->items.size(); ++i){
            if(data->items[i].data()->factory() != nullptr){
                continue;
            }
            double min_move_time;
            int to = data->items[i].data()->nextFPMid();
            if(!to){
                qDebug() << "DAMNIT! (secondrule, selectitem)";
            }
            int from = 0;
            int dumb;
            data->findBestAtmsToMoveFromTo(from, to, &min_move_time, &dumb);
            double total_time = data->items[i].data()->sumOfFPMtimes(0) + min_move_time;
            if(total_time > max){
                max = total_time;
                ret = i;
            }
        }
    }
    return ret;
}

int FourthRule::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr)
{
    int ret{-1};
    int min{INT_MAX};
    if(ptr.data() != nullptr){
        for(int i{0}; i < ptr.data()->queueSize(); ++i){
            QSharedPointer<Item> c_item = (*ptr.data())[i];
            if(c_item.data()->on_the_way){
                continue;
            }
            double time = c_item.data()->sumOfFPMtimes(c_item.data()->currentOp() + 1);
            if(time < min){
                min = time;
                ret = i;
            }
        }
    }else{
        for(int i{0}; i < data->items.size(); ++i){
            if(data->items[i].data()->factory() != nullptr){
                continue;
            }
            double min_move_time;
            int to = data->items[i].data()->nextFPMid();
            if(!to){
                qDebug() << "DAMNIT! (secondrule, selectitem)";
            }
            int from = 0;
            int dumb;
            data->findBestAtmsToMoveFromTo(from, to, &min_move_time, &dumb);
            double total_time = data->items[i].data()->sumOfFPMtimes(0) + min_move_time;
            if(total_time < min){
                min = total_time;
                ret = i;
            }
        }
    }
    return ret;
}

int FifthRule::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr)
{
    int ret{-1};
    int max{0};
    if(ptr.data() != nullptr){
        for(int i{0}; i < ptr.data()->queueSize(); ++i){
            QSharedPointer<Item> c_item = (*ptr.data())[i];
            if(c_item.data()->on_the_way){
                continue;
            }
            if(c_item.data()->nextFPMtime() > max){
                max = c_item.data()->nextFPMtime();
                ret = i;
            }
        }
    }else{
        for(int i{0}; i < data->items.size(); ++i){
            if(data->items[i].data()->factory() != nullptr){
                continue;
            }
            double min_move_time;
            int to = data->items[i].data()->nextFPMid();
            if(!to){
                qDebug() << "DAMNIT! (firstrule, selectitem)";
            }
            int from = 0;
            int dumb;
            data->findBestAtmsToMoveFromTo(from, to, &min_move_time, &dumb);
            double total_time = data->items[i].data()->nextFPMtime() + min_move_time;
            if(total_time > max){
                max = total_time;
                ret = i;
            }
        }
    }
    return ret;
}

int ThirdReich::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr)
{
    int ret{-1};
    int min{INT_MAX};
    if(ptr.data() != nullptr){
        int low_priority_ret{-1};
        int low_priority_min{INT_MAX};
        for(int i{0}; i < ptr.data()->queueSize(); ++i){
            if((*ptr.data())[i].data()->on_the_way){
                continue;
            }
            int n_fpm = (*ptr.data())[i].data()->nextNextFPMid();
            double c_time{0};
            if(n_fpm){
                c_time = data->fpms[n_fpm - data->asCount()].data()->queueTime();
                if(c_time < min){
                    min = c_time;
                    ret = i;
                }
            }else{
                low_priority_min = 0;
                low_priority_ret = i;
            }
        }

        if(ret >= 0){
            return ret;
        }
        return low_priority_ret;
    }else{
        for(int i{0}; i < data->items_count; ++i){
            if(data->items[i].data()->factory() != nullptr){
                continue;
            }
            return i;
        }
    }
    return 0;
}

//FIFO
int SixthRule::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr)
{
    int ret{-1};
    if(ptr.data() != nullptr){
        for(int i{0}; i < ptr.data()->queueSize(); ++i){
            if((*ptr.data())[i].data()->on_the_way){
                continue;
            }
            return i;
        }
    }else{
        for(int i{0}; i < data->items_count; ++i){
            if(data->items[i].data()->factory() != nullptr){
                continue;
            }
            return i;
        }
    }
    return ret;
}

//LIFO
int SeventhRule::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr)
{
    if(ptr.data() != nullptr){
        for(int i{ptr.data()->queueSize() - 1}; i >= 0; --i){
            if((*ptr.data())[i].data()->on_the_way){
                continue;
            }
            return i;
        }
    }else{
        for(int i{data->items_count - 1}; i >= 0; --i){
            if(data->items[i].data()->factory() != nullptr){
                continue;
            }
            return i;
        }
    }
    return -1;
}
