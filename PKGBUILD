# Maintainer: Your Name <your.email@example.com>
pkgname=elfexec
pkgver=1.0.0
pkgrel=1
pkgdesc="An in-memory ELF execution wrapper for polyglot scripts"
arch=('x86_64' 'aarch64' 'i686' 'armv7h')
url="https://github.com/spagreeks/elfexec"
license=('MIT')
depends=('glibc')

source=("$pkgname-$pkgver.tar.gz::$url/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cd "$pkgname-$pkgver"
    
    # We use Arch Linux's standard $CFLAGS and $LDFLAGS here.
    # -Os optimizes for size, and -s strips debug symbols to keep the binary tiny!
    gcc $CFLAGS -Os -s $LDFLAGS elfexec.c -o elfexec
}

package() {
    cd "$pkgname-$pkgver"
    
    # This installs the compiled binary into /usr/bin/
    install -Dm755 elfexec "$pkgdir/usr/bin/elfexec"
}
