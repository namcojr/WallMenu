#include "Config.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QTextStream>

namespace {
Config::FontStyle parseFontStyle(const QString& value) {
    const QString normalized = value.trimmed().toLower();
    if (normalized == "bold") {
        return Config::FontStyle::Bold;
    }
    if (normalized == "italic") {
        return Config::FontStyle::Italic;
    }
    if (normalized == "bold_italic" || normalized == "bolditalic") {
        return Config::FontStyle::BoldItalic;
    }
    return Config::FontStyle::Normal;
}

QString toString(Config::FontStyle style) {
    switch (style) {
    case Config::FontStyle::Normal:
        return "normal";
    case Config::FontStyle::Bold:
        return "bold";
    case Config::FontStyle::Italic:
        return "italic";
    case Config::FontStyle::BoldItalic:
        return "bold_italic";
    }
    return "normal";
}
}

Config::Data Config::defaultConfig() {
    Data data;
    const QString home = QDir::homePath();
    data.wallpaperDir = std::filesystem::path((home + "/Pictures/wallpapers").toStdString());
    data.recursive = true;

    const QFont defaultFont;
    data.listFontName = defaultFont.family();
    data.listFontSize = 11;

    data.titleFontName = defaultFont.family();
    data.titleFontSize = 11;
    data.titleFontStyle = FontStyle::Bold;
    return data;
}

QString Config::configFilePath() {
    return QDir::homePath() + "/.config/wallmenu.conf";
}

bool Config::writeDefaultConfig(const QString& path, const Data& data, QString* errorMessage) {
    QFileInfo fileInfo(path);
    QDir parentDir = fileInfo.dir();
    if (!parentDir.exists() && !parentDir.mkpath(".")) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("Failed to create config directory: %1").arg(parentDir.path());
        }
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("Failed to create config file: %1").arg(path);
        }
        return false;
    }

    QTextStream out(&file);
    out << "wallpaper_dir=" << QString::fromStdString(data.wallpaperDir.string()) << "\n";
    out << "recursive=" << (data.recursive ? "true" : "false") << "\n";
    out << "list_font_name=" << data.listFontName << "\n";
    out << "list_font_size=" << data.listFontSize << "\n";
    out << "title_font_name=" << data.titleFontName << "\n";
    out << "title_font_size=" << data.titleFontSize << "\n";
    out << "title_font_style=" << toString(data.titleFontStyle) << "\n";
    return true;
}

bool Config::parseLine(const QString& line, QString* key, QString* value) {
    const int equalsIndex = line.indexOf('=');
    if (equalsIndex <= 0) {
        return false;
    }

    const QString parsedKey = line.left(equalsIndex).trimmed();
    const QString parsedValue = line.mid(equalsIndex + 1).trimmed();
    if (parsedKey.isEmpty()) {
        return false;
    }

    if (key != nullptr) {
        *key = parsedKey;
    }
    if (value != nullptr) {
        *value = parsedValue;
    }
    return true;
}

std::optional<Config::Data> Config::load(QString* errorMessage) {
    const QString path = configFilePath();
    Data data = defaultConfig();

    QFile file(path);
    if (!file.exists()) {
        if (!writeDefaultConfig(path, data, errorMessage)) {
            return std::nullopt;
        }
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("Failed to open config file: %1").arg(path);
        }
        return std::nullopt;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString rawLine = in.readLine().trimmed();
        if (rawLine.isEmpty() || rawLine.startsWith('#')) {
            continue;
        }

        QString key;
        QString value;
        if (!parseLine(rawLine, &key, &value)) {
            continue;
        }

        if (key == "wallpaper_dir") {
            data.wallpaperDir = std::filesystem::path(value.toStdString());
        } else if (key == "recursive") {
            const QString normalized = value.toLower();
            data.recursive = (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on");
        } else if (key == "list_font_name") {
            if (!value.isEmpty()) {
                data.listFontName = value;
            }
        } else if (key == "list_font_size") {
            bool ok = false;
            const int parsed = value.toInt(&ok);
            if (ok && parsed >= 6 && parsed <= 72) {
                data.listFontSize = parsed;
            }
        } else if (key == "title_font_name") {
            if (!value.isEmpty()) {
                data.titleFontName = value;
            }
        } else if (key == "title_font_size") {
            bool ok = false;
            const int parsed = value.toInt(&ok);
            if (ok && parsed >= 6 && parsed <= 72) {
                data.titleFontSize = parsed;
            }
        } else if (key == "title_font_style") {
            data.titleFontStyle = parseFontStyle(value);
        }
    }

    std::error_code ec;
    if (!std::filesystem::exists(data.wallpaperDir, ec) || !std::filesystem::is_directory(data.wallpaperDir, ec)) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("Configured wallpaper_dir is invalid: %1")
                                .arg(QString::fromStdString(data.wallpaperDir.string()));
        }
        return std::nullopt;
    }

    return data;
}
