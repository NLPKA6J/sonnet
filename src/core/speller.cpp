/**
 *
 * Copyright (C)  2007  Zack Rusin <zack@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#include "speller.h"

#include "loader_p.h"
#include "settings_p.h"
#include "spellerplugin_p.h"

#include <QtCore/QLocale>
#include <QSet>
#include <QDebug>

namespace Sonnet
{

class Speller::Private
{
public:
    ~Private()
    {
        delete dict;
        dict = 0;
    }
    void init(const QString &lang)
    {
        Loader *loader = Loader::openLoader();
        settings = loader->settings();

        language = lang;

        if (language.isEmpty()) {
            language = settings->defaultLanguage();
        }

        dict = loader->createSpeller(language);
    }
    bool isValid()
    {
        if (settings->modified()) {
            recreateDict();
            settings->setModified(false);
        }
        return dict;
    }
    void recreateDict()
    {
        delete dict;
        dict = Loader::openLoader()->createSpeller(language);
    }

    SpellerPlugin *dict;
    Settings      *settings;

    QString        language;
};

Speller::Speller(const QString &lang)
    : d(new Private)
{
    d->init(lang);
}

Speller::~Speller()
{
    //qDebug()<<"deleting "<<this;
    delete d;
}

Speller::Speller(const Speller &speller)
    : d(new Private)
{
    d->language = speller.language();
    d->init(d->language);
}

Speller &Speller::operator=(const Speller &speller)
{
    d->language = speller.language();
    d->recreateDict();
    return *this;
}

bool Speller::isCorrect(const QString &word) const
{
    if (!d->isValid()) {
        return true;
    }
    return d->dict->isCorrect(word);
}

bool Speller::isMisspelled(const QString &word) const
{
    if (!d->isValid()) {
        return false;
    }
    return d->dict->isMisspelled(word);
}

QStringList Speller::suggest(const QString &word) const
{
    if (!d->isValid()) {
        return QStringList();
    }
    return d->dict->suggest(word);
}

bool Speller::checkAndSuggest(const QString &word,
                              QStringList &suggestions) const
{
    if (!d->isValid()) {
        return true;
    }
    return d->dict->checkAndSuggest(word, suggestions);
}

bool Speller::storeReplacement(const QString &bad,
                               const QString &good)
{
    if (!d->isValid()) {
        return false;
    }
    return d->dict->storeReplacement(bad, good);
}

bool Speller::addToPersonal(const QString &word)
{
    if (!d->isValid()) {
        return false;
    }
    return d->dict->addToPersonal(word);
}

bool Speller::addToSession(const QString &word)
{
    if (!d->isValid()) {
        return false;
    }
    return d->dict->addToSession(word);
}

QString Speller::language() const
{
    if (!d->isValid()) {
        return QString();
    }
    return d->dict->language();
}

void Speller::save()
{
    if (d->settings) {
        d->settings->save();
    }
}

void Speller::restore()
{
    if (d->settings) {
        d->settings->restore();
        d->recreateDict();
    }
}

QStringList Speller::availableBackends() const
{
    Loader *l = Loader::openLoader();
    return l->clients();
}

QStringList Speller::availableLanguages() const
{
    Loader *l = Loader::openLoader();
    return l->languages();
}

QStringList Speller::availableLanguageNames() const
{
    Loader *l = Loader::openLoader();
    return l->languageNames();
}

void Speller::setDefaultLanguage(const QString &lang)
{
    d->settings->setDefaultLanguage(lang);
}

QString Speller::defaultLanguage() const
{
    return d->settings->defaultLanguage();
}

void Speller::setDefaultClient(const QString &client)
{
    d->settings->setDefaultClient(client);
}

QString Speller::defaultClient() const
{
    return d->settings->defaultClient();
}

void Speller::setAttribute(Attribute attr, bool b)
{
    switch (attr) {
    case CheckUppercase:
        d->settings->setCheckUppercase(b);
        break;
    case SkipRunTogether:
        d->settings->setSkipRunTogether(b);
        break;
    }
}

bool Speller::testAttribute(Attribute attr) const
{
    switch (attr) {
    case CheckUppercase:
        return d->settings->checkUppercase();
        break;
    case SkipRunTogether:
        return d->settings->skipRunTogether();
        break;
    }
    return false;
}

bool Speller::isValid() const
{
    return d->dict;
}

void Speller::setLanguage(const QString &lang)
{
    d->language = lang;
    d->recreateDict();
}

QMap<QString, QString> Sonnet::Speller::availableDictionaries() const
{
    Loader *l = Loader::openLoader();
    const QStringList lst = l->languages();
    QMap<QString, QString> langs;

    Q_FOREACH (QString tag, lst) { // krazy:exclude=foreach (no const& because tag is modified below)
        tag = tag.mid(0, tag.indexOf(QLatin1Char('-')));
        QLocale loc(tag);
        QString description;

        if (!loc.nativeCountryName().isEmpty())
            description = QString::fromLatin1("%1 (%2)")
                          .arg(loc.nativeLanguageName())
                          .arg(loc.nativeCountryName());
        else {
            description = loc.nativeLanguageName();
        }
        //qDebug()<<"Dict is "<<tag<<" ( "<<loc.name()<<")"<<", descr = "<<description;
        langs.insert(description, tag);
    }

    return langs;
}

}