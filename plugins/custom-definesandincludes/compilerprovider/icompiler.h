/*
 * This file is part of KDevelop
 *
 * Copyright 2014 Sergey Kalinichev <kalinichev.so.0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef ICOMPILER_H
#define ICOMPILER_H

#include <QString>
#include <QSharedPointer>

#include "../idefinesandincludesmanager.h"

namespace Utils
{
enum LanguageType
{
    C,
    Cpp,
    OpenCl,
    Cuda,
    ObjC,
    ObjCpp,

    Other
};
}

/// An interface that represents a compiler. Compiler provides standard include directories and standard defined macros.
class ICompiler
{
public:
    /**
     * @param name The user visible name
     * @param path path to the compiler
     * @param factoryName name of the factory that created this compiler
     * @param editable whether user can change the name and the path to the compiler (should be set to false for automatically detected compilers)
    **/
    ICompiler( const QString& name, const QString& path, const QString& factoryName, bool editable );

    /**
     * @param type Language type, must not be @ref Utils::Other.
     * @param arguments compiler command-line arguments
     * @return list of defined macros for the compiler
     */
    virtual KDevelop::Defines defines(Utils::LanguageType type, const QString& arguments) const = 0;

    /**
     * @param type Language type, must not be @ref Utils::Other.
     * @param arguments compiler command-line arguments
     * @return list of include directories for the compiler
     */
    virtual KDevelop::Path::List includes(Utils::LanguageType type, const QString& arguments) const = 0;

    void setPath( const QString &path );

    /// @return path to the compiler
    QString path() const;

    void setName( const QString &name );

    /// @return user visible name
    QString name() const;

    /// Indicates if the compiler name/path can be set manually
    bool editable() const;

    /// @return name of the factory that created this compiler
    QString factoryName() const;

    virtual ~ICompiler() = default;

private:
    bool m_editable;
    QString m_name;
    QString m_path;
    QString m_factoryName;
};

using CompilerPointer = QSharedPointer<ICompiler>;

Q_DECLARE_METATYPE(CompilerPointer)

#endif // ICOMPILER_H
