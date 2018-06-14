#include "atm.h"
#include "datastorage.h"

static int global_atm_id{0};

ATM::ATM() : BaseFactory()
{
    id = global_atm_id;
    ++global_atm_id;
}

ATM::ATM(int ts_id) : ATM()
{
    transport_system_id = ts_id;
}

ATM::ATM(const ATM &a) : transport_system_id{a.transport_system_id}
{
    id = a.id;
}

void ATM::resetIdCounter()
{
    global_atm_id = 0;
}

int ATM::tId()
{
    return transport_system_id;
}

int ATM::cPos()
{
    return current_pos;
}

double ATM::haulFromTo(int payload, int pickup, int dest, DataStorage *data, double start_time, int item_op)
{
    if(log.size()){
        if(start_time < log.last().end() + data->timeToPosTF(pickup, current_pos, transport_system_id)){
            start_time = log.last().end() + data->timeToPosTF(pickup, current_pos, transport_system_id);
        }
    }


    setStatus(FStatus::Busy);

    double duration{0};
    duration += data->timeToMoveTF(dest, pickup, transport_system_id);
    if(dest > data->asCount() - 1){
        duration -= data->loadTime();
    }

    log.push_back(LogEntry(start_time, duration, payload, pickup, dest, item_op));
    moveTo(dest);
    return start_time + duration;
}

//It's da copy-paste time!
double ATM::deliverFromTo(int payload, int pickup, int dest, DataStorage *data, double start_time, int item_op)
{
    if(log.size()){
        if(start_time < log.last().end() + data->timeToPosTF(pickup, current_pos, transport_system_id)){
            start_time = log.last().end() + data->timeToPosTF(pickup, current_pos, transport_system_id);
        }
    }

    setStatus(FStatus::Busy);

    double duration{0};
    duration += data->timeToMoveTF(dest, pickup, transport_system_id);
    if(pickup > data->asCount() - 1){
        duration -= data->unloadTime();
    }

    log.push_back(LogEntry(start_time, duration, payload, pickup, dest, item_op));
    moveTo(dest);
    return start_time + duration;
}

double ATM::transportToNextPos(QSharedPointer<Item> payload, DataStorage *data, double start_time, int ts_to_use)
{
    int c_pos = payload.data()->current_pos;
    int n_pos = payload.data()->nextFPMid();

    if(log.size()){
        if(start_time < log.last().end() + data->timeToPosTF(c_pos, current_pos, transport_system_id)){
            start_time = log.last().end() + data->timeToPosTF(c_pos, current_pos, transport_system_id);
        }
    }


    setStatus(FStatus::Busy);

    double duration{0};
    if(ts_to_use > 1){
        duration +=  data->timeToMoveTF(n_pos, c_pos, ts_to_use);
    }else{
        duration += data->timeToMoveTF(n_pos, c_pos, transport_system_id);
    }

    log.push_back(LogEntry(start_time, duration, payload.data()->id(), c_pos, n_pos, payload.data()->currentOp()));
    moveTo(n_pos);
    return start_time + duration;
}

void ATM::moveTo(int new_pos)
{
    current_pos = new_pos;
}

