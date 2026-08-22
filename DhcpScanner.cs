using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;
using System.Text.RegularExpressions;

namespace DhcpScanner
{
    /// <summary>
    /// 扫描结果信息
    /// </summary>
    public class DhcpServerInfo
    {
        public IPAddress IpAddress { get; set; }
        public string MacAddress { get; set; }
        public string HostName { get; set; }
        public DateTime ResponseTime { get; set; }
        public bool IsActive { get; set; }
        public bool IsDhcpServer { get; set; }
        public long PingMs { get; set; }

        public DhcpServerInfo()
        {
            IpAddress = IPAddress.None;
            MacAddress = string.Empty;
            HostName = string.Empty;
            ResponseTime = DateTime.Now;
            IsActive = false;
            IsDhcpServer = false;
            PingMs = 0;
        }
    }

    /// <summary>
    /// 网络扫描器
    /// </summary>
    public class DhcpScanner
    {
        private readonly List<DhcpServerInfo> _discoveredServers;
        private readonly ConcurrentDictionary<string, Lazy<Task<string>>> _macCache = new();
        private readonly object _lock = new();
        private CancellationTokenSource _cancellationTokenSource;
        private bool _isScanning;

        public event EventHandler<DhcpServerInfo>? ServerFound;
        public event EventHandler<int>? ScanProgress;
        public event EventHandler<List<DhcpServerInfo>>? ScanCompleted;
        public event EventHandler<string>? ScanError;

        public DhcpScanner()
        {
            _discoveredServers = new List<DhcpServerInfo>();
            _cancellationTokenSource = new CancellationTokenSource();
            _isScanning = false;
        }

        public IReadOnlyList<DhcpServerInfo> DiscoveredServers => _discoveredServers.AsReadOnly();
        public bool IsScanning => _isScanning;

        /// <summary>
        /// 扫描并发线程数
        /// </summary>
        public int MaxParallelism { get; set; } = 30;

        /// <summary>
        /// 开始扫描网络设备
        /// </summary>
        public async Task StartScanAsync(string ipRange, int startRange = 1, int endRange = 254)
        {
            if (_isScanning)
                throw new InvalidOperationException("扫描已在进行中");

            _isScanning = true;
            _cancellationTokenSource = new CancellationTokenSource();
            _discoveredServers.Clear();
            _macCache.Clear();

            try
            {
                int total = endRange - startRange + 1;
                int completed = 0;
                var allResults = new List<DhcpServerInfo>();
                var resultsLock = new object();

                // 多线程并行扫描
                await Parallel.ForEachAsync(
                    Enumerable.Range(startRange, total),
                    new ParallelOptions
                    {
                        MaxDegreeOfParallelism = MaxParallelism,
                        CancellationToken = _cancellationTokenSource.Token
                    },
                    async (i, token) =>
                    {
                        string ip = $"{ipRange}.{i}";
                        var result = await ScanSingleIpAsync(ip, token);

                        lock (resultsLock)
                        {
                            allResults.Add(result);
                        }

                        // 更新进度
                        int current = Interlocked.Increment(ref completed);
                        int progress = (int)((double)current / total * 100);
                        ScanProgress?.Invoke(this, progress);
                    });

                await ApplyArpResultsAsync(allResults, _cancellationTokenSource.Token);

                allResults.Sort((a, b) =>
                {
                    string aLast = a.IpAddress.ToString().Split('.').Last();
                    string bLast = b.IpAddress.ToString().Split('.').Last();
                    if (int.TryParse(aLast, out int aNum) && int.TryParse(bLast, out int bNum))
                        return aNum.CompareTo(bNum);
                    return string.Compare(a.IpAddress.ToString(), b.IpAddress.ToString());
                });

                // 一次性通知所有结果
                lock (_lock)
                {
                    _discoveredServers.Clear();
                    _discoveredServers.AddRange(allResults);
                }
                foreach (var r in allResults)
                {
                    ServerFound?.Invoke(this, r);
                }

                ScanCompleted?.Invoke(this, allResults);
            }
            catch (OperationCanceledException)
            {
                // 用户取消
            }
            catch (Exception ex)
            {
                ScanError?.Invoke(this, ex.Message);
            }
            finally
            {
                _isScanning = false;
            }
        }

