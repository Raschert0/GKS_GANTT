#ifndef FPM_H
#define FPM_H

#include "basefactory.h"
#include "suprule.h"

class FPM : public BaseFactory
{
public:
    FPM();
    FPM(const FPM &f);
    static void resetIdCounter();
    int queueSize();
    QSharedPointer<Item> operator[](const int id);
    void selectFromQueue(int num);
    void selectFromQueueById(int num);
    double queueTime();
    TSelectionResult last_best;
    double last_wait_for_supply{0};
    double last_delay{0};
};

#endif // FPM_H
