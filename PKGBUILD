# Maintainer: Idan Koblik <https://github.com/IdanKoblik>

pkgname=baguette
pkgver=r61.gf1d673f
pkgrel=1
pkgdesc='A minimal Wayland HUD / status bar written in C17'
arch=('x86_64')
url='https://github.com/IdanKoblik/baguette'
license=('GPL-3.0-or-later')
depends=('wayland' 'cairo' 'libconfig')
makedepends=('git' 'pkgconf')
install="$pkgname.install"

pkgver() {
	cd "$startdir"
	# Derive from git: <tag>.r<commits>.g<hash>, or r<commits>.g<hash> before
	# the first tag. Falls back to the static pkgver outside a git checkout.
	if git rev-parse HEAD >/dev/null 2>&1; then
		if git describe --long --tags >/dev/null 2>&1; then
			git describe --long --tags | sed 's/\([^-]*-g\)/r\1/;s/-/./g'
		else
			printf 'r%s.g%s' "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
		fi
	else
		printf '%s' "$pkgver"
	fi
}

build() {
	cd "$startdir"
	make
}

check() {
	cd "$startdir"
	make test
}

package() {
	cd "$startdir"
	make PREFIX=/usr DESTDIR="$pkgdir" install
	# Ship the example style.cfg as a reference config; the .install scriptlet
	# tells the user to copy it to ~/.config/baguette/style.cfg.
	install -Dm644 assets/style.cfg "$pkgdir/usr/share/$pkgname/style.cfg"
}