        /// <summary>
        /// 扫描单个IP
        /// </summary>
        private async Task<DhcpServerInfo> ScanSingleIpAsync(string ip, CancellationToken cancellationToken)
        {
            var serverInfo = new DhcpServerInfo { IpAddress = IPAddress.Parse(ip) };
            var activePorts = new HashSet<int>();

            try
            {
                PingReply? reply = null;
                // 保持与旧版相同的一次 500ms ICMP 探测；同网段漏报由扫描结束时的
                // ARP 邻居表补齐，避免对每个空地址重复等待。
                for (int attempt = 0; attempt < 1; attempt++)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    reply = await SendPingAsync(ip, 500, cancellationToken);
                    if (reply?.Status == IPStatus.Success)
                        break;
                }

                serverInfo.ResponseTime = DateTime.Now;
                serverInfo.IsActive = reply?.Status == IPStatus.Success;
                serverInfo.PingMs = reply?.Status == IPStatus.Success ? reply.RoundtripTime : -1;

                // ICMP 可能被防火墙阻断；仅对 Ping 失败的地址做有限 TCP 存活探测。
                if (!serverInfo.IsActive)
                {
                    // 只探测最常见的服务端口。端口拒绝同样能证明主机在线，其他设备
                    // 则由 ARP 补偿，避免 9 个端口 × 300ms 将扫描时间放大数倍。
                    int[] probePorts = { 80, 443, 445, 22, 8080 };
                    using var probeCts = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
                    using var probeSemaphore = new SemaphoreSlim(4);
                    var probeTasks = probePorts.Select(port => ProbePortAsync(ip, port, 120, probeSemaphore, probeCts.Token)).ToArray();
                    try
                    {
                        while (probeTasks.Length > 0)
                        {
                            cancellationToken.ThrowIfCancellationRequested();
                            var completed = await Task.WhenAny(probeTasks);
                            var probeResult = await completed;
                            if (probeResult.HasValue)
                            {
                                // 正数表示端口已连接；负数表示收到 TCP RST（端口关闭但主机明确在线）。
                                if (probeResult.Value > 0)
                                    activePorts.Add(probeResult.Value);
                                serverInfo.IsActive = true;
                                probeCts.Cancel();
                                break;
                            }
                            probeTasks = probeTasks.Where(task => !task.IsCompleted).ToArray();
                        }
                    }
                    finally
                    {
                        probeCts.Cancel();
                        try { await Task.WhenAll(probeTasks); } catch (OperationCanceledException) { }
                    }
                }

                if (serverInfo.IsActive)
                {
                    var macTask = GetMacAddressAsync(ip);
                    var hostTask = GetHostNameAsync(ip, cancellationToken);
                    await Task.WhenAll(macTask, hostTask).ConfigureAwait(false);
                    serverInfo.MacAddress = await macTask;
                    serverInfo.HostName = await hostTask;
                    serverInfo.IsDhcpServer = await IsLikelyRouterOrDhcp(ip, serverInfo.HostName, activePorts, cancellationToken);
                }
            }
            catch (OperationCanceledException)
            {
                throw;
            }
            catch
            {
                // 忽略单个IP扫描失败
            }

            return serverInfo;
        }

        /// <summary>
        /// 使用本次扫描产生的 ARP 邻居表补充在线设备。ICMP 可能被设备防火墙拦截，
        /// 但同网段通信仍会完成 ARP，因此这是比 TCP 端口猜测更可靠的补充信号。
        /// </summary>
        private async Task ApplyArpResultsAsync(List<DhcpServerInfo> results, CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            var arpTable = ReadArpTable();
            if (arpTable.Count == 0)
                return;

            var arpDevices = results
                .Where(result => !result.IsActive && arpTable.ContainsKey(result.IpAddress.ToString()))
                .ToArray();
            await Parallel.ForEachAsync(
                arpDevices,
                new ParallelOptions
                {
                    MaxDegreeOfParallelism = Math.Clamp(MaxParallelism / 4, 2, 8),
                    CancellationToken = cancellationToken
                },
                async (result, token) =>
                {
                    string ip = result.IpAddress.ToString();
                    result.IsActive = true;
                    result.ResponseTime = DateTime.Now;
                    result.MacAddress = arpTable[ip];
                    result.HostName = await GetHostNameAsync(ip, token).ConfigureAwait(false);
                    result.IsDhcpServer = await IsLikelyRouterOrDhcp(ip, result.HostName, new HashSet<int>(), token).ConfigureAwait(false);
                });
        }

