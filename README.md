# LanIPScanner for Windows
一个普通的多线程局域网IP扫描工具
## 软件由来
作者闲着没事干用AI做出来的东西<br>
（作者PS：其实网上有很多比我这个做的更好的工具了，我做这个纯属白搭）
## 支持系统
Windows 10~11
## 软件功能
- IP占用扫描（主机发现由 [nmap](https://nmap.org/) 子进程完成）
- DHCP服务器扫描<br>
（后面再加）
## 开源协议
本项目使用 **GNU General Public License v3.0** 开源，完整协议见 [LICENSE](LICENSE)。<br>
主机发现调用随程序分发的官方 nmap，nmap 使用 NPSL 许可（与 GPLv3 不兼容），
其许可文件位于 release 目录下的 `nmap/LICENSE` 与 `nmap/3rd-party-licenses.txt`，版权归 nmap 项目所有。<br>
未随程序分发 Npcap。未安装 Npcap 时 nmap 自动退化为 connect() 模式，功能可用但速度较慢、拿不到 MAC 地址。
## 软件截图（2026-08-04拍摄）
<img width="1202" height="832" alt="图片" src="https://github.com/user-attachments/assets/dc33ee63-5ec7-4235-b51d-8de6ba82e605" />
<img width="1202" height="832" alt="图片" src="https://github.com/user-attachments/assets/4f3cfd90-c9be-4e92-a88e-806945115069" />
<img width="1501" height="866" alt="图片" src="https://github.com/user-attachments/assets/b2dc53d1-6c59-460c-8492-ccc516f8e9c7" />
<img width="1202" height="832" alt="图片" src="https://github.com/user-attachments/assets/ec055fa4-7229-4edb-9c50-c5a58c91d162" />



## 结尾

有建议或bug：[提issues](https://github.com/youye-luna/LanIPScanner/issues)<br>
不喜勿喷<br>
本人是真的不太会编程
