#include "suprule.h"
#include "datastorage.h"
#include "fpm.h"
#include <QVector>
#include <climits>

#include <QDebug>

SupRule::SupRule()
{

}

SupRule::~SupRule()
{

}

TSelectionResult FirstRule::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr, bool is_alt)
{
    (void)data;
    TSelectionResult ret;
    double min{INT_MAX};
    for(int i{0}; i < ptr.data()->queueSize(); ++i){
        QSharedPointer<Item> c_item = (*ptr.data())[i];
        if(c_item.data()->on_the_way){
            continue;
        }

        ret.relative_candidates.push_back(i);
        ret.absolute_candidates.push_back(c_item.data()->id());
        ret.candidates_times.push_back(c_item.data()->nextFPMtime());

        if(c_item.data()->nextFPMtime() > 0 && c_item.data()->nextFPMtime() < min){
            min = c_item.data()->nextFPMtime();
            ret.relative_item_id = i;
            ret.absolute_item_id = c_item.data()->id();
            ret.best_time = min;
        }
    }
    if(!is_alt){
        if(ret.candidates_times.count(ret.best_time) > 1){
            FourthRule alt_rule;
            TSelectionResult alt_res = alt_rule.selectItem(data, ptr, true);
            ret.alt_times = alt_res.candidates_times;
            for(int i{0}; i < ret.relative_candidates.size(); ++i){
                if(ret.candidates_times[i] != ret.best_time){
                    continue;
                }
                if(!ret.alt_time || ret.alt_times[i] < ret.alt_time){
                    ret.relative_item_id = i;
                    ret.absolute_item_id = ret.absolute_candidates[i];
                    ret.alt_time = ret.alt_times[i];
                }
            }
            ret.alt_calculated = true;
        }
    }
    return ret;
}

TSelectionResult SecondRule::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr, bool is_alt)
{
    (void)data;
    TSelectionResult ret;
    double max{0};
    for(int i{0}; i < ptr.data()->queueSize(); ++i){
        QSharedPointer<Item> c_item = (*ptr.data())[i];
        if(c_item.data()->on_the_way){
            continue;
        }
        double time = c_item.data()->sumOfFPMtimes(c_item.data()->currentOp() + 1);

        ret.relative_candidates.push_back(i);
        ret.absolute_candidates.push_back(c_item.data()->id());
        ret.candidates_times.push_back(time);

        if(time > max){
            max = time;
            ret.relative_item_id = i;
            ret.absolute_item_id = c_item.data()->id();
            ret.best_time = max;
        }
    }

    if(!is_alt){
        if(ret.candidates_times.count(ret.best_time) > 1){
            FifthRule alt_rule;
            TSelectionResult alt_res = alt_rule.selectItem(data, ptr, true);
            ret.alt_times = alt_res.candidates_times;
            for(int i{0}; i < ret.relative_candidates.size(); ++i){
                if(ret.candidates_times[i] != ret.best_time){
                    continue;
                }
                if(!ret.alt_time || ret.alt_times[i] > ret.alt_time){
                    ret.relative_item_id = i;
                    ret.absolute_item_id = ret.absolute_candidates[i];
                    ret.alt_time = ret.alt_times[i];
                }
            }
            ret.alt_calculated = true;
        }
    }

    return ret;
}

TSelectionResult FourthRule::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr, bool is_alt)
{
    (void)data;
    TSelectionResult ret;
    double min{INT_MAX};
    for(int i{0}; i < ptr.data()->queueSize(); ++i){
        QSharedPointer<Item> c_item = (*ptr.data())[i];
        if(c_item.data()->on_the_way){
            continue;
        }
        double time = c_item.data()->sumOfFPMtimes(c_item.data()->currentOp() + 1);

        ret.relative_candidates.push_back(i);
        ret.absolute_candidates.push_back(c_item.data()->id());
        ret.candidates_times.push_back(time);

        if(time < min){
            min = time;
            ret.relative_item_id = i;
            ret.absolute_item_id = c_item.data()->id();
            ret.best_time = min;
        }
    }

    if(!is_alt){
        if(ret.candidates_times.count(ret.best_time) > 1){
            FirstRule alt_rule;
            TSelectionResult alt_res = alt_rule.selectItem(data, ptr, true);
            ret.alt_times = alt_res.candidates_times;
            for(int i{0}; i < ret.relative_candidates.size(); ++i){
                if(ret.candidates_times[i] != ret.best_time){
                    continue;
                }
                if(!ret.alt_time || ret.alt_times[i] < ret.alt_time){
                    ret.relative_item_id = i;
                    ret.absolute_item_id = ret.absolute_candidates[i];
                    ret.alt_time = ret.alt_times[i];
                }
            }
            ret.alt_calculated = true;
        }
    }

    return ret;
}

