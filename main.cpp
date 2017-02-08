#include <QApplication>
#include "fecoview.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FecoView fecoView;
    fecoView.show();
    return a.exec();
}
