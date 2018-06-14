#include "item.h"
#include <QDebug>

static int global_item_id{0};

Item::Item()
{

    _id = global_item_id;
    ++global_item_id;
}

Item::Item(const Item &i) : operations{i.operations}, times{i.times}
{
    _id = i._id;
}

void Item::resetIdCounter()
{
    global_item_id = 0;
}

void Item::addOperation(int fpm_id, double time)
{
    operations.push_back(fpm_id);
    times.push_back(time);
}

void Item::setNextOperation()
{
    ++current_op;
}

int Item::currentOp()
{
    return current_op;
}

void Item::setFactory(const QSharedPointer<BaseFactory> &ptr)
{
    current_factory = ptr;
}

int Item::id()
{
    return _id;
}

QSharedPointer<BaseFactory> Item::factory()
{
    return current_factory;
}

int Item::nextFPMid()
{
    if(send_me_to_as){
        return send_me_to_as - 1;
    }
    if(current_op >= operations.size() - 1){
        return 0;
    }
    return operations[current_op + 1];
}

int Item::nextNextFPMid()
{
    if(current_op >= operations.size() - 2){
        return 0;
    }
    return operations[current_op + 2];
}

int Item::currentFPMid()
{
    if(current_op < 0 || current_op > operations.size() - 1){
        return 0;
    }
    return operations[current_op];
}

double Item::nextFPMtime()
{
    if(current_op >= times.size() - 1){
        return -1;
    }
    return times[current_op + 1];
}

double Item::sumOfFPMtimes(int op_id)
{
    double ret{0};
    for(int i{op_id}; i < times.size(); ++i){
        ret += times[i];
    }
    return ret;
}
