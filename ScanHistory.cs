using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Text.Json;

namespace DhcpScanner
{
    /// <summary>
    /// 扫描历史中的设备记录（可序列化存储）
    /// </summary>
    public class ScanHistoryDevice
    {
        public string IpAddress { get; set; } = string.Empty;
        public string MacAddress { get; set; } = string.Empty;
        public string HostName { get; set; } = string.Empty;
        public long PingMs { get; set; }
        public bool IsActive { get; set; }
        public bool IsDhcpServer { get; set; }
        public DateTime ResponseTime { get; set; }

        /// <summary>
        /// 从扫描结果转换
        /// </summary>
        public static ScanHistoryDevice From(DhcpServerInfo info) => new()
        {
            IpAddress = info.IpAddress?.ToString() ?? string.Empty,
            MacAddress = info.MacAddress ?? string.Empty,
            HostName = info.HostName ?? string.Empty,
            PingMs = info.PingMs,
            IsActive = info.IsActive,
            IsDhcpServer = info.IsDhcpServer,
            ResponseTime = info.ResponseTime
        };

        /// <summary>
        /// 还原为扫描结果对象
        /// </summary>
        public DhcpServerInfo ToServerInfo() => new()
        {
            IpAddress = IPAddress.TryParse(IpAddress, out var ip) ? ip : IPAddress.None,
            MacAddress = MacAddress,
            HostName = HostName,
            PingMs = PingMs,
            IsActive = IsActive,
            IsDhcpServer = IsDhcpServer,
            ResponseTime = ResponseTime
        };
    }

    /// <summary>
    /// 一次扫描的历史记录
    /// </summary>
    public class ScanHistoryRecord
    {
        public Guid Id { get; set; } = Guid.NewGuid();
        public DateTime ScanTime { get; set; } = DateTime.Now;
        public string StartIp { get; set; } = string.Empty;
        public string EndIp { get; set; } = string.Empty;
        public List<ScanHistoryDevice> Devices { get; set; } = new();
        public string FilePath { get; set; } = string.Empty;
    }

    /// <summary>
    /// 扫描历史持久化 —— 每次扫描保存为独立 CSV 文件，存储在安装目录下的 ScanHistory 文件夹中。
    /// 文件名格式：2026年08月05日 192.168.1.1~192.168.1.254（IP）.csv
    /// 文件头存储扫描元信息（起始IP、结束IP、时间），正文为设备数据。
    /// </summary>
    public static class ScanHistoryStore
    {
        private static readonly string HistoryDir = Path.Combine(AppContext.BaseDirectory, "ScanHistory");
        private const string MetaPrefix = "# ScanHistory v1";
        private const string MetaTimePrefix = "# Time:";
        private const string MetaStartIpPrefix = "# StartIp:";
        private const string MetaEndIpPrefix = "# EndIp:";

        /// <summary>
        /// 加载全部历史记录（按时间倒序，并按当前保存配置清理过期记录）
        /// </summary>
        public static List<ScanHistoryRecord> Load()
        {
            var records = ApplyRetention(ReadAll());
            return records;
        }

        /// <summary>
        /// 新增一条历史记录（保存 CSV 文件并按当前保存配置清理）
        /// </summary>
        public static void Save(ScanHistoryRecord record)
        {
            string targetPath = string.Empty;
            try
            {
                EnsureDir();
                string fileName = BuildFileName(record.ScanTime, record.StartIp, record.EndIp);
                string filePath = Path.Combine(HistoryDir, fileName);
                targetPath = filePath;

                // 同名文件处理（同秒多次扫描）
                if (File.Exists(filePath))
                {
                    int seq = 2;
                    string baseName = Path.GetFileNameWithoutExtension(fileName);
                    while (File.Exists(filePath))
                    {
                        filePath = Path.Combine(HistoryDir, $"{baseName}({seq}).csv");
                        seq++;
                    }
                }

                WriteCsv(filePath, record);
                record.FilePath = filePath;
                targetPath = filePath;
                Prune();
            }
            catch (Exception ex)
            {
            }
        }

        /// <summary>
        /// 按当前保存配置立即清理过期历史记录（保存配置变更后调用）
        /// </summary>
        public static void Prune()
        {
            try
            {
                var records = ReadAll();
                var retained = ApplyRetention(records);
                var retainedPaths = new HashSet<string>(
                    retained.Where(r => !string.IsNullOrEmpty(r.FilePath)).Select(r => r.FilePath));
                // 删除未保留的文件
                foreach (var r in records)
                {
                    if (!retainedPaths.Contains(r.FilePath) && !string.IsNullOrEmpty(r.FilePath) && File.Exists(r.FilePath))
                        File.Delete(r.FilePath);
                }
            }
            catch { }
        }

