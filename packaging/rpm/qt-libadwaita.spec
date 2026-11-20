# Build from a git snapshot:  rpmbuild -ba qt-libadwaita.spec  (sources: the GitHub tarball of the tag or commit)
%global commit0 master

Name:           qt-libadwaita
Version:        0.1.0
Release:        1%{?dist}
Summary:        Qt widget style that looks like GNOME libadwaita
License:        GPL-2.0-or-later
URL:            https://github.com/SeniorMatt/qt-libadwaita
Source0:        %{url}/archive/%{commit0}/%{name}-%{commit0}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  cmake >= 3.16
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt5-qtbase-devel
Recommends:     adwaita-sans-fonts

%description
A QStyle plugin (style key "Libadwaita") for Qt 6 and Qt 5.15 that draws widgets like GNOME libadwaita,
and KDE colour schemes that match it. Select it in System Settings > Application Style, or run an
application with QT_STYLE_OVERRIDE=Libadwaita.

%prep
%autosetup -n %{name}-%{commit0}

%build
for q in 6 5; do
  case $q in 6) pd=%{_qt6_plugindir} ;; 5) pd=%{_qt5_plugindir} ;; esac
  cmake -S . -B build-qt$q -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=%{_prefix} \
        -DQT_MAJOR=$q -DQT_PLUGIN_INSTALL_DIR=$pd/styles
  cmake --build build-qt$q %{?_smp_mflags}
done

%install
DESTDIR=%{buildroot} cmake --install build-qt6
DESTDIR=%{buildroot} cmake --install build-qt5
install -Dpm644 data/kde-colors/*.colors -t %{buildroot}%{_datadir}/color-schemes

%files
%license COPYING
%doc README.md docs/
%{_qt6_plugindir}/styles/qtlibadwaitastyle.so
%{_qt5_plugindir}/styles/qtlibadwaitastyle.so
%{_bindir}/adwaita-gallery
%{_bindir}/adwaita-gallery-qt5
%{_datadir}/color-schemes/Libadwaita*.colors

%changelog
* Sat Nov 21 2026 Senior Matthew <130131083+SeniorMatt@users.noreply.github.com> - 0.1.0-1
- Initial package
