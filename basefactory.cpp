#include "basefactory.h"

BaseFactory::BaseFactory()
{

}

void BaseFactory::addToQueue(QSharedPointer<Item> item)
{
    queue.append(item);
}

QVector<LogEntry> &BaseFactory::getLog()
{
    return log;
}

FStatus BaseFactory::status()
{
    return _status;
}

void BaseFactory::setStatus(FStatus s)
{
    _status = s;
}

int BaseFactory::Id()
{
    return id;
}

void BaseFactory::setCurrent(QSharedPointer<Item> &ptr)
{
    current_item = ptr;
}

QSharedPointer<Item> &BaseFactory::currentItem()
{
    return current_item;
}
