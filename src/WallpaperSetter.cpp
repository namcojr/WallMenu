#include "WallpaperSetter.h"

#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

namespace {
// Escapes a string so it can be embedded inside a single- or double-quoted
// JavaScript string literal.
QString escapeForJs(const QString& input) {
    QString escaped;
    escaped.reserve(input.size());
    for (const QChar c : input) {
        switch (c.unicode()) {
        case '\\': escaped += "\\\\"; break;
        case '\'': escaped += "\\'"; break;
        case '"': escaped += "\\\""; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default: escaped += c; break;
        }
    }
    return escaped;
}

// Returns the first program from the list that exists in PATH (or as an
// absolute path), or an empty string if none is found.
QString findFirstExecutable(const QStringList& candidates) {
    for (const QString& candidate : candidates) {
        if (candidate.startsWith('/')) {
            if (QFile::exists(candidate)) {
                return candidate;
            }
            continue;
        }
        const QString found = QStandardPaths::findExecutable(candidate);
        if (!found.isEmpty()) {
            return found;
        }
    }
    return {};
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
            // Some tools (e.g. plasma-apply-wallpaperimage) report errors on stdout.
            QStringList output;
            for (const QByteArray& stream : {process.readAllStandardError(), process.readAllStandardOutput()}) {
                const QString text = QString::fromLocal8Bit(stream).trimmed();
                if (!text.isEmpty()) {
                    output << text;
                }
            }
            *errorMessage = QString("Command failed: %1\n%2").arg(program, output.join('\n'));
        }
        return false;
    }

    return true;
}

bool WallpaperSetter::setForKde(const QString& imagePath, QString* errorMessage) {
    // Preferred: the official Plasma CLI (Plasma 5.24+ and Plasma 6).
    const QString applyTool = QStandardPaths::findExecutable("plasma-apply-wallpaperimage");
    if (!applyTool.isEmpty()) {
        return runCommand(applyTool, {imagePath}, errorMessage);
    }

    // Fallback: evaluate a Plasma script over D-Bus. On Plasma 6 the tool is
    // usually qdbus6; plain qdbus is often the Qt5 build or missing entirely.
    const QString qdbus = findFirstExecutable({
        "qdbus6",
        "qdbus-qt6",
        "/usr/lib/qt6/bin/qdbus",
        "qdbus",
        "qdbus-qt5",
    });
    if (qdbus.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = "KDE Plasma detected, but neither plasma-apply-wallpaperimage "
                            "nor qdbus/qdbus6 was found.";
        }
        return false;
    }

    return setForKdeWithScript(qdbus, imagePath, errorMessage);
}

bool WallpaperSetter::setForKdeWithScript(const QString& qdbusProgram, const QString& imagePath, QString* errorMessage) {
    const QString uri = QUrl::fromLocalFile(imagePath).toString(QUrl::FullyEncoded);
    const QString script = QString(
        "var allDesktops = desktops();"
        "for (var i = 0; i < allDesktops.length; i++) {"
        "var d = allDesktops[i];"
        "d.wallpaperPlugin = 'org.kde.image';"
        "d.currentConfigGroup = ['Wallpaper', 'org.kde.image', 'General'];"
        "d.writeConfig('Image', '%1');"
        "}"
    ).arg(escapeForJs(uri));

    return runCommand(
        qdbusProgram,
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

    // When a known desktop is detected, report its own error instead of
    // falling back to feh, which cannot draw over Plasma or a Wayland session.
    if (desktop.contains("kde")) {
        return setForKde(imagePath, errorMessage);
    }

    if (desktop.contains("sway")) {
        return setForSway(imagePath, errorMessage);
    }

    return setWithFeh(imagePath, errorMessage);
}
