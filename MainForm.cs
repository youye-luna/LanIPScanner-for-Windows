using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace DhcpScanner
{
    public partial class MainForm : Form
    {
        private readonly DhcpScanner _scanner;
        private bool _isScanning;
        private string _currentStartIp = "";
        private string _currentEndIp = "";

        public MainForm()
        {
            // 加载设置（语言、扫描线程数）
            var settings = AppSettings.Load();
            Lang.Current = settings.Language;

            InitializeComponent();
            _scanner = new DhcpScanner { MaxParallelism = settings.ScanThreads };
            _isScanning = false;

            // 绑定事件
            _scanner.ServerFound += Scanner_ServerFound;
            _scanner.ScanProgress += Scanner_ScanProgress;
            _scanner.ScanCompleted += Scanner_ScanCompleted;
            _scanner.ScanError += Scanner_ScanError;

            // 应用界面语言
            ApplyLanguage();

            // 启动时按保存配置清理过期历史记录
            ScanHistoryStore.Prune();

            // 初始布局按钮位置
            PanelSearch_Resize(this, EventArgs.Empty);
        }

        /// <summary>
        /// 根据当前语言刷新主窗口文本
        /// </summary>
        private void ApplyLanguage()
        {
            Text = Lang.Get("FormTitle");
            labelTitle.Text = Lang.Get("ScanRangeTitle");
            labelStartIp.Text = Lang.Get("StartIp");
            labelTo.Text = Lang.Get("To");
            labelEndIp.Text = Lang.Get("EndIp");
            buttonScan.Text = Lang.Get("StartScan");
            buttonStop.Text = Lang.Get("StopScan");
            buttonClear.Text = Lang.Get("ClearResults");
            buttonExport.Text = Lang.Get("ExportResults");
            buttonSettings.Text = Lang.Get("Settings");
            buttonHistory.Text = Lang.Get("History");
            toolStripStatusLabel.Text = Lang.Get("Ready");
            toolStripStatusCount.Text = Lang.Get("StatusCountInit");

            // 刷新所有已打开的结果面板（表格列头 + IP分布图标题/图例）
            foreach (TabPage tab in tabControlResults.TabPages)
            {
                if (tab.Tag is string subnet)
                    tab.Text = string.Format(Lang.Get("SubnetTab"), subnet);
                if (tab.Controls[0] is SubnetResultPanel panel)
                    panel.RefreshLanguage();
            }
        }

        /// <summary>
        /// 设置按钮点击事件
        /// </summary>
        private void ButtonSettings_Click(object sender, EventArgs e)
        {
            using var settingsForm = new SettingsForm();
            if (settingsForm.ShowDialog(this) == DialogResult.OK)
            {
                // 重新加载已保存的设置并应用
                var settings = AppSettings.Load();
                Lang.Current = settings.Language;
                _scanner.MaxParallelism = settings.ScanThreads;
                ApplyLanguage();
            }
        }

        /// <summary>
        /// 历史按钮点击事件 —— 打开扫描历史窗口
        /// </summary>
        private void ButtonHistory_Click(object sender, EventArgs e)
        {
            using var historyForm = new HistoryForm();
            if (historyForm.ShowDialog(this) == DialogResult.OK && historyForm.SelectedRecord != null)
            {
                DisplayHistoryRecord(historyForm.SelectedRecord);
            }
        }

        /// <summary>
        /// 将历史记录重新加载到结果页
        /// </summary>
        private void DisplayHistoryRecord(ScanHistoryRecord record)
        {
            var results = record.Devices.Select(d => d.ToServerInfo()).ToList();
            PopulateResultTabs(results);
            toolStripStatusLabel.Text = string.Format(Lang.Get("HistoryLoaded"), record.StartIp, record.EndIp);
        }

        /// <summary>
        /// 获取默认起始IP（本机IP，最后一段为1）
        /// </summary>
        private static string GetDefaultStartIp()
        {
            try
            {
                var host = System.Net.Dns.GetHostEntry(System.Net.Dns.GetHostName());
                foreach (var ip in host.AddressList)
                {
                    if (ip.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
                    {
                        var parts = ip.ToString().Split('.');
                        if (parts.Length == 4)
                            return $"{parts[0]}.{parts[1]}.{parts[2]}.1";
                    }
                }
            }
            catch { }
            return "192.168.1.1";
        }

        /// <summary>
        /// 获取默认结束IP（本机IP，最后一段为255）
        /// </summary>
        private static string GetDefaultEndIp()
        {
            try
            {
                var host = System.Net.Dns.GetHostEntry(System.Net.Dns.GetHostName());
                foreach (var ip in host.AddressList)
                {
                    if (ip.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
                    {
                        var parts = ip.ToString().Split('.');
                        if (parts.Length == 4)
                            return $"{parts[0]}.{parts[1]}.{parts[2]}.255";
                    }
                }
            }
            catch { }
            return "192.168.1.255";
        }

        /// <summary>
        /// 开始扫描按钮点击事件
        /// </summary>
        private async void ButtonScan_Click(object sender, EventArgs e)
        {
            if (_isScanning)
            {
                MessageBox.Show(Lang.Get("ScanningInProgress"), Lang.Get("Tip"),
                    MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            string startIp = ipStart.GetAddressText();
            string endIp = ipEnd.GetAddressText();

            if (string.IsNullOrEmpty(startIp) || string.IsNullOrEmpty(endIp))
            {
                MessageBox.Show(Lang.Get("InputIpRequired"), Lang.Get("Error"),
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            // 限制只能扫描内网地址
            if (!DhcpScanner.IsPrivateIp(startIp) || !DhcpScanner.IsPrivateIp(endIp))
            {
                MessageBox.Show(Lang.Get("PrivateIpOnly"), Lang.Get("Error"),
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            // 清除之前的标签页
            tabControlResults.TabPages.Clear();

            // 记录本次扫描范围（用于自动保存历史）
            _currentStartIp = startIp;
            _currentEndIp = endIp;

            // 更新UI状态
            _isScanning = true;
            buttonScan.Enabled = false;
            buttonStop.Enabled = true;
            buttonClear.Enabled = false;
            buttonExport.Enabled = false;
            buttonSettings.Enabled = false;
            buttonHistory.Enabled = false;
            progressBarScan.Value = 0;
            toolStripStatusLabel.Text = string.Format(Lang.Get("ScanningRange"), startIp, endIp);
            toolStripStatusCount.Text = Lang.Get("StatusCountScanning");

            try
            {
                await _scanner.StartIpRangeScanAsync(startIp, endIp);
            }
            catch (Exception ex)
            {
                if (ex.Message.StartsWith("TOO_MANY_SUBNETS:"))
                {
                    int count = int.Parse(ex.Message.Split(':')[1]);
                    ShowTooManySubnetsMessage(count);
                }
                else
                {
                    MessageBox.Show(string.Format(Lang.Get("ScanErrorStatus"), ex.Message), Lang.Get("Error"),
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            finally
            {
                _isScanning = false;
                buttonScan.Enabled = true;
                buttonStop.Enabled = false;
                buttonClear.Enabled = true;
                buttonExport.Enabled = true;
                buttonSettings.Enabled = true;
                buttonHistory.Enabled = true;
            }
        }

        /// <summary>
        /// 停止扫描按钮点击事件
        /// </summary>
        private void ButtonStop_Click(object sender, EventArgs e)
        {
            if (_isScanning)
            {
                _scanner.StopScan();
                toolStripStatusLabel.Text = Lang.Get("ScanStopped");
            }
        }

        /// <summary>
        /// 清空结果按钮点击事件
        /// </summary>
        private void ButtonClear_Click(object sender, EventArgs e)
        {
            tabControlResults.TabPages.Clear();
            progressBarScan.Value = 0;
            toolStripStatusCount.Text = Lang.Get("StatusCountInit");
            toolStripStatusLabel.Text = Lang.Get("Ready");
        }

        /// <summary>
        /// 导出结果按钮点击事件
        /// </summary>
        private void ButtonExport_Click(object sender, EventArgs e)
        {
            if (tabControlResults.TabPages.Count == 0)
            {
                MessageBox.Show(Lang.Get("NoDataToExport"), Lang.Get("Tip"),
                    MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            using var saveFileDialog = new SaveFileDialog
            {
                Filter = Lang.Get("ExportFilter"),
                FileName = $"{Lang.Get("ExportFileName")}_{DateTime.Now:yyyyMMdd_HHmmss}",
                DefaultExt = "csv"
            };

            if (saveFileDialog.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    ExportToCsv(saveFileDialog.FileName);
                    MessageBox.Show(string.Format(Lang.Get("ExportSuccess"), saveFileDialog.FileName), Lang.Get("Success"),
                        MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                catch (Exception ex)
                {
                    MessageBox.Show(string.Format(Lang.Get("ExportFailed"), ex.Message), Lang.Get("Error"),
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        /// <summary>
        /// 导出数据到CSV文件（所有标签页）
        /// </summary>
        private void ExportToCsv(string filePath)
        {
            using var writer = new StreamWriter(filePath, false, System.Text.Encoding.UTF8);

            writer.WriteLine(Lang.Get("CsvHeader"));

            foreach (TabPage tab in tabControlResults.TabPages)
            {
                if (tab.Controls[0] is SubnetResultPanel panel)
                {
                    string subnet = tab.Tag as string ?? tab.Text;
                    foreach (var row in panel.GetRows())
                    {
                        string ip = row.Cells["IpAddress"].Value?.ToString() ?? "";
                        string mac = row.Cells["MacAddress"].Value?.ToString() ?? "";
                        string host = row.Cells["HostName"].Value?.ToString() ?? "";
                        string ping = row.Cells["PingMs"].Value?.ToString() ?? "";
                        string router = row.Cells["IsRouter"].Value?.ToString() ?? "";
                        string status = row.Cells["Status"].Value?.ToString() ?? "";

                        writer.WriteLine($"{EscapeCsvField(subnet)},{EscapeCsvField(ip)},{EscapeCsvField(mac)},{EscapeCsvField(host)},{EscapeCsvField(ping)},{EscapeCsvField(router)},{EscapeCsvField(status)}");
                    }
                }
            }
        }

        private string EscapeCsvField(string field)
        {
            if (field.Contains(",") || field.Contains("\"") || field.Contains("\n"))
                return $"\"{field.Replace("\"", "\"\"")}\"";
            return field;
        }

        /// <summary>
        /// 发现设备事件处理
        /// </summary>
        private void Scanner_ServerFound(object? sender, DhcpServerInfo serverInfo)
        {
            if (InvokeRequired)
            {
                Invoke(new Action(() => Scanner_ServerFound(sender, serverInfo)));
                return;
            }
        }

        /// <summary>
        /// 扫描进度更新事件处理
        /// </summary>
        private void Scanner_ScanProgress(object? sender, int progress)
        {
            if (InvokeRequired)
            {
                Invoke(new Action(() => Scanner_ScanProgress(sender, progress)));
                return;
            }

            progressBarScan.Value = Math.Min(progress, 100);
            toolStripStatusLabel.Text = string.Format(Lang.Get("ScanProgressPercent"), progress);
        }

        /// <summary>
        /// 扫描完成事件处理 —— 自动保存历史并按网段分组显示
        /// </summary>
        private void Scanner_ScanCompleted(object? sender, List<DhcpServerInfo> results)
        {
            if (InvokeRequired)
            {
                Invoke(new Action(() => Scanner_ScanCompleted(sender, results)));
                return;
            }

            progressBarScan.Value = 100;
            toolStripStatusLabel.Text = Lang.Get("OrganizingResults");

            // 自动保存扫描历史
            SaveScanHistory(results);

            PopulateResultTabs(results);

            int totalOnline = results.Count(x => x.IsActive);
            int totalRouter = results.Count(x => x.IsDhcpServer);
            int totalNoDevice = results.Count - totalOnline;

            string message = string.Format(Lang.Get("ScanSummary"), results.Count, totalOnline, totalNoDevice, totalRouter);
            MessageBox.Show(message, Lang.Get("Completed"), MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        /// <summary>
        /// 将扫描结果按网段分组填充到结果标签页
        /// </summary>
        private void PopulateResultTabs(List<DhcpServerInfo> results)
        {
            // 按网段分组
            var groups = results
                .GroupBy(r => string.Join(".", r.IpAddress.ToString().Split('.').Take(3)))
                .OrderBy(g => g.Key)
                .ToList();

            tabControlResults.TabPages.Clear();

            foreach (var group in groups)
            {
                var subnetResults = group.OrderBy(r =>
                {
                    var parts = r.IpAddress.ToString().Split('.');
                    return long.Parse(parts[0]) << 24 | long.Parse(parts[1]) << 16 | long.Parse(parts[2]) << 8 | long.Parse(parts[3]);
                }).ToList();

                var panel = new SubnetResultPanel
                {
                    Dock = DockStyle.Fill
                };
                panel.PopulateData(subnetResults);

                var tab = new TabPage(string.Format(Lang.Get("SubnetTab"), group.Key))
                {
                    Tag = group.Key
                };
                tab.Controls.Add(panel);
                tabControlResults.TabPages.Add(tab);
            }

            // 统计总数
            int totalOnline = results.Count(x => x.IsActive);
            int totalRouter = results.Count(x => x.IsDhcpServer);
            int totalNoDevice = results.Count - totalOnline;

            toolStripStatusCount.Text = string.Format(Lang.Get("StatusCountDone"), totalOnline, totalNoDevice, totalRouter);
            toolStripStatusLabel.Text = Lang.Get("ScanCompletedStatus");
        }

        /// <summary>
        /// 将本次扫描结果自动保存到历史记录
        /// </summary>
        private void SaveScanHistory(List<DhcpServerInfo> results)
        {
            var record = new ScanHistoryRecord
            {
                ScanTime = DateTime.Now,
                StartIp = _currentStartIp,
                EndIp = _currentEndIp,
                Devices = results.Select(ScanHistoryDevice.From).ToList()
            };
            ScanHistoryStore.Save(record);
        }

        /// <summary>
        /// 扫描错误事件处理
        /// </summary>
        private void Scanner_ScanError(object? sender, string errorMessage)
        {
            if (InvokeRequired)
            {
                Invoke(new Action(() => Scanner_ScanError(sender, errorMessage)));
                return;
            }

            toolStripStatusLabel.Text = string.Format(Lang.Get("ScanErrorStatus"), errorMessage);
            MessageBox.Show(string.Format(Lang.Get("ScanErrorDialog"), errorMessage), Lang.Get("Error"),
                MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        /// <summary>
        /// 窗体关闭事件
        /// </summary>
        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            if (_isScanning)
            {
                var result = MessageBox.Show(Lang.Get("ExitWhileScanning"), Lang.Get("Confirm"),
                    MessageBoxButtons.YesNo, MessageBoxIcon.Question);

                if (result == DialogResult.No)
                {
                    e.Cancel = true;
                    return;
                }

                _scanner.StopScan();
            }

            base.OnFormClosing(e);
        }

        /// <summary>
        /// 搜索面板大小变化时，重新布局按钮位置
        /// </summary>
        private void PanelSearch_Resize(object? sender, EventArgs e)
        {
            if (flowButtons == null || panelSearch == null) return;

            // 按钮行放在 IP 输入框下方
            int buttonsY = ipEnd.Location.Y + ipEnd.Height + 8;

            // 历史/设置按钮靠右（历史在设置左边）
            int paddingRight = panelSearch.Padding.Right;
            buttonHistory.Location = new System.Drawing.Point(
                panelSearch.Width - paddingRight - buttonSettings.Width - buttonHistory.Width - 8,
                buttonsY);
            buttonSettings.Location = new System.Drawing.Point(
                panelSearch.Width - paddingRight - buttonSettings.Width,
                buttonsY);

            // 按钮面板宽度留出右侧历史/设置按钮的空间
            int settingsBtnWidth = buttonSettings.Width;
            int historyBtnWidth = buttonHistory.Width;
            int flowWidth = panelSearch.Width - 20 - paddingRight - settingsBtnWidth - historyBtnWidth - 10 - 8;
            flowButtons.Location = new System.Drawing.Point(20, buttonsY);
            flowButtons.Size = new System.Drawing.Size(flowWidth, 36);
        }

        /// <summary>
        /// 显示网段过多提示（带灰色副标题）
        /// </summary>
        private static void ShowTooManySubnetsMessage(int count)
        {
            using var form = new Form
            {
                Text = Lang.Get("Tip"),
                Size = new System.Drawing.Size(360, 180),
                FormBorderStyle = FormBorderStyle.FixedDialog,
                StartPosition = FormStartPosition.CenterParent,
                MaximizeBox = false,
                MinimizeBox = false,
                BackColor = System.Drawing.Color.White,
            };

            var labelMain = new Label
            {
                Text = Lang.Get("MaxSubnets"),
                Font = new System.Drawing.Font("Microsoft YaHei", 12F, System.Drawing.FontStyle.Bold),
                ForeColor = System.Drawing.Color.FromArgb(50, 50, 50),
                AutoSize = false,
                Location = new System.Drawing.Point(20, 25),
                Size = new System.Drawing.Size(310, 35),
                TextAlign = System.Drawing.ContentAlignment.MiddleCenter,
            };

            var labelSub = new Label
            {
                Text = string.Format(Lang.Get("TooManySubnetsSub"), count),
                Font = new System.Drawing.Font("Microsoft YaHei", 9F),
                ForeColor = System.Drawing.Color.FromArgb(160, 160, 160),
                AutoSize = false,
                Location = new System.Drawing.Point(20, 60),
                Size = new System.Drawing.Size(310, 45),
                TextAlign = System.Drawing.ContentAlignment.MiddleCenter,
            };

            var btnOk = new Button
            {
                Text = Lang.Get("Ok"),
                DialogResult = DialogResult.OK,
                FlatStyle = FlatStyle.System,
                Size = new System.Drawing.Size(80, 30),
                Location = new System.Drawing.Point(135, 105),
                Font = new System.Drawing.Font("Microsoft YaHei", 9F),
                Cursor = System.Windows.Forms.Cursors.Hand,
            };

            form.Controls.AddRange(new Control[] { labelMain, labelSub, btnOk });
            form.AcceptButton = btnOk;

            form.ShowDialog();
        }
    }
}
