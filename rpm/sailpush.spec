Name:       sailpush

Summary:    SailPush - Unofficial Pushover Client for SailfishOS
Version:    0.0.0
Release:    1
License:    MIT
URL:        https://github.com/zackslash/sailpush
Source0:    %{name}-%{version}.tar.bz2

Requires:   sailfishsilica-qt5 >= 0.10.9
# Requires:   nemo-qml-plugin-notifications (not in emulator, needed for QML UI)
# Requires:   nemo-qml-plugin-configuration (not in emulator, needed for QML UI)
# Requires:   nemo-qml-plugin-dbus (not in emulator, needed for QML UI)
Requires:   systemd-user-session-targets
Requires:   mce

BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5WebSockets)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Multimedia)
BuildRequires:  desktop-file-utils

%description
SailPush is an unofficial Pushover client for SailfishOS that delivers
instant push notifications via WebSocket connection. Unlike polling-based
clients, SailPush maintains a persistent connection to Pushover servers
for real-time message delivery, similar to the iOS and Android experience.

Features:
- Real-time notifications via WebSocket
- Background daemon with systemd integration
- System notification integration
- Message history and management
- Emergency priority message support

This is an unofficial client. It is not released or supported by Pushover, LLC.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5
%make_build

%install
rm -rf %{buildroot}
%qmake5_install

desktop-file-install --delete-original \
    --dir %{buildroot}%{_datadir}/applications \
    %{buildroot}%{_datadir}/applications/*.desktop

# Enable systemd service via user-session.target
install -d %{buildroot}%{_userunitdir}/user-session.target.wants
ln -sf ../sailpush.service %{buildroot}%{_userunitdir}/user-session.target.wants/

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_userunitdir}/sailpush.service
%{_userunitdir}/user-session.target.wants/sailpush.service
%{_datadir}/lipstick/notificationcategories/x-sailpush.conf
%{_datadir}/dbus-1/services/com.zackslash.sailpush.service

%post
systemctl --user daemon-reload

%preun
if [ "$1" = "0" ]; then
    systemctl --user stop sailpush.service 2>/dev/null || :
fi

%postun
systemctl --user daemon-reload
