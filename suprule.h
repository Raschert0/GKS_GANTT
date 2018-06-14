#ifndef SUPRULE_H
#define SUPRULE_H

#include <QSharedPointer>
#include <QVector>

class FPM;

class DataStorage;

class TSelectionResult{
public:
    int relative_item_id{-1};
    int absolute_item_id{-1};
    bool alt_calculated{false};
    QVector<int> relative_candidates{};
    QVector<int> absolute_candidates{};
    QVector<int> cached_queue{};
    QVector<double> candidates_times{};
    QVector<double> alt_times{};
    double best_time{0};
    double alt_time{0};
};

class SupRule
{
public:
    SupRule();
    virtual ~SupRule();
    //Вибір індексу деталі для обробки згідно до правила переваги
    virtual TSelectionResult selectItem(DataStorage *data, QSharedPointer<FPM> &ptr, bool is_alt = false) = 0;
};

class FirstRule : public SupRule{
public:
    TSelectionResult selectItem(DataStorage *data, QSharedPointer<FPM> &ptr, bool is_alt = false);
};

class SecondRule : public SupRule{
public:
    TSelectionResult selectItem(DataStorage *data, QSharedPointer<FPM> &ptr, bool is_alt = false);
};

class FourthRule : public SupRule{
public:
    TSelectionResult selectItem(DataStorage *data, QSharedPointer<FPM> &ptr, bool is_alt = false);
};

class FifthRule : public SupRule{
public:
    TSelectionResult selectItem(DataStorage *data, QSharedPointer<FPM> &ptr, bool is_alt = false);
};

class ThirdReich : public SupRule{
public:
    TSelectionResult selectItem(DataStorage *data, QSharedPointer<FPM> &ptr, bool is_alt = false);
};

#endif // SUPRULE_H
