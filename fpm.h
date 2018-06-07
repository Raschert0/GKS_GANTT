#ifndef FPM_H
#define FPM_H

#include "basefactory.h"

class FPM : public BaseFactory
{
public:
    FPM();
    FPM(const FPM &f);
    int queueSize();
    QSharedPointer<Item> operator[](const int id);
    void selectFromQueue(int num);
    void selectFromQueueById(int num);
    double queueTime();
};

#endif // FPM_H
