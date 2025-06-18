
Debian
====================
This directory contains files used to package fapaliend/fapalien-qt
for Debian-based Linux systems. If you compile fapaliend/fapalien-qt yourself, there are some useful files here.

## fapalien: URI support ##


fapalien-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install fapalien-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your fapalien-qt binary to `/usr/bin`
and the `../../share/pixmaps/fapalien128.png` to `/usr/share/pixmaps`

fapalien-qt.protocol (KDE)

