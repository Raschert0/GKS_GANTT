#ifndef ATM_H
#define ATM_H

#include "basefactory.h"

class DataStorage;

class ATM : public BaseFactory
{
public:
    ATM();
    ATM(int ts_id);
    ATM(const ATM &a);
    static void resetIdCounter();
    int tId();
    int cPos();
    double haulFromTo(int payload, int pickup, int dest, DataStorage *data, double start_time, int item_op);
    double deliverFromTo(int payload, int pickup, int dest, DataStorage *data, double start_time, int item_op);
    double transportToNextPos(QSharedPointer<Item> payload, DataStorage *data, double start_time, int ts_to_use);
    void moveTo(int new_pos);
private:
    int current_pos{0};
    int transport_system_id{0};

};

#endif // ATM_H
