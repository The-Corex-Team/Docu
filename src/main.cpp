#include "gui/MainWindow.h"
#include "Style.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Docu"));
    QApplication::setOrganizationName(QStringLiteral("Docu"));
    application.setStyleSheet(DocuStyle::applicationStyleSheet());

    MainWindow window;
    window.show();

    return application.exec();
}
