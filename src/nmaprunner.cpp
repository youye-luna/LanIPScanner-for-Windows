// nmaprunner.cpp —— 调用官方 nmap 子进程完成主机发现
//
// 说明：nmap 与 GPLv3 不兼容（NPSL 许可），本项目按用户要求在 release 内随附
//       nmap 自带的许可文件（release/nmap/LICENSE），此处仅以独立子进程方式调用。
//
// nmap 的 -oX - 会把 XML 写到标准输出，--stats-every 让 nmap 周期性地在 XML 中
// 插入 <taskprogress ... percent="xx.xx"/>，因此进度可以直接从标准输出解析，
// 无需解析标准错误。

#include "nmaprunner.h"

#include "scanner.h" // ScanCancelToken

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QXmlStreamReader>

namespace
{
/// 传给 nmap 的 TCP 探测端口（无 Npcap 时 nmap 会退化为 connect() 模式）
const char *kProbePorts = "80,443,445,22,8080";
/// 单次 nmap 调用的硬超时，避免子进程异常时长时间卡住
const int kScanTimeoutMs = 20 * 60 * 1000;
/// 等待子进程退出的时间
const int kFinishTimeoutMs = 5000;

/// nmap 输出 AA:BB:CC:DD:EE:FF，界面统一使用 AA-BB-CC-DD-EE-FF
QString normalizeMac(const QString &raw)
{
    QString mac = raw.toUpper();
    mac.replace(QLatin1Char(':'), QLatin1Char('-'));
    return mac;
}

/// 解析 nmap 的 XML 输出，只保留在线主机
QVector<NmapHost> parseXml(const QByteArray &xml, bool *parsed)
{
    QVector<NmapHost> hosts;
    *parsed = false;

    QXmlStreamReader reader(xml);
    NmapHost current;
    bool inHost = false;
    bool hostUp = false;

    while (!reader.atEnd())
    {
        reader.readNext();
        if (reader.isStartElement())
        {
            const QString name = reader.name().toString();

            if (name == QLatin1String("host"))
            {
                current = NmapHost();
                inHost = true;
                hostUp = false;
            }
            else if (!inHost)
            {
                continue;
            }
            else if (name == QLatin1String("status"))
            {
                hostUp = reader.attributes().value(QLatin1String("state")) == QLatin1String("up");
            }
            else if (name == QLatin1String("address"))
            {
                const QString addressType = reader.attributes().value(QLatin1String("addrtype")).toString();
                const QString address = reader.attributes().value(QLatin1String("addr")).toString();
                if (addressType == QLatin1String("ipv4"))
                    current.ipAddress = address;
                else if (addressType == QLatin1String("mac"))
                    current.macAddress = normalizeMac(address);
            }
            else if (name == QLatin1String("hostname"))
            {
                // nmap 用 -n 时不解析主机名，这里只在有值时采用
                if (current.hostName.isEmpty())
                    current.hostName = reader.attributes().value(QLatin1String("name")).toString();
            }
            else if (name == QLatin1String("times"))
            {
                bool ok = false;
                const double microSeconds = reader.attributes().value(QLatin1String("srtt")).toDouble(&ok);
                if (ok && microSeconds >= 0)
                    current.rttMs = static_cast<qint64>(microSeconds / 1000.0); // 微秒 → 毫秒
            }
        }
        else if (reader.isEndElement() && reader.name() == QLatin1String("host"))
        {
            if (inHost && hostUp && !current.ipAddress.isEmpty())
                hosts.append(current);
            inHost = false;
        }
    }

    *parsed = !reader.hasError();
    return hosts;
}
} // namespace

QString NmapRunner::findNmapExecutable()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + QStringLiteral("/nmap/nmap.exe"),
        QDir::cleanPath(appDir + QStringLiteral("/../nmap/nmap.exe")),
        QDir::cleanPath(appDir + QStringLiteral("/../release/nmap/nmap.exe"))};

    for (const QString &candidate : candidates)
    {
        const QFileInfo info(candidate);
        if (info.isFile())
            return QDir::toNativeSeparators(info.absoluteFilePath());
    }

    const QString fromPath = QStandardPaths::findExecutable(QStringLiteral("nmap"));
    return fromPath.isEmpty() ? QString() : QDir::toNativeSeparators(fromPath);
}

