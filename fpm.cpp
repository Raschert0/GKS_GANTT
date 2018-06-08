#include "fpm.h"
#include <QDebug>

static int global_fpm_id{0};

FPM::FPM() : BaseFactory()
{
    id = global_fpm_id;
    ++global_fpm_id;
}

FPM::FPM(const FPM &f)
{
    id = f.id;
}

void FPM::resetIdCounter()
{
    global_fpm_id = 0;
}

int FPM::queueSize()
{
    return queue.size();
}

QSharedPointer<Item> FPM::operator[](const int id)
{
    if(id < 0 || id > queue.size()){
        return QSharedPointer<Item>();
    }
    return queue[id];
}

void FPM::selectFromQueue(int num)
{
    if(num < 0 || num > queue.size()){
        qDebug() << "wrong num (select from queue)";
        return;
    }
    current_item = queue.takeAt(num);
}

void FPM::selectFromQueueById(int num)
{
    for(int i{0}; i < queue.size(); ++i){
        if(queue[i].data()->id() == num){
            current_item = queue.takeAt(i);
            return;
        }
    }
}

double FPM::queueTime()
{
    double ret{0};
    for(int i{0}; i < queue.size(); ++i){
        ret += queue[i].data()->nextFPMtime();
    }
    return ret;
}
