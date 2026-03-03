#include "WallpaperSetter.h"

#include <QProcess>

namespace {
QString escapeForJs(const QString& input) {
    QString escaped = input;
    escaped.replace('\\', "\\\\");
    escaped.replace('\"', "\\\"");
    return escaped;
}
}

bool WallpaperSetter::runCommand(const QString& program, const QStringList& args, QString* errorMessage) {
    QProcess process;
    process.start(program, args);

    if (!process.waitForStarted()) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("Failed to start command: %1").arg(program);
        }
        return false;
    }

    if (!process.waitForFinished(10000)) {
        process.kill();
        if (errorMessage != nullptr) {
            *errorMessage = QString("Command timed out: %1").arg(program);
        }
        return false;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("Command failed: %1\n%2")
                                .arg(program, QString::fromLocal8Bit(process.readAllStandardError()));
        }
        return false;
    }

    return true;
}

bool WallpaperSetter::setForKde(const QString& imagePath, QString* errorMessage) {
    const QString uri = QString("file://%1").arg(imagePath);
    const QString script = QString(
        "var allDesktops = desktops();"
        "for (i=0;i<allDesktops.length;i++) {"
        "d = allDesktops[i];"
        "d.wallpaperPlugin = 'org.kde.image';"
        "d.currentConfigGroup = ['Wallpaper', 'org.kde.image', 'General'];"
        "d.writeConfig('Image', '" + escapeForJs(uri) + "');"
        "}"
    );

    return runCommand(
        "qdbus",
        {
            "org.kde.plasmashell",
            "/PlasmaShell",
            "org.kde.PlasmaShell.evaluateScript",
            script,
        },
        errorMessage
    );
}

bool WallpaperSetter::setForSway(const QString& imagePath, QString* errorMessage) {
    return runCommand("swaymsg", {"output", "*", "bg", imagePath, "fill"}, errorMessage);
}

bool WallpaperSetter::setWithFeh(const QString& imagePath, QString* errorMessage) {
    return runCommand("feh", {"--bg-fill", imagePath}, errorMessage);
}

bool WallpaperSetter::setWallpaper(const QString& imagePath, QString* errorMessage) {
    const QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP").toLower();

    if (desktop.contains("kde")) {
        if (setForKde(imagePath, errorMessage)) {
            return true;
        }
    }

    if (desktop.contains("sway")) {
        if (setForSway(imagePath, errorMessage)) {
            return true;
        }
    }

    return setWithFeh(imagePath, errorMessage);
}
