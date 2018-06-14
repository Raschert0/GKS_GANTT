#ifndef ITEM_H
#define ITEM_H

#include <QVector>
#include <QSharedPointer>

class BaseFactory;

class Item
{
public:
    Item();
    Item(const Item &i);
    static void resetIdCounter();
    void addOperation(int fpm_id, double time);
    int nextFPMid();
    int nextNextFPMid();
    int currentFPMid();
    double nextFPMtime();
    double sumOfFPMtimes(int op_id); //Сума всіх часів обробки після операції (включно з нею)
    void setNextOperation();
    int currentOp();
    void setFactory(const QSharedPointer<BaseFactory> &ptr);
    int id();
    QSharedPointer<BaseFactory> factory();
    bool on_the_way{false};
    int send_me_to_as{0};
    int current_pos{0};
private:
    int _id;
    int current_op{-1};
    QVector<int> operations;
    QVector<double> times;
    QSharedPointer<BaseFactory> current_factory{nullptr};
};

#endif // ITEM_H
