#pragma once

#include <QString>
#include <QStringList>

class WallpaperSetter {
public:
    static bool setWallpaper(const QString& imagePath, QString* errorMessage);

private:
    static bool setForKde(const QString& imagePath, QString* errorMessage);
    static bool setForKdeWithScript(const QString& qdbusProgram, const QString& imagePath, QString* errorMessage);
    static bool setForSway(const QString& imagePath, QString* errorMessage);
    static bool setWithFeh(const QString& imagePath, QString* errorMessage);
    static bool runCommand(const QString& program, const QStringList& args, QString* errorMessage);
};
