#include "as.h"

AS::AS()
{
    static int global_as_id{0};
    as_id = global_as_id;
    ++global_as_id;
}

int AS::id()
{
    return as_id;
}