TSelectionResult FifthRule::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr, bool is_alt)
{
    (void)data;
    TSelectionResult ret;
    int max{0};
    for(int i{0}; i < ptr.data()->queueSize(); ++i){
        QSharedPointer<Item> c_item = (*ptr.data())[i];
        if(c_item.data()->on_the_way){
            continue;
        }

        ret.relative_candidates.push_back(i);
        ret.absolute_candidates.push_back(c_item.data()->id());
        ret.candidates_times.push_back(c_item.data()->nextFPMtime());

        if(c_item.data()->nextFPMtime() > max){
            max = c_item.data()->nextFPMtime();
            ret.relative_item_id = i;
            ret.absolute_item_id = c_item.data()->id();
            ret.best_time = max;
        }
    }
    if(!is_alt){
        if(ret.candidates_times.count(ret.best_time) > 1){
            SecondRule alt_rule;
            TSelectionResult alt_res = alt_rule.selectItem(data, ptr, true);
            ret.alt_times = alt_res.candidates_times;
            for(int i{0}; i < ret.relative_candidates.size(); ++i){
                if(ret.candidates_times[i] != ret.best_time){
                    continue;
                }
                if(!ret.alt_time || ret.alt_times[i] > ret.alt_time){
                    ret.relative_item_id = i;
                    ret.absolute_item_id = ret.absolute_candidates[i];
                    ret.alt_time = ret.alt_times[i];
                }
            }
            ret.alt_calculated = true;
        }
    }
    return ret;
}

TSelectionResult ThirdReich::selectItem(DataStorage *data, QSharedPointer<FPM> &ptr, bool is_alt)
{
    TSelectionResult ret;
    TSelectionResult low_priority_ret;
    double min{INT_MAX};
    double low_priority_min{INT_MAX};
    for(int i{0}; i < ptr.data()->queueSize(); ++i){
        if((*ptr.data())[i].data()->on_the_way){
            continue;
        }

        int n_fpm = (*ptr.data())[i].data()->nextNextFPMid();
        double c_time{0};
        if(n_fpm > data->asCount() - 1){
            c_time = data->fpms[n_fpm - data->asCount()].data()->queueTime();

            ret.relative_candidates.push_back(i);
            ret.absolute_candidates.push_back((*ptr.data())[i].data()->id());
            ret.candidates_times.push_back((*ptr.data())[i].data()->nextFPMtime());

            if(c_time < min){
                min = c_time;
                ret.relative_item_id = i;
                ret.absolute_item_id = (*ptr.data())[i].data()->id();
            }
        }else{
            low_priority_min = 0;
            low_priority_ret.relative_item_id = i;
            low_priority_ret.absolute_item_id = (*ptr.data())[i].data()->id();
        }
    }

    if(!is_alt){
        if(ret.candidates_times.count(ret.best_time) > 1){
            FirstRule alt_rule;
            TSelectionResult alt_res = alt_rule.selectItem(data, ptr, true);
            ret.alt_times = alt_res.candidates_times;
            for(int i{0}; i < ret.relative_candidates.size(); ++i){
                if(ret.candidates_times[i] != ret.best_time){
                    continue;
                }
                if(!ret.alt_time || ret.alt_times[i] < ret.alt_time){
                    ret.relative_item_id = i;
                    ret.absolute_item_id = ret.absolute_candidates[i];
                    ret.alt_time = ret.alt_times[i];
                }
            }
            ret.alt_calculated = true;
        }
    }

    if(ret.relative_item_id >= 0){
        return ret;
    }
    return low_priority_ret;
}

