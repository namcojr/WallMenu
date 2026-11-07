#include "WallpaperSetter.h"

#include <QProcess>
#include <QStandardPaths>

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

    // Plasma ships a dedicated CLI for this; prefer it.
    if (!QStandardPaths::findExecutable("plasma-apply-wallpaperimage").isEmpty()) {
        if (runCommand("plasma-apply-wallpaperimage", {imagePath}, errorMessage)) {
            return true;
        }
    }

    // Fall back to the scripting API. The qdbus binary name differs per distro/Qt version.
    const QStringList dbusArgs = {
        "org.kde.plasmashell",
        "/PlasmaShell",
        "org.kde.PlasmaShell.evaluateScript",
        script,
    };
    for (const QString& qdbus : {QStringLiteral("qdbus6"), QStringLiteral("qdbus-qt6"), QStringLiteral("qdbus")}) {
        if (QStandardPaths::findExecutable(qdbus).isEmpty()) {
            continue;
        }
        if (runCommand(qdbus, dbusArgs, errorMessage)) {
            return true;
        }
    }

    if (errorMessage != nullptr && errorMessage->isEmpty()) {
        *errorMessage = "KDE detected, but neither plasma-apply-wallpaperimage nor qdbus6/qdbus was found.";
    }
    return false;
}

bool WallpaperSetter::setForSway(const QString& imagePath, QString* errorMessage) {
    return runCommand("swaymsg", {"output", "*", "bg", imagePath, "fill"}, errorMessage);
}

bool WallpaperSetter::setWithFeh(const QString& imagePath, QString* errorMessage) {
    return runCommand("feh", {"--bg-fill", imagePath}, errorMessage);
}

bool WallpaperSetter::setWallpaper(const QString& imagePath, QString* errorMessage) {
    const QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP").toLower();

    // Report the error from the desktop-specific setter rather than a misleading feh failure.
    if (desktop.contains("kde")) {
        return setForKde(imagePath, errorMessage);
    }

    if (desktop.contains("sway")) {
        return setForSway(imagePath, errorMessage);
    }

    // feh only works on X11.
    if (qEnvironmentVariable("XDG_SESSION_TYPE") == "wayland") {
        if (errorMessage != nullptr) {
            *errorMessage = QString("Unsupported Wayland desktop: %1").arg(desktop.isEmpty() ? "unknown" : desktop);
        }
        return false;
    }

    return setWithFeh(imagePath, errorMessage);
}
