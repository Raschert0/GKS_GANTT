#ifndef SUPRULE_H
#define SUPRULE_H

#include <QSharedPointer>
#include <fpm.h>

class DataStorage;

class SupRule
{
public:
    SupRule();
    virtual ~SupRule();
    //Вибір індексу деталі для обробки згідно до правила переваги
    virtual int selectItem(DataStorage *data, QSharedPointer<FPM> &ptr) = 0;
};

class FirstRule : public SupRule{
public:
    int selectItem(DataStorage *data, QSharedPointer<FPM> &ptr);
};

class SecondRule : public SupRule{
public:
    int selectItem(DataStorage *data, QSharedPointer<FPM> &ptr);
};

class FourthRule : public SupRule{
public:
    int selectItem(DataStorage *data, QSharedPointer<FPM> &ptr);
};

class FifthRule : public SupRule{
public:
    int selectItem(DataStorage *data, QSharedPointer<FPM> &ptr);
};

class ThirdReich : public SupRule{
public:
    int selectItem(DataStorage *data, QSharedPointer<FPM> &ptr);
};

class SixthRule : public SupRule{
public:
    int selectItem(DataStorage *data, QSharedPointer<FPM> &ptr);
};

class SeventhRule : public SupRule{
public:
    int selectItem(DataStorage *data, QSharedPointer<FPM> &ptr);
};

#endif // SUPRULE_H
