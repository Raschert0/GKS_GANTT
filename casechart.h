#ifndef CASECHART_H
#define CASECHART_H

#include <QMap>

class CaseChart
{
public:
    CaseChart(int size);
    void registerTime(int time);
    void saveString(QString &value, int id, int time);

    QMap<int, QVector<QString>> data;
private:
    int size{0};
};

#endif // CASECHART_H