        /// <summary>
        /// 删除一条历史记录（删除 CSV 文件）
        /// </summary>
        public static void Delete(ScanHistoryRecord record)
        {
            try
            {
                if (!string.IsNullOrEmpty(record.FilePath) && File.Exists(record.FilePath))
                    File.Delete(record.FilePath);
            }
            catch { }
        }

        /// <summary>
        /// 清空所有历史记录（删除 ScanHistory 文件夹下所有 CSV）
        /// </summary>
        public static void Clear()
        {
            try
            {
                if (Directory.Exists(HistoryDir))
                    Directory.Delete(HistoryDir, true);
            }
            catch { }
        }

        /// <summary>
        /// 读取 ScanHistory 文件夹下所有 CSV 文件并解析为记录（按时间倒序）
        /// </summary>
        private static List<ScanHistoryRecord> ReadAll()
        {
            var list = new List<ScanHistoryRecord>();
            try
            {
                if (!Directory.Exists(HistoryDir))
                    return list;

                foreach (var file in Directory.GetFiles(HistoryDir, "*.csv"))
                {
                    var record = ReadCsv(file);
                    if (record != null)
                        list.Add(record);
                }
            }
            catch { }
            return list.OrderByDescending(r => r.ScanTime).ToList();
        }

        /// <summary>
        /// 根据设置的保存策略筛选应保留的记录列表（不删除文件）
        /// </summary>
        private static List<ScanHistoryRecord> ApplyRetention(List<ScanHistoryRecord> list)
        {
            if (list.Count == 0)
                return list;

            var settings = AppSettings.Load();
            if (settings.HistorySaveMode == HistorySaveMode.ByCount)
            {
                return list.Take(settings.HistorySaveMaxRecords).ToList();
            }
            else if (settings.HistorySaveDays <= 0)
            {
                // 永不清除
                return list;
            }
            else
            {
                DateTime cutoff = DateTime.Now.AddDays(-settings.HistorySaveDays);
                return list.Where(r => r.ScanTime >= cutoff).ToList();
            }
        }

        /// <summary>
        /// 生成文件名：2026年08月05日 192.168.1.1~192.168.1.254（IP）.csv
        /// </summary>
        private static string BuildFileName(DateTime time, string startIp, string endIp)
        {
            string dateStr = time.ToString("yyyy年MM月dd日");
            string name = $"{dateStr} {startIp}~{endIp}（IP）";
            // 清理文件名中不允许的字符（IP地址不应有，但防御性处理）
            foreach (char c in Path.GetInvalidFileNameChars())
                name = name.Replace(c, '_');
            return name + ".csv";
        }

        /// <summary>
        /// 从文件名尝试解析扫描时间和 IP 范围
        /// </summary>
        private static (DateTime time, string startIp, string endIp)? ParseFileName(string filePath)
        {
            try
            {
                string name = Path.GetFileNameWithoutExtension(filePath);
                // 格式：2026年08月05日 192.168.1.1~192.168.1.254（IP）
                // 去掉末尾的（IP）
                if (name.EndsWith("（IP）"))
                    name = name.Substring(0, name.Length - "（IP）".Length);

                int spaceIdx = name.IndexOf(' ');
                if (spaceIdx < 0) return null;

                string datePart = name.Substring(0, spaceIdx);
                string ipPart = name.Substring(spaceIdx + 1);

                if (DateTime.TryParseExact(datePart, "yyyy年MM月dd日",
                    CultureInfo.InvariantCulture, DateTimeStyles.None, out var time))
                {
                    var ips = ipPart.Split('~');
                    if (ips.Length == 2)
                        return (time, ips[0], ips[1]);
                }
            }
            catch { }
            return null;
        }

        /// <summary>
        /// 写入 CSV 文件：头部为元信息注释行，正文为设备数据
        /// </summary>
        private static void WriteCsv(string filePath, ScanHistoryRecord record)
        {
            using var writer = new StreamWriter(filePath, false, Encoding.UTF8);

            // 元信息（注释行，供读取时还原 ScanHistoryRecord）
            writer.WriteLine(MetaPrefix);
            writer.WriteLine($"{MetaTimePrefix}{record.ScanTime:O}");
            writer.WriteLine($"{MetaStartIpPrefix}{record.StartIp}");
            writer.WriteLine($"{MetaEndIpPrefix}{record.EndIp}");

            // 表头
            writer.WriteLine(Lang.Get("CsvHeader"));

            // 设备数据
            foreach (var d in record.Devices)
            {
                string subnet = string.Join(".", d.IpAddress.Split('.').Take(3));
                string active = d.IsActive ? Lang.Get("Online") : Lang.Get("NoDevice");
                string ping = d.IsActive ? d.PingMs.ToString() : "-";
                string dhcp = d.IsDhcpServer ? Lang.Get("Yes") : Lang.Get("No");

                writer.WriteLine(string.Join(",",
                    EscapeCsvField(subnet),
                    EscapeCsvField(d.IpAddress),
                    EscapeCsvField(d.MacAddress),
                    EscapeCsvField(d.HostName),
                    EscapeCsvField(ping),
                    EscapeCsvField(dhcp),
                    EscapeCsvField(active)));
            }
        }

