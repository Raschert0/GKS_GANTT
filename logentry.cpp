#include "logentry.h"

LogEntry::LogEntry(double start, double dur, int id) : start_time{start}, duration_t{dur}, item_id{id}
{
    end_time = start + dur;
}

LogEntry::LogEntry(double start, double dur, int id, int s_pos, int e_pos) : start_time{start}, duration_t{dur}, item_id{id}, s_pos{s_pos}, e_pos{e_pos}
{
    end_time = start + dur;
}

double LogEntry::start()
{
    return start_time;
}

double LogEntry::end()
{
    return end_time;
}

double LogEntry::duration()
{
    return duration_t;
}

int LogEntry::itemId()
{
    return item_id;
}

bool LogEntry::isDone()
{
    return finished;
}

void LogEntry::finish()
{
    finished = true;
}

int LogEntry::dest()
{
    return e_pos;
}
