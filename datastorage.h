#ifndef DATASTORAGE_H
#define DATASTORAGE_H

#include <QTextStream>

#include "item.h"
#include "atm.h"
#include "fpm.h"
#include "as.h"
#include "suprule.h"
#include "casechart.h"

struct ATMSel{
    QVector<QSharedPointer<ATM>> used_atms;
    //bool path_is_valid{false};
    int used_ts;
    double time_for_all_moves;
};

class DataStorage
{
public:
    DataStorage();
    ~DataStorage();
    void clear();
    bool load(QString filename);
    void reset();
    void setNewRule(SupRule *ptr);
    void firstRun();
    void calculateThemAll();
    double timeToPosTF(int to, int from, int ts);
    double timeToMoveTF(int to, int from, int ts);
    QVector<QSharedPointer<ATM>> findBestAtmsToMoveFromTo(int from, int to, double *ret_time, int *ret_ts);
    QVector<QSharedPointer<ATM>> findBestAtmsToMoveFromToInList(int from, int to, double *ret_time, int *ret_ts, QVector<QSharedPointer<ATM>> &container);
    QVector<ATMSel> findAllAtmsTimesToMoveFromToInList(int from, int to, QVector<QSharedPointer<ATM>> &container);

    double loadTime();
    double unloadTime();
    int asCount();
    void disable_atms();

    QVector<bool> fpm_has_two_buffers;

private:
    //Ocean's Eleven
    friend class FirstRule;
    friend class SecondRule;
    friend class ThirdReich;
    friend class FourthRule;
    friend class FifthRule;
    friend class SixthRule;
    friend class SeventhRule;
    friend class MainWidget;

    bool no_atm{false};

    const double discrete{0.1};
    double current_time{0};

    template<typename T>
    bool readNumber(QTextStream &stream, T &numvar, int line = 0, bool zero_is_valid = false);

    QVector<QSharedPointer<Item>> original_items, items;
    QVector<QSharedPointer<ATM>> original_atms, atms;
    QVector<QSharedPointer<FPM>> original_fpms, fpms;
    QVector<QSharedPointer<AS>> original_as, as;

    QVector<QVector<QSharedPointer<ATM>>> stored_atms;
    QVector<QVector<QSharedPointer<FPM>>> stored_fpms;
    QVector<double> stored_time;

    QSharedPointer<BaseFactory> reloader;
    QVector<QSharedPointer<BaseFactory>> stored_reloaders;
    QSharedPointer<CaseChart> archive;
    QVector<QSharedPointer<CaseChart>> stored_archives;

    int items_count;
    int fpms_count;
    int ts_count;
    int atms_count;
    int as_count;

    double time_to_take_place;
    double time_to_load;
    double time_to_unload;
    double time_to_move;

    QVector<QVector<QVector<double>>> time_to_move_to_from;
    QVector<QVector<QVector<double>>> time_to_pos_to_from;
    QVector<QVector<QVector<int>>> move_intersections;

    QSharedPointer<SupRule> current_rule;
};

#endif // DATASTORAGE_H
