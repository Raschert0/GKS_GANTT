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
    (void)data;
    int ret{-1};
    int min{INT_MAX};
    for(int i{0}; i < ptr.data()->queueSize(); ++i){
        QSharedPointer<Item> c_item = (*ptr.data())[i];
        if(c_item.data()->on_the_way){
            continue;
        }
        if(c_item.data()->nextFPMtime() > 0 && c_item.data()->nextFPMtime() < min){
            min = c_item.data()->nextFPMtime();
            ret = i;
        }
    }
    return ret;
}

int SecondRule::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr)
{
    (void)data;
    int ret{-1};
    int max{0};
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
    return ret;
}

int FourthRule::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr)
{
    (void)data;
    int ret{-1};
    int min{INT_MAX};
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
    return ret;
}

int FifthRule::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr)
{
    (void)data;
    int ret{-1};
    int max{0};
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
    return ret;
}

int ThirdReich::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr)
{
    int ret{-1};
    int min{INT_MAX};
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
}

//FIFO
int SixthRule::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr)
{
    (void)data;
    int ret{-1};
    for(int i{0}; i < ptr.data()->queueSize(); ++i){
        if((*ptr.data())[i].data()->on_the_way){
            continue;
        }
        return i;
    }
    return ret;
}

//LIFO
int SeventhRule::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr)
{
    (void)data;
    for(int i{ptr.data()->queueSize() - 1}; i >= 0; --i){
        if((*ptr.data())[i].data()->on_the_way){
            continue;
        }
        return i;
    }
    return -1;
}
