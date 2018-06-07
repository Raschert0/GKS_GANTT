#ifndef BASEFACTORY_H
#define BASEFACTORY_H

#include <QSharedPointer>
#include <QVector>
#include "item.h"
#include "logentry.h"

enum class FStatus{
    Idle,
    Busy,
    Positioning,
    WaitingForSupply
};

class BaseFactory
{
public:
    BaseFactory();
    void addToQueue(QSharedPointer<Item> item);
    QVector<LogEntry> &getLog();
    FStatus status();
    void setStatus(FStatus s);
    int Id();
    void setCurrent(QSharedPointer<Item> &ptr);
    QSharedPointer<Item> &currentItem();

public: //because i can do
    int id{0};
    FStatus _status{FStatus::Idle};
    /* 0 - idle
     * 1 - busy
     * 2 - positioning
     **/
    QVector<QSharedPointer<Item>> queue;
    QSharedPointer<Item> current_item{nullptr};
    QVector<LogEntry> log;
};

#endif // BASEFACTORY_H