        private static Dictionary<string, string> ReadArpTable()
        {
            var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            try
            {
                using var process = new Process
                {
                    StartInfo = new ProcessStartInfo
                    {
                        FileName = "arp",
                        Arguments = "-a",
                        UseShellExecute = false,
                        RedirectStandardOutput = true,
                        RedirectStandardError = true,
                        CreateNoWindow = true,
                        StandardOutputEncoding = System.Text.Encoding.Default
                    }
                };
                process.Start();
                string output = process.StandardOutput.ReadToEnd();
                if (!process.WaitForExit(1500))
                {
                    try { process.Kill(); } catch { }
                    return result;
                }

                foreach (var line in output.Split(new[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries))
                {
                    var fields = line.Split(new[] { ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries);
                    if (fields.Length < 2 || !IPAddress.TryParse(fields[0], out var address) || address.AddressFamily != AddressFamily.InterNetwork)
                        continue;
                    if (fields.Length >= 3 && fields[2].Equals("static", StringComparison.OrdinalIgnoreCase))
                        continue;

                    string mac = fields[1].Replace(':', '-').ToUpperInvariant();
                    if (Regex.IsMatch(mac, "^[0-9A-F]{2}(-[0-9A-F]{2}){5}$") && !mac.Equals("FF-FF-FF-FF-FF-FF", StringComparison.OrdinalIgnoreCase))
                        result[address.ToString()] = mac;
                }
            }
            catch { }
            return result;
        }

        private static async Task<PingReply?> SendPingAsync(string ip, int timeoutMs, CancellationToken cancellationToken)
        {
            using var timeoutCts = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
            timeoutCts.CancelAfter(timeoutMs);
            using var ping = new Ping();
            try
            {
                var pingTask = ping.SendPingAsync(IPAddress.Parse(ip), timeoutMs, Array.Empty<byte>(), new PingOptions());
                var completed = await Task.WhenAny(pingTask, Task.Delay(timeoutMs, cancellationToken));
                if (completed != pingTask)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    return null;
                }
                return await pingTask;
            }
            catch (OperationCanceledException) when (!cancellationToken.IsCancellationRequested)
            {
                return null;
            }
        }

        private async Task<bool> IsLikelyRouterOrDhcp(string ip, string hostName, HashSet<int> activePorts, CancellationToken cancellationToken)
        {
            try
            {
                cancellationToken.ThrowIfCancellationRequested();
                string name = hostName.ToLowerInvariant();
                string? gatewayIp = GetGatewayIp();
                bool gateway = string.Equals(ip, gatewayIp, StringComparison.OrdinalIgnoreCase);
                bool explicitRouterName = name.Contains("miwifi") || name.Contains("xiaoqiang") || name.Contains("openwrt") || name.Contains("asus") || name.Contains("tp-link") || name.Contains("tplink") || name.Contains("netgear") || name.Contains("huawei") || name.Contains("h3c") || name.Contains("小米路由") || name.Contains("华硕") || name.Contains("路由器") || name.Contains("网关") || name.Contains("router") || name.Contains("gateway") || name.Contains("路由");
                bool managementPort = activePorts.Contains(80) || activePorts.Contains(443) || activePorts.Contains(8080) || activePorts.Contains(8443);
                if (gateway || explicitRouterName)
                {
                    return true;
                }

                bool result = managementPort && activePorts.Contains(53);
                return result;
            }
            catch (OperationCanceledException)
            {
                throw;
            }
            catch
            {
                return false;
            }
        }

        private async Task<int?> ProbePortAsync(string ip, int port, int timeoutMs, SemaphoreSlim semaphore, CancellationToken cancellationToken)
        {
            await semaphore.WaitAsync(cancellationToken);
            try
            {
                using var timeoutCts = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
                timeoutCts.CancelAfter(timeoutMs);
                using var client = new TcpClient();
                try
                {
                    await client.ConnectAsync(ip, port, timeoutCts.Token).ConfigureAwait(false);
                    return client.Connected ? port : null;
                }
                catch (SocketException ex) when (ex.SocketErrorCode == SocketError.ConnectionRefused)
                {
                    return -port;
                }
                catch (OperationCanceledException) when (!cancellationToken.IsCancellationRequested)
                {
                    return null;
                }
                catch
                {
                    return null;
                }
            }
            finally
            {
                semaphore.Release();
            }
        }

        private async Task<bool> CheckPortAsync(string ip, int port, int timeoutMs, CancellationToken cancellationToken)
        {
            using var timeoutCts = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
            timeoutCts.CancelAfter(timeoutMs);
            try
            {
                using var client = new TcpClient();
                await client.ConnectAsync(ip, port, timeoutCts.Token);
                return client.Connected;
            }
            catch (OperationCanceledException) when (!cancellationToken.IsCancellationRequested)
            {
                return false;
            }
            catch
            {
                return false;
            }
        }

        /// <summary>
        /// 获取MAC地址
        /// </summary>
        private Task<string> GetMacAddressAsync(string ip)
        {
            var lazyTask = _macCache.GetOrAdd(ip, key => new Lazy<Task<string>>(() => QueryMacAddressAsync(key)));
            return lazyTask.Value;
        }

        private async Task<string> QueryMacAddressAsync(string ip)
        {
            try
            {
                var mac = GetMacFromArpCache(ip);
                if (!string.IsNullOrEmpty(mac))
                    return mac;

                using var ping = new Ping();
                await ping.SendPingAsync(ip, 200);
                return GetMacFromArpCache(ip) ?? "未知";
            }
            catch
            {
                return "未知";
            }
        }

        /// <summary>
        /// 从ARP缓存获取MAC地址
        /// </summary>
        private string? GetMacFromArpCache(string ip)
        {
            try
            {
                var p = new System.Diagnostics.Process
                {
                    StartInfo = new System.Diagnostics.ProcessStartInfo
                    {
                        FileName = "arp",
                        Arguments = "-a " + ip,
                        UseShellExecute = false,
                        RedirectStandardOutput = true,
                        CreateNoWindow = true
                    }
                };

                p.Start();
                string output = p.StandardOutput.ReadToEnd();
                p.WaitForExit();

                // 解析ARP输出
                var lines = output.Split('\n');
                foreach (var line in lines)
                {
                    if (line.Contains(ip))
                    {
                        var parts = line.Split(new[] { ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries);
                        if (parts.Length >= 2)
                        {
                            string mac = parts[1].Trim();
                            if (mac.Contains("-") && mac.Length == 17)
                                return mac.ToUpper();
                        }
                    }
                }
            }
            catch { }

            return null;
        }

        /// <summary>
        /// 获取主机名
        /// </summary>
        private async Task<string> GetHostNameAsync(string ip, CancellationToken cancellationToken)
        {
            try
            {
                cancellationToken.ThrowIfCancellationRequested();
                using var timeoutCts = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
                timeoutCts.CancelAfter(800);
                var hostEntry = await Dns.GetHostEntryAsync(ip, timeoutCts.Token);
                return hostEntry.HostName;
            }
            catch (OperationCanceledException) when (!cancellationToken.IsCancellationRequested)
            {
                return "未知";
            }
            catch (OperationCanceledException)
            {
                throw;
            }
            catch
            {
                return "未知";
            }
        }

        /// <summary>
        /// 多网段扫描
        /// </summary>
        public async Task StartMultiSubnetScanAsync(List<string> subnets, int startRange = 1, int endRange = 254)
        {
            if (_isScanning)
                throw new InvalidOperationException("扫描已在进行中");

            _isScanning = true;
            _cancellationTokenSource = new CancellationTokenSource();
            _discoveredServers.Clear();
            _macCache.Clear();

            try
            {
                int totalPerSubnet = endRange - startRange + 1;
                int totalIps = totalPerSubnet * subnets.Count;
                int completed = 0;
                var allResults = new List<DhcpServerInfo>();
                var resultsLock = new object();

                foreach (var subnet in subnets)
                {
                    _cancellationTokenSource.Token.ThrowIfCancellationRequested();

                    await Parallel.ForEachAsync(
                        Enumerable.Range(startRange, totalPerSubnet),
                        new ParallelOptions
                        {
                            MaxDegreeOfParallelism = MaxParallelism,
                            CancellationToken = _cancellationTokenSource.Token
                        },
                        async (i, token) =>
                        {
                            string ip = $"{subnet}.{i}";
                            var result = await ScanSingleIpAsync(ip, token);

                            lock (resultsLock)
                            {
                                allResults.Add(result);
                            }

                            int current = Interlocked.Increment(ref completed);
                            int progress = (int)((double)current / totalIps * 100);
                            ScanProgress?.Invoke(this, progress);
                        });
                }

                // 按网段再按IP最后一段排序
                await ApplyArpResultsAsync(allResults, _cancellationTokenSource.Token);

                allResults.Sort((a, b) =>
                {
                    string aIp = a.IpAddress.ToString();
                    string bIp = b.IpAddress.ToString();
                    var aParts = aIp.Split('.');
                    var bParts = bIp.Split('.');
                    // 先比较前三段
                    for (int i = 0; i < 3; i++)
                    {
                        if (int.TryParse(aParts[i], out int aNum) && int.TryParse(bParts[i], out int bNum))
                        {
                            int cmp = aNum.CompareTo(bNum);
                            if (cmp != 0) return cmp;
                        }
                    }
                    // 再比较最后一段
                    if (int.TryParse(aParts[3], out int aLast) && int.TryParse(bParts[3], out int bLast))
                        return aLast.CompareTo(bLast);
                    return string.Compare(aIp, bIp);
                });

                foreach (var r in allResults)
                {
                    ServerFound?.Invoke(this, r);
                }

                ScanCompleted?.Invoke(this, allResults);
            }
            catch (OperationCanceledException)
            {
                // 用户取消
            }
            catch (Exception ex)
            {
                ScanError?.Invoke(this, ex.Message);
            }
            finally
            {
                _isScanning = false;
            }
        }

        /// <summary>
        /// 判断IP是否为内网（私有）地址
        /// 10.0.0.0/8、172.16.0.0/12、192.168.0.0/16
        /// </summary>
        public static bool IsPrivateIp(string ip)
        {
            var parts = ip.Split('.');
            if (parts.Length != 4) return false;
            if (!int.TryParse(parts[0], out int a) || !int.TryParse(parts[1], out int b)) return false;
            // 10.0.0.0/8
            if (a == 10) return true;
            // 172.16.0.0/12
            if (a == 172 && b >= 16 && b <= 31) return true;
            // 192.168.0.0/16
            if (a == 192 && b == 168) return true;
            return false;
        }

        /// <summary>
        /// 完整IP范围扫描（如 192.168.1.1 至 192.168.3.254）
        /// </summary>
        public async Task StartIpRangeScanAsync(string startIp, string endIp)
        {
            if (_isScanning)
                throw new InvalidOperationException("扫描已在进行中");

            _isScanning = true;
            _cancellationTokenSource = new CancellationTokenSource();
            _discoveredServers.Clear();
            _macCache.Clear();

            long startNum = IpToLong(startIp);
            long endNum = IpToLong(endIp);

            if (startNum > endNum)
                throw new ArgumentException("起始IP不能大于结束IP");

            // 构建有效IP列表（跳过最后一段为0的IP，且只保留内网地址）
            var ipList = new List<long>();
            for (long n = startNum; n <= endNum; n++)
            {
                if ((n & 0xFF) != 0 && IsPrivateIp(LongToIp(n)))
                    ipList.Add(n);
            }

            if (ipList.Count == 0)
            {
                _isScanning = false;
                throw new ArgumentException("扫描范围内不包含内网地址");
            }

            // 统计网段数量（前三段相同为一个网段），超过100个直接抛出
            int subnetCount = ipList.Select(ip => ip >> 8).Distinct().Count();
            if (subnetCount > 100)
            {
                _isScanning = false;
                throw new InvalidOperationException($"TOO_MANY_SUBNETS:{subnetCount}");
            }

            try
            {
                int total = ipList.Count;
                int completed = 0;
                var allResults = new List<DhcpServerInfo>();
                var resultsLock = new object();

                await Parallel.ForEachAsync(
                    ipList,
                    new ParallelOptions
                    {
                        MaxDegreeOfParallelism = MaxParallelism,
                        CancellationToken = _cancellationTokenSource.Token
                    },
                    async (ipNum, token) =>
                    {
                        string ip = LongToIp(ipNum);
                        var result = await ScanSingleIpAsync(ip, token);

                        lock (resultsLock)
                        {
                            allResults.Add(result);
                        }

                        int current = Interlocked.Increment(ref completed);
                        int progress = (int)((double)current / total * 100);
                        ScanProgress?.Invoke(this, progress);
                    });

                // 按IP数值排序
                await ApplyArpResultsAsync(allResults, _cancellationTokenSource.Token);

                allResults.Sort((a, b) =>
                {
                    long aNum = IpToLong(a.IpAddress.ToString());
                    long bNum = IpToLong(b.IpAddress.ToString());
                    return aNum.CompareTo(bNum);
                });

                lock (_lock)
                {
                    _discoveredServers.Clear();
                    _discoveredServers.AddRange(allResults);
                }
                foreach (var r in allResults)
                {
                    ServerFound?.Invoke(this, r);
                }

                ScanCompleted?.Invoke(this, allResults);
            }
            catch (OperationCanceledException)
            {
                // 用户取消
            }
            catch (Exception ex)
            {
                ScanError?.Invoke(this, ex.Message);
            }
            finally
            {
                _isScanning = false;
            }
        }

        /// <summary>
        /// 将IP地址转为long数值
        /// </summary>
        private static long IpToLong(string ip)
        {
            var parts = ip.Split('.');
            if (parts.Length != 4)
                throw new ArgumentException($"IP格式不正确: {ip}");
            return (long.Parse(parts[0]) << 24) + (long.Parse(parts[1]) << 16) +
                   (long.Parse(parts[2]) << 8) + long.Parse(parts[3]);
        }

        /// <summary>
        /// 将long数值转为IP地址字符串
        /// </summary>
        private static string LongToIp(long value)
        {
            return $"{(value >> 24) & 0xFF}.{(value >> 16) & 0xFF}.{(value >> 8) & 0xFF}.{value & 0xFF}";
        }

        /// <summary>
        /// 停止扫描
        /// </summary>
        public void StopScan()
        {
            if (_isScanning)
            {
                _cancellationTokenSource.Cancel();
            }
        }

        /// <summary>
        /// 获取本地网络IP范围
        /// </summary>
        public static string GetLocalNetworkRange()
        {
            try
            {
                var host = Dns.GetHostEntry(Dns.GetHostName());
                foreach (var ip in host.AddressList)
                {
                    if (ip.AddressFamily == AddressFamily.InterNetwork)
                    {
                        var parts = ip.ToString().Split('.');
                        if (parts.Length == 4)
                            return $"{parts[0]}.{parts[1]}.{parts[2]}";
                    }
                }
            }
            catch { }

            return "192.168.1";
        }

        /// <summary>
        /// 获取所有本地网络子网
        /// </summary>
        public static List<string> GetLocalNetworkSubnets()
        {
            var subnets = new List<string>();
            try
            {
                var host = Dns.GetHostEntry(Dns.GetHostName());
                foreach (var ip in host.AddressList)
                {
                    if (ip.AddressFamily == AddressFamily.InterNetwork)
                    {
                        var parts = ip.ToString().Split('.');
                        if (parts.Length == 4)
                        {
                            string subnet = $"{parts[0]}.{parts[1]}.{parts[2]}";
                            if (!subnets.Contains(subnet))
                                subnets.Add(subnet);
                        }
                    }
                }
            }
            catch { }

            if (subnets.Count == 0)
                subnets.Add("192.168.1");

            return subnets;
        }

        /// <summary>
        /// 获取网关IP
        /// </summary>
        public static string? GetGatewayIp()
        {
            try
            {
                var interfaces = NetworkInterface.GetAllNetworkInterfaces();
                foreach (var iface in interfaces)
                {
                    if (iface.OperationalStatus == OperationalStatus.Up)
                    {
                        var props = iface.GetIPProperties();
                        foreach (var gateway in props.GatewayAddresses)
                        {
                            if (gateway.Address.AddressFamily == AddressFamily.InterNetwork)
                            {
                                string gatewayIp = gateway.Address.ToString();
                                return gatewayIp;
                            }
                        }
                    }
                }
            }
            catch { }

            return null;
        }
    }
}