        /// <summary>
        /// 读取 CSV 文件，解析为 ScanHistoryRecord
        /// </summary>
        private static ScanHistoryRecord? ReadCsv(string filePath)
        {
            try
            {
                var lines = File.ReadAllLines(filePath, Encoding.UTF8);
                if (lines.Length == 0) return null;

                var record = new ScanHistoryRecord { FilePath = filePath };
                int dataStart = 0;

                // 解析元信息注释行
                for (int i = 0; i < lines.Length; i++)
                {
                    string line = lines[i];
                    if (line.StartsWith(MetaTimePrefix))
                    {
                        if (DateTime.TryParse(line.Substring(MetaTimePrefix.Length), CultureInfo.InvariantCulture,
                            DateTimeStyles.RoundtripKind, out var t))
                            record.ScanTime = t;
                        dataStart = i + 1;
                    }
                    else if (line.StartsWith(MetaStartIpPrefix))
                    {
                        record.StartIp = line.Substring(MetaStartIpPrefix.Length);
                        dataStart = i + 1;
                    }
                    else if (line.StartsWith(MetaEndIpPrefix))
                    {
                        record.EndIp = line.Substring(MetaEndIpPrefix.Length);
                        dataStart = i + 1;
                    }
                    else if (line.StartsWith("#"))
                    {
                        dataStart = i + 1;
                    }
                    else
                    {
                        break;
                    }
                }

                // 跳过表头行（如果有）
                if (dataStart < lines.Length && !string.IsNullOrWhiteSpace(lines[dataStart]))
                    dataStart++;

                // 旧版无元信息文件 → 从文件名解析
                if (string.IsNullOrEmpty(record.StartIp) || string.IsNullOrEmpty(record.EndIp))
                {
                    var parsed = ParseFileName(filePath);
                    if (parsed.HasValue)
                    {
                        record.ScanTime = parsed.Value.time;
                        record.StartIp = parsed.Value.startIp;
                        record.EndIp = parsed.Value.endIp;
                    }
                }

                // 解析设备数据行
                for (int i = dataStart; i < lines.Length; i++)
                {
                    string line = lines[i];
                    if (string.IsNullOrWhiteSpace(line)) continue;

                    var fields = ParseCsvLine(line);
                    if (fields.Count < 7) continue;

                    // 字段顺序：网段,IP地址,MAC地址,主机名,延迟(ms),DHCP服务器,状态
                    bool isActive = fields[6] == Lang.Get("Online") || fields[6] == "在线" || fields[6] == "Online" || fields[6] == "線上";
                    bool isDhcp = fields[5] == Lang.Get("Yes") || fields[5] == "是" || fields[5] == "Yes";
                    long ping = -1;
                    if (fields[4] != "-" && long.TryParse(fields[4], out var p))
                        ping = p;

                    record.Devices.Add(new ScanHistoryDevice
                    {
                        IpAddress = fields[1],
                        MacAddress = fields[2],
                        HostName = fields[3],
                        PingMs = ping,
                        IsActive = isActive,
                        IsDhcpServer = isDhcp,
                        ResponseTime = record.ScanTime
                    });
                }

                return record;
            }
            catch (Exception ex)
            {
            }
            return null;
        }

        /// <summary>
        /// 解析一行 CSV（处理引号内的逗号）
        /// </summary>
        private static List<string> ParseCsvLine(string line)
        {
            var fields = new List<string>();
            bool inQuotes = false;
            var current = new StringBuilder();

            for (int i = 0; i < line.Length; i++)
            {
                char c = line[i];
                if (c == '"')
                {
                    if (inQuotes && i + 1 < line.Length && line[i + 1] == '"')
                    {
                        current.Append('"');
                        i++;
                    }
                    else
                    {
                        inQuotes = !inQuotes;
                    }
                }
                else if (c == ',' && !inQuotes)
                {
                    fields.Add(current.ToString());
                    current.Clear();
                }
                else
                {
                    current.Append(c);
                }
            }
            fields.Add(current.ToString());
            return fields;
        }

        private static long GetHistoryLength()
        {
            try
            {
                return Directory.Exists(HistoryDir)
                    ? Directory.GetFiles(HistoryDir, "*.csv").Sum(f => new FileInfo(f).Length)
                    : 0;
            }
            catch
            {
                return 0;
            }
        }

        private static string EscapeCsvField(string field)
        {
            if (field.Contains(",") || field.Contains("\"") || field.Contains("\n"))
                return $"\"{field.Replace("\"", "\"\"")}\"";
            return field;
        }

        private static void EnsureDir()
        {
            if (!Directory.Exists(HistoryDir))
                Directory.CreateDirectory(HistoryDir);
        }
    }
}
