// SPDX-FileCopyrightText: 2026 OpenUaSim contributors
// SPDX-License-Identifier: MIT

///
/// \file appicons.cpp
/// \brief Implements the theme-aware application icons.
///

#include <QCoreApplication>
#include <QFile>
#include <QGuiApplication>
#include <QHash>
#include <QIconEngine>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QSvgRenderer>

#include "appcolors.h"
#include "appicons.h"

namespace {

const QByteArray kCurrentColor = QByteArrayLiteral("currentColor");

///
/// \brief Where an icon takes the colour that replaces \c currentColor.
///
enum class Tint { Palette, Status };

///
/// \brief Icon engine rendering an SVG whose \c currentColor is resolved at paint time.
///
class TintedSvgEngine : public QIconEngine
{
public:
    TintedSvgEngine(const QByteArray &svg, Tint tint, StatusKind status)
        : _svg(svg), _tint(tint), _status(status)
    {
    }

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) override;
    QIconEngine *clone() const override;
    bool isNull() override;

private:
    QColor colorFor(QIcon::Mode mode) const;
    QByteArray renderedSvg(QIcon::Mode mode) const;

    QByteArray _svg;
    Tint _tint;
    StatusKind _status;
};

///
/// \brief Draws the icon directly with the painter, keeping it vector-sharp.
/// \param painter Target painter.
/// \param rect Target rectangle.
/// \param mode Icon mode selecting the colour.
/// \param state Unused.
///
void TintedSvgEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(state)
    QSvgRenderer renderer(renderedSvg(mode));
    renderer.render(painter, rect);
}

///
/// \brief Renders the icon at device-independent size.
/// \param size Pixmap size.
/// \param mode Icon mode selecting the colour.
/// \param state Icon state.
/// \return Rendered pixmap.
///
QPixmap TintedSvgEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    return scaledPixmap(size, mode, state, 1.0);
}

///
/// \brief Renders the icon for a device pixel ratio.
/// \param size Logical pixmap size.
/// \param mode Icon mode selecting the colour.
/// \param state Unused.
/// \param scale Device pixel ratio.
/// \return Rendered pixmap carrying the device pixel ratio.
///
QPixmap TintedSvgEngine::scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale)
{
    Q_UNUSED(state)
    QImage image(size * scale, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    QSvgRenderer renderer(renderedSvg(mode));
    renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(image.size())));
    painter.end();

    QPixmap result = QPixmap::fromImage(image);
    result.setDevicePixelRatio(scale);
    return result;
}

///
/// \brief Copies the engine for a detached QIcon.
/// \return New engine with the same SVG and tint.
///
QIconEngine *TintedSvgEngine::clone() const
{
    return new TintedSvgEngine(_svg, _tint, _status);
}

///
/// \brief Reports whether there is anything to draw.
/// \return True when the SVG source is empty.
///
bool TintedSvgEngine::isNull()
{
    return _svg.isEmpty();
}

///
/// \brief Resolves the colour replacing \c currentColor.
/// \param mode Icon mode; disabled palette icons use the disabled text colour.
/// \return Colour to render with.
///
QColor TintedSvgEngine::colorFor(QIcon::Mode mode) const
{
    if (_tint == Tint::Status)
        return AppColors::status(_status);
    const QPalette palette = QGuiApplication::palette();
    if (mode == QIcon::Disabled)
        return palette.color(QPalette::Disabled, QPalette::WindowText);
    if (mode == QIcon::Selected)
        return palette.color(QPalette::HighlightedText);
    return palette.color(QPalette::WindowText);
}

///
/// \brief Returns the SVG source with \c currentColor substituted.
/// \param mode Icon mode selecting the colour.
/// \return SVG ready for QSvgRenderer.
///
QByteArray TintedSvgEngine::renderedSvg(QIcon::Mode mode) const
{
    QByteArray svg = _svg;
    return svg.replace(kCurrentColor, colorFor(mode).name(QColor::HexRgb).toLatin1());
}

///
/// \brief Reads an icon resource, cached because delegates request icons on every paint.
/// \param name Resource name without directory and extension.
/// \return SVG source, or an empty array when the resource does not exist.
///
QByteArray readSvg(const QString &name)
{
    static QHash<QString, QByteArray> cache;
    const auto cached = cache.constFind(name);
    if (cached != cache.cend())
        return *cached;

    QFile file(QStringLiteral(":/icons/%1.svg").arg(name));
    const QByteArray svg = file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    cache.insert(name, svg);
    return svg;
}

///
/// \brief Wraps an SVG source into a tinted icon.
/// \param svg SVG source.
/// \param tint Colour source.
/// \param status Status used with Tint::Status.
/// \return Icon; null when the source is empty.
///
QIcon tinted(const QByteArray &svg, Tint tint, StatusKind status = StatusKind::None)
{
    if (svg.isEmpty())
        return {};
    return QIcon(new TintedSvgEngine(svg, tint, status));
}

}

namespace AppIcons {

///
/// \brief Builds an icon whose outline follows the palette text colour.
/// \param name Resource name under ":/icons/", without the ".svg" extension.
/// \return Icon; null when the resource does not exist.
///
QIcon icon(const QString &name)
{
    return tinted(readSvg(name), Tint::Palette);
}

///
/// \brief Builds the round indicator icon of a status.
/// \param kind Status to show.
/// \return Filled dot in the status colour; null for StatusKind::None.
///
QIcon statusDot(StatusKind kind)
{
    if (kind == StatusKind::None)
        return {};
    static const QByteArray dot = QByteArrayLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\">"
        "<circle cx=\"12\" cy=\"12\" r=\"9.5\" fill=\"currentColor\"/></svg>");
    return tinted(dot, Tint::Status, kind);
}

///
/// \brief Lists the icons a variable can be given.
/// \return Icons in picker order.
///
QList<NamedIcon> variableIcons()
{
    const auto named = [](const char *name, const char *label) {
        return NamedIcon{ QString::fromLatin1(name), QCoreApplication::translate("AppIcons", label) };
    };
    return {
        named("signal", QT_TRANSLATE_NOOP("AppIcons", "Signal")),
        named("gauge", QT_TRANSLATE_NOOP("AppIcons", "Gauge")),
        named("thermometer", QT_TRANSLATE_NOOP("AppIcons", "Temperature")),
        named("droplet", QT_TRANSLATE_NOOP("AppIcons", "Liquid")),
        named("zap", QT_TRANSLATE_NOOP("AppIcons", "Electrical")),
        named("clock", QT_TRANSLATE_NOOP("AppIcons", "Time")),
        named("hash", QT_TRANSLATE_NOOP("AppIcons", "Counter")),
        named("text", QT_TRANSLATE_NOOP("AppIcons", "Text")),
        named("status-good", QT_TRANSLATE_NOOP("AppIcons", "Active")),
        named("status-inactive", QT_TRANSLATE_NOOP("AppIcons", "Inactive")),
    };
}

}
