#include <QMessageBox>
#include "errorshandler.h"

ErrorsHandler::ErrorsHandler()
{

}

void ErrorsHandler::outErr(ErrorCode code, int line)
{
    switch(code){
    case ErrorCode::UNEXPECTED_END_OF_FILE:
        QMessageBox::critical(nullptr,"Error","Неочікуваний кінець файлу. Рядок коду: " + QString::number(line));
        break;
    case ErrorCode::PARSE_ERROR:
        QMessageBox::critical(nullptr,"Error","Помилка при спробі розібрати вміст файлу. Рядок коду: " + QString::number(line));
        break;
    case ErrorCode::INCORRECT_NUMBER_OF_TS:
        QMessageBox::critical(nullptr,"Error","Кількість транспортних систем має бути в межах [1; 2].");
        break;
    default:
        break;
    }
}
