#ifndef LOGENTRY_H
#define LOGENTRY_H


class LogEntry
{
public:
    LogEntry(){}
    LogEntry(double start, double dur, int id);
    LogEntry(double start, double dur, int id, int s_pos, int e_pos);
    double start();
    double end();
    double duration();
    int itemId();
    bool isDone();
    void finish();
    int dest();
    int cross_move{0};
private:
    int s_pos{0};
    int e_pos{0};
    double start_time;
    double end_time;
    double duration_t;
    int item_id;
    bool finished{false};

};

#endif // LOGENTRY_H
