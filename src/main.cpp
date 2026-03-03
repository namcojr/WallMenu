#include "Config.h"
#include "MainWindow.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QString errorMessage;
    auto config = Config::load(&errorMessage);
    if (!config.has_value()) {
        QMessageBox::critical(nullptr, "Wallmenu", errorMessage);
        return 1;
    }

    MainWindow window(std::move(config.value()));
    window.show();

    return app.exec();
}
