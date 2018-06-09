#include <QVector>
#include <QString>

#include "casechart.h"

CaseChart::CaseChart(int size) : size{size}
{

}

void CaseChart::registerTime(int time)
{
    if(data.find(time) == data.end()){
        data[time].resize(size);
    }
}

void CaseChart::saveString(QString &value, int id, int time)
{
    registerTime(time);
    data[time][id] = value;
    int time_index =  data.keys().indexOf(time);
    if(time_index <= 0){
        return;
    }
    int prev_time = data.keys()[time_index - 1];
    for(int i{0}; i < size; ++i){
        if(i == id){
            continue;
        }
        if(data[prev_time][i].isEmpty() || !data[time][i].isNull()){
            continue;
        }
        data[time][i] = "-//-";
    }
}
