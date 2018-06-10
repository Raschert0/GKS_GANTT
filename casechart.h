#ifndef CASECHART_H
#define CASECHART_H

#include <QMap>

class CaseChart
{
public:
    CaseChart(int size);
    void registerTime(int time);
    void saveString(QString &value, int id, double time);

    QMap<int, QVector<QString>> data;
    QVector<QMap<double, QString>> detailed_data;
private:
    int size{0};
};

#endif // CASECHART_H
