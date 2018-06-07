#ifndef ERRORSHANDLER_H
#define ERRORSHANDLER_H

enum class ErrorCode{
    OK,
    PARSE_ERROR,
    UNEXPECTED_END_OF_FILE,
    INCORRECT_NUMBER_OF_TS
};

class ErrorsHandler
{
public:
    ErrorsHandler();
    static void outErr(ErrorCode code, int line = 0);
};

#endif // ERRORSHANDLER_H
