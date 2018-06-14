#ifndef CASECHART_H
#define CASECHART_H

#include <QMap>

struct Case{
    QVector<int> ids;
    QVector<QString> times_strs;
    QString string_rep;
};

class CaseChart
{
public:
    CaseChart(int size);
    void registerTime(int time);
    void saveCase(const Case &value, int id, double time);

    QMap<int, QVector<Case>> data;
    QVector<QMap<double, Case>> detailed_data;
private:
    int size{0};
};

#endif // CASECHART_H
