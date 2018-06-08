#include "as.h"

static int global_as_id{0};

AS::AS()
{
    as_id = global_as_id;
    ++global_as_id;
}

int AS::id()
{
    return as_id;
}

void AS::resetIdCounter()
{
    global_as_id = 0;
}