QVector<NmapHost> NmapRunner::scanHosts(const QStringList &ipList,
                                        const std::function<void(int)> &onProgress,
                                        const ScanCancelToken *token,
                                        QString *errorMessage)
{
    QVector<NmapHost> hosts;
    if (ipList.isEmpty())
        return hosts;

    const QString exePath = findNmapExecutable();
    if (exePath.isEmpty())
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("未找到 nmap.exe，请确认程序目录下的 nmap 子目录完整");
        return hosts;
    }

    const QStringList arguments = {
        QStringLiteral("-sn"),                        // 只做主机发现
        QStringLiteral("-n"),                         // 不做反向 DNS（交给 NetUtils 解析，避免 nmap 侧等待）
        QStringLiteral("-oX"), QStringLiteral("-"),   // XML 输出到标准输出
        QStringLiteral("--stats-every"), QStringLiteral("1s"),
        QStringLiteral("-PS%1").arg(QLatin1String(kProbePorts)),
        QStringLiteral("-iL"), QStringLiteral("-")};  // 目标列表从标准输入读取，避免命令行过长

    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.setWorkingDirectory(QFileInfo(exePath).absolutePath());
    process.start(exePath, arguments, QIODevice::ReadWrite);
    if (!process.waitForStarted(kFinishTimeoutMs))
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法启动 nmap：%1").arg(process.errorString());
        return hosts;
    }

    process.write(ipList.join(QLatin1Char('\n')).toLatin1());
    process.write("\n");
    process.closeWriteChannel();

    static const QRegularExpression progressPattern(
        QStringLiteral("<taskprogress[^>]*percent=\"([0-9.]+)\""));

    QByteArray xml;
    QByteArray standardError;
    QElapsedTimer timer;
    timer.start();

    while (process.state() != QProcess::NotRunning)
    {
        if (token && token->isCancelled())
        {
            process.kill();
            process.waitForFinished(kFinishTimeoutMs);
            return hosts;
        }
        if (timer.elapsed() > kScanTimeoutMs)
        {
            process.kill();
            process.waitForFinished(kFinishTimeoutMs);
            if (errorMessage)
                *errorMessage = QStringLiteral("nmap 扫描超时，请缩小扫描范围后重试");
            return hosts;
        }

        if (!process.waitForReadyRead(200))
            continue;

        const QByteArray chunk = process.readAllStandardOutput();
        if (!chunk.isEmpty())
        {
            xml += chunk;

            // 只在新到达的数据里取最后一次进度，避免反复扫描整段 XML
            QRegularExpressionMatch lastMatch;
            QRegularExpressionMatchIterator matches = progressPattern.globalMatch(QString::fromLatin1(chunk));
            while (matches.hasNext())
                lastMatch = matches.next();
            if (lastMatch.hasMatch() && onProgress)
                onProgress(qBound(0, static_cast<int>(lastMatch.captured(1).toDouble()), 100));
        }

        // 及时排空标准错误，否则 nmap 的告警信息可能填满管道缓冲区导致阻塞
        standardError += process.readAllStandardError();
        if (standardError.size() > 8192)
            standardError = standardError.right(8192);
    }

    xml += process.readAllStandardOutput();
    standardError += process.readAllStandardError();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
    {
        if (errorMessage)
        {
            const QString detail = QString::fromLocal8Bit(standardError).trimmed();
            *errorMessage = detail.isEmpty()
                                ? QStringLiteral("nmap 进程异常退出（代码 %1）").arg(process.exitCode())
                                : QStringLiteral("nmap 执行失败：%1").arg(detail);
        }
        return hosts;
    }

    bool parsed = false;
    const QVector<NmapHost> result = parseXml(xml, &parsed);
    if (!parsed)
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("nmap 输出解析失败");
        return hosts;
    }

    return result;
}
