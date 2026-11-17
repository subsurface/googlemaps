#include "qgeomapreplygooglemaps.h"
#include <QNetworkAccessManager>
#include <QNetworkCacheMetaData>
#include <QDateTime>

QT_BEGIN_NAMESPACE

QGeoMapReplyGooglemaps::QGeoMapReplyGooglemaps(QNetworkReply *reply, const QGeoTileSpec &spec, QObject *parent)
        : QGeoTiledMapReply(spec, parent),
        m_reply(reply)
{
    connect(m_reply, &QNetworkReply::finished, this, &QGeoMapReplyGooglemaps::networkFinished);
    connect(m_reply, &QNetworkReply::errorOccurred, this, &QGeoMapReplyGooglemaps::networkError);
}

QGeoMapReplyGooglemaps::~QGeoMapReplyGooglemaps()
{
}

QNetworkReply *QGeoMapReplyGooglemaps::networkReply() const
{
    return m_reply;
}

void QGeoMapReplyGooglemaps::abort()
{
    if (!m_reply)
        return;

    m_reply->abort();
}

void QGeoMapReplyGooglemaps::networkFinished()
{
    if (!m_reply)
        return;

    // Failures are reported by networkError(), which runs before this slot.
    if (m_reply->error() != QNetworkReply::NoError)
        return;

    // Only a 2xx response with a non-empty body is a usable tile. Anything
    // else (e.g. an unfollowed redirect or a 204) must be reported as an
    // error, otherwise QtLocation treats it as a successfully fetched tile.
    const QVariant statusAttribute = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    const int status = statusAttribute.toInt();
    const QByteArray data = m_reply->readAll();
    const bool statusOk = !statusAttribute.isValid() || (status >= 200 && status < 300);

    if (!statusOk || data.isEmpty()) {
        const QString message = statusOk
                ? QStringLiteral("Empty tile response")
                : QStringLiteral("Unexpected HTTP status %1").arg(status);
        m_reply->deleteLater();
        m_reply = 0;
        setCached(false);
        setError(QGeoTiledMapReply::CommunicationError, message);
        return;
    }

    setMapImageData(data);
    const int _mid = tileSpec().mapId();
    if (_mid == 2)
        setMapImageFormat("jpeg");
    else
        setMapImageFormat("png");
    setFinished(true);

    m_reply->deleteLater();
    m_reply = 0;
}

void QGeoMapReplyGooglemaps::networkError(QNetworkReply::NetworkError error)
{
    if (!m_reply)
        return;

    const QString message = m_reply->errorString();
    m_reply->deleteLater();
    m_reply = 0;

    setCached(false);

    // A cancelled request (see abort()) is not a failure; QtLocation has
    // already dropped the tile request, so just complete the reply.
    if (error == QNetworkReply::OperationCanceledError)
        setFinished(true);
    else
        setError(QGeoTiledMapReply::CommunicationError, message);
}

QT_END_NAMESPACE
