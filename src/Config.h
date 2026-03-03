#pragma once

#include <filesystem>
#include <optional>
#include <QString>

class Config {
public:
    enum class FontStyle {
        Normal,
        Bold,
        Italic,
        BoldItalic,
    };

    struct Data {
        std::filesystem::path wallpaperDir;
        bool recursive = true;

        QString listFontName;
        int listFontSize = 11;

        QString titleFontName;
        int titleFontSize = 11;
        FontStyle titleFontStyle = FontStyle::Bold;
    };

    static QString configFilePath();
    static std::optional<Data> load(QString* errorMessage);

private:
    static Data defaultConfig();
    static bool writeDefaultConfig(const QString& path, const Data& data, QString* errorMessage);
    static bool parseLine(const QString& line, QString* key, QString* value);
};
