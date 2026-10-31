# chimera-1.70p1_reborn

A revival of **Chimera 1.70p1**, the small X11 web browser written by John Kilburg at UNLV in the 1990s. Chimera stopped being maintained around 2000. This repository takes the 1.70p1 source and makes it build with current and very old toolchains, and makes it usable on the web as it is in 2026, within the limits of what a browser without JavaScript or CSS can do.

Chimera uses only Xlib, Xt and an Athena-style widget set. Statically linked against uClibc, the browser binary is about 780 kB.

## What works

- **Protocols handled by Chimera itself:** HTTP, FTP, Gopher and local files.
- **HTTPS:** through an external helper script that calls `curl` (see [HTTPS](#https)).
- **Other protocols:** `telnet:` and `mailto:` are handed to external programs through the `mailcap` and `protocol` files.
- **HTML:** the NCSA Mosaic HTML widget, covering headings, lists, text styles, forms, tables and inline images.
- **Inline images decoded in the browser:** JPEG, PNG, BMP, GIF, TGA and PSD (stb_image, recognised from the image data whatever type name the server sends), SVG (NanoSVG), and XBM, XPM and PBM/PGM/PPM (xloadimage). True-colour images are reduced to a 256-colour palette with xloadimage's median-cut reducer; images with 256 colours or fewer keep their exact colours.
- **Inline images through converters:** TIFF, WebP and SGI (converted to PNG by `xvl -c`), FITS, PostScript and PDF, using the rules in the `convert` file.
- **External viewers:** anything else is passed to the program named in `mailcap`, after a dialog that shows the file's type and address (resource `confirmExternal`). The default `mailcap` hands images, audio, video, PDF and other documents to `chimera-open`, which opens a copy of the file with the desktop's default program (`xdg-open`), or with `xvl`, the small image viewer included here, when there is none. `chimera-open` refuses programs, scripts and `.desktop` files.
- **Right-click menu:** a right-click on a link runs `sendto`, which pops up the menu defined in `~/chimera/sendto` with the link available as `"$F"` (a local path for `file:` links). Edit that file to change the entries; label and command are separated by a single tab.
- **Printing:** the print dialog pipes the page to `chimera_print.sh`, which turns plain text into a PDF with the bundled `text2pdf`.

## What does not work

- No JavaScript, no CSS, no frames, no HTML5 elements.
- No native TLS. HTTPS depends on the helper script and its limitations.
- HTTP requests are HTTP/1.0.
- Many modern sites will be unreadable or unusable. Simple, text-oriented sites work best.

## Changes from the original 1.70p1

- **Build system:** plain GNU makefiles with one shared configuration file, `mk.config`. No Imake or `xmkmf` step.
- **Toolchains:** builds as a 64-bit dynamically linked binary with gcc 5.5 and glibc, and as a 32-bit statically linked binary with gcc 3.4.3 and uClibc. Compiler warnings have been reduced on both.
- **Images:** a new image path in `src/inline.c` using stb_image and NanoSVG, with colour reduction by `xloadimage/reduce.c` (compile flag `IMAGENEW`).
- **HTTPS:** added through the `protocol` mechanism and the script in `helpers/curl`.
- **Widget set:** XawPlus 2.0.2 is bundled and used by default. Plain Xaw or Xaw3d can be selected in `mk.config`.
- **Resizing:** window resize events are debounced so the page is not reformatted on every intermediate size (compile flag `REBOUNCH`).
- **Parser:** faster tag matching in the HTML parser, found by profiling.
- **Character sets:** UTF-8 pages are converted to Latin-1 for the X core fonts, and form data is sent back in the page's character set.
- **Images in the background:** pages show at once, and their images are fetched several at a time while the window stays usable.
- **Forms:** checkboxes and radio buttons show a mark, select lists are sized to their entries, password fields show `*`, `maxlength` is honoured, and Enter submits.
- **Themes:** a dark (default) and a light colour theme as X resource files.
- **Helpers:** new small programs `xvl` (image viewer, and converter for inline images with `-c`) and `fits2gif`, plus bundled `text2pdf` and Terry Loveall's `menu`, installed as `chimera-menu`.

## Building

### Requirements

- GNU make and a C compiler that accepts `-std=gnu99`.
- X11 development headers and libraries: `libX11`, `libXt`, `libXmu`, `libXext`, `libXpm`, `libSM`, `libICE`.
- `giflib` for `fits2gif`.
- `zlib` for `xvl`.
- At run time, `curl` for HTTPS, and a terminal program for telnet links and "Terminal Here" (see `chimera-term` below).
- Only for `make -C helpers/curl static-curl`: `sha256sum`, `bzip2` and network access (and `git` for `LEGACY=1`).

On Debian or Ubuntu:

```sh
sudo apt install build-essential libx11-dev libxt-dev libxmu-dev libxext-dev \
                 libxpm-dev libsm-dev libice-dev libgif-dev zlib1g-dev
```

XawPlus, libtiff and libwebp are built from the sources in this repository. The default XawPlus build does not need `libxaw7-dev`: the `X11/Xaw/...` includes in libhtmlw, mxw and src are mapped to the XawPlus headers. Only the Xaw and Xaw3d builds (see `mk.config`) need the system's Xaw headers.

The source tree may live in a directory whose path contains spaces, such as `/media/user/Linux ext2/src`. `DESTDIR` and the install directories must not contain spaces.

### Build

```sh
make
```

This builds, in order, the support libraries, XawPlus, the browser (`src/chimera`) and the helpers.

The top-level makefile keeps going when one directory fails, so check that `src/chimera` exists afterwards. To build only the browser and what it needs:

```sh
for d in lib libhtmlw mxw util xloadimage XawPlus-2.0.2 src; do make -C $d; done
```

### Static build with uClibc

Point `CC` at the uClibc compiler:

```sh
make CC=i386-linux-uclibc-gcc
```

`mk.config` asks the compiler which C library it uses. For uClibc it links statically and adds the flags and math wrappers it needs, so nothing has to be edited when switching between the 64-bit and 32-bit toolchains. For a 32-bit target, `mk.config` has a commented-out flag line with `-march=i386`.

### Configuration

| File | What it sets |
|---|---|
| `mk.config` | Compiler, flags, install prefix, widget set (`XAW_LIB_TYPE`: `XAWPLUS`, `XAW3D` or plain Xaw); detects the target and uClibc |
| `options.h` | User-Agent string, print command, mail command, network timeouts. `options.h.dist` is the original |
| `src/fallback.c` | Built-in X resources (colours, labels, fonts) |
| `lib/Chimera` | X resources installed as `~/Chimera`: the dark theme (default), with colours, fonts, sizes, labels, scroll wheel |
| `lib/Chimera-light` | The same with the light theme, installed as `~/Chimera-light` |
| `src/Chimera.ad` | The original, annotated list of all resources and their defaults, for reference; not installed |

### Install

```sh
make install
```

Programs go to `$(PREFIX)/bin` (default `/usr/bin`). The data files go to `$HOME/chimera`, and the X resources to `$HOME/Chimera` (dark theme) and `$HOME/Chimera-light` (light theme), where Xt finds them without further setup. `make install` overwrites the data files, so keep a copy of any you have changed. A resource file that differs from the new one is kept as `~/Chimera.old` or `~/Chimera-light.old`.

**The data directory is fixed at build time.** `mk.config` expands `$HOME` while compiling, so the paths to `home.html`, `help.html`, `mime.types`, `mailcap`, `convert`, `protocol` and `cacheinfo` are compiled in for the user who ran `make`. Build as the user who will run the browser. If you install with `sudo`, check that the data files ended up in that user's `~/chimera` and not in root's home directory.

## Running

```sh
chimera [URL]
```

Without a URL, Chimera opens the address in the `WWW_HOME` environment variable if it is set, and otherwise `~/chimera/home.html`. The Help button opens `~/chimera/help.html`, the original Chimera manual, which describes the buttons, keyboard shortcuts, environment variables and X resources.

### Themes

The dark theme in `~/Chimera` is the default. For the light theme:

```sh
chimera -xrm '*customization: -light'     # one session: X reads ~/Chimera-light instead
cp ~/Chimera-light ~/Chimera               # make it the default
```

Both files are plain X resources and can be edited; the colours are near the top. `#rrggbb` colours also work in static builds whose Xlib has no Xcms (see `NO_XCMS` in `mk.config`).

### Files in `~/chimera`

| File | Purpose |
|---|---|
| `home.html`, `help.html` | Start page and manual |
| `mime.types` | File extension to content type |
| `mailcap` | Content type to external viewer |
| `convert` | Conversion rules, including converters for inline images |
| `protocol` | Protocol to helper program, used for `https` and `mailto` |
| `cacheinfo` | Cache directories and limits |
| `sendto` | Entries of the right-click menu for links |
| `cookies/cookies.txt` | Cookie jar written by the HTTPS helper |
| `cacert.pem` | Optional root certificates for the HTTPS helper |
| `cache/` | The page cache, when it is turned on (`cacheOff: False`); closed to other users |
| `helper.log` | Request log of the HTTPS helper, only with `DEBUG=true` |

## HTTPS

Chimera has no TLS code. The `protocol` file maps `https` to `https-helper.sh`. Chimera sends the request to that script, which fetches the page with curl and returns it as a plain HTTP/1.0 response.

The script uses the system's `curl`, which checks server certificates against the system's certificate store. Its settings are at the top of the script and can also be set as environment variables:

| Setting | Default | Meaning |
|---|---|---|
| `CURL` | `chimera-curl` if installed, else `curl` | curl program to run |
| `COOKIES` | `true` | Keep cookies in `~/chimera/cookies/cookies.txt` |
| `CA_BUNDLE` | `~/chimera/cacert.pem` if it exists | File of trusted root certificates, passed to curl with `--cacert` |
| `INSECURE` | `false` | `true` skips the certificate check (`curl -k`); last resort when no usable CA file is available |
| `DEBUG` | `false` | `true` logs every request, including form data such as passwords, to `~/chimera/helper.log` |

Redirects are followed only to `https` addresses. When an https page redirects to plain `http`, the script shows a short page with the link instead, so nothing is fetched unencrypted under an https address without you seeing it.

Cookies are sent and stored for pages. An image gets the cookies only when it comes from the same site as the page it is on (Chimera tells the script which page that is), and an image never stores cookies: there are no third-party cookies.

### Static curl

For a fully static system, such as one built with the 32-bit uClibc toolchain, `helpers/curl` can build a small static curl (curl 8.23.0 with Mbed TLS 3.6.7, HTTP and HTTPS only, TLS 1.3):

```sh
make -C helpers/curl static-curl
make -C helpers/curl install-static-curl
```

The sources are not kept in this repository. `static-curl` downloads the curl and Mbed TLS release tarballs and checks their SHA-256, so it needs network access, `sha256sum` and `bzip2` once. It installs the binary as `chimera-curl`, which the script then uses automatically.

`make -C helpers/curl static-curl LEGACY=1` builds the versions used before, curl 7.47.0 with mbed TLS 2.16.0, for a compiler that cannot build the current ones. They no longer get security fixes and have no TLS 1.3, so use them only if you must.

chimera-curl checks certificates against the CA file found when it was built. On an older system that file lacks today's root certificates, and pages fail with `Cert verify failed: BADCERT_NOT_TRUSTED`. Fetch a current file with

```sh
make -C helpers/curl cacert
```

which downloads curl's [cacert.pem](https://curl.se/ca/cacert.pem) to `~/chimera/cacert.pem`, where the script picks it up. Run it on a machine with a working system curl and copy the file over if needed, and refresh it now and then. `make -C helpers/curl distclean` removes the downloaded sources.

**Read this before using HTTPS for anything that matters:**

- **Some URLs are blocked.** The script answers any URL containing `flattr`, `badge`, `counter` or `analytics` with a 1x1 transparent GIF, and does the same for images that fail to load.
- **Timeouts are short.** 3 seconds to connect, 10 seconds in total.
- **Keep curl up to date.** The script is only as safe as the curl it runs: the system curl gets updates with the system, a `chimera-curl` only when you rebuild it. The `LEGACY=1` build is out of support.

## Security

Chimera is a 1990s browser and was not written with today's web in mind. Use it to read public pages, not for banking, shopping or anything else where a mistake would cost you.

**What helps.** Chimera runs no JavaScript, no CSS, no plug-ins and no web fonts, so the largest attack surface of a modern browser is simply not there: no script can run code, read what you type, fingerprint the machine or open pop-ups, and tracking that depends on scripts does not work. What a page can make Chimera do is fetch a document and its images, and show them.

**What has been done here.**

- The HTML parser, the HTTP and character-set handling, URL parsing and the XBM/PBM image loaders were fuzzed with AFL++ and AddressSanitizer, and the overflows found were fixed. The rest of the C code was reviewed for unbounded string handling (`sprintf`, `strcpy`, `strcat`), and the overflows found there (Gopher menus, printing, status lines) were fixed too.
- HTTPS redirects are followed only to https, and there are no third-party cookies (see [HTTPS](#https)).
- A downloaded file is handed to an external program only after you confirm it, and `chimera-open` refuses programs, scripts and `.desktop` files.
- Temporary files are made with `mkstemp`, and the cache lives in a directory of its own, closed to other users.
- FTP data connections go to the server's own address, not to one the server names, which could be another machine on your network.
- Helper programs no longer inherit text waiting in Chimera's output buffers.

**What remains.**

- The C code is old and large; fuzzing and review find many problems but not all. Chimera has no sandbox: a flaw in it runs with your rights.
- HTTPS is only as good as curl and its certificate store. Plain HTTP, FTP and Gopher are unencrypted, and Chimera shows no padlock or other sign of which you are on: look at the address.
- The image decoders in stb_image and the converters for TIFF, WebP and others (libtiff 3.7.0 and libwebp 0.4.4 in `xvl`) read data from the web. libtiff and libwebp in particular are old versions.
- Programs started from `mailcap`, `convert` and `protocol` see data from the web; keep those files to programs you trust.
- Do not run Chimera as root.

## Repository layout

| Path | Contents |
|---|---|
| `src/` | The browser: URL handling, HTTP, FTP, Gopher, cache, MIME, inline images, main window |
| `libhtmlw/` | HTML widget from NCSA Mosaic, as modified for Chimera |
| `mxw/` | Chimera's dialog widgets: bookmarks, authentication, string and output prompts |
| `xloadimage/` | Image loaders, dithering and colour reduction from xloadimage |
| `common/` | `common.h`, shared by all parts: memory helper macros mapped onto Xt's allocator |
| `XawPlus-2.0.2/` | Bundled XawPlus widget set |
| `lib/` | Runtime data files installed to `~/chimera`, and `chimera_print.sh` |
| `util/` | Helper scripts: `chimera-open`, `chimera-term`, `chimera-mailto`, and the original telnet, 3270 and bookmark conversion scripts |
| `helpers/curl/` | The HTTPS helper script, and a makefile that can build a static curl |
| `helpers/xvl/` | `xvl` image viewer, with libtiff 3.7.0 and libwebp 0.4.4 |
| `helpers/fits2gif/` | Converter for inline FITS images |
| `helpers/text2pdf/` | text2pdf 1.1, used for printing |
| `helpers/menu/` | Small Xlib pop-up menu (`chimera-menu`) and the `sendto` script for the right-click menu |

## Known rough edges

- `make install` writes the helper paths in `~/chimera/protocol` for the chosen `PREFIX`, but the commands in `mailcap` and `convert` are found through `PATH`.
- Some of the original scripts in `util/` expect Perl or programs that are rare today.
- A `mailcap` command ends at the next `;`; each `%s` becomes the file name. Chimera deletes the downloaded file as soon as the command returns. `chimera-open` exists for that reason.
- `chimera-mailto` hands `mailto:` links to the desktop's mail program with `xdg-open`, or to the program in `MAIL_CLIENT` (for example `MAIL_CLIENT='chimera-term mutt'`). Chimera stays on the page.
- `chimera-term` opens a terminal window for telnet links and the sendto menu's "Terminal Here", so xterm is not required. It uses `$TERMINAL` if set, otherwise the first of `x-terminal-emulator`, `defaultterminal`, `xterm`, `urxvt`, `rxvt`, `lxterminal`, `xfce4-terminal`, `mate-terminal`, `konsole` and `gnome-terminal` it finds.

## Licensing

This repository combines code under several licences. **Chimera itself is not open-source software in the usual sense:** its licence allows free redistribution for academic, research and internal business use only, and forbids charging for copies. The HTML widget from NCSA Mosaic has similar terms.

| Component | Licence, as stated in its files |
|---|---|
| Chimera (`src/`, most of `mxw/`) | John Kilburg's licence: non-commercial use, no fee. See `src/copyright.h` |
| `libhtmlw/` | NCSA Mosaic licence: academic, research and internal business use, no fee. As the licence requires, the files are marked as a modified version, not the original NCSA Mosaic |
| `libhtmlw/HTML-PSformat.c` | Public domain (US Government work) |
| `libhtmlw/DrawingArea.*` | Gustaf Neumann, 1993; no licence notice |
| `mxw/ScrollText.*` | University of Newcastle upon Tyne: any purpose except commercial exploitation |
| `xloadimage/`, `mxw/Porthole.c`, XawPlus | MIT / X Consortium style permissive licences |
| `src/md5.c` | Public domain |
| stb_image, stb_image_resize2, stb_image_write | Public domain |
| NanoSVG | zlib licence |
| curl 8.23.0 (7.47.0 with `LEGACY=1`; only if `static-curl` is built) | curl licence (MIT style) |
| Mbed TLS 3.6.7 (2.16.0 with `LEGACY=1`; only if `static-curl` is built) | Apache 2.0 (3.6 is also available under GPL 2.0 or later) |
| libtiff 3.7.0 | libtiff licence (BSD style) |
| libwebp 0.4.4 | BSD 3-clause |
| text2pdf 1.1 | Free redistribution; modification only with the author's permission |
| `menu` (`chimera-menu`) | GPL v2 or later |
| `xvl`, `fits2gif`, HTTPS helper scripts | zlib licence |
| `XawPlus-2.0.2/XmuStrToPixmap.c` | MIT |

Both Chimera's and NCSA's licences allow binaries and source to be distributed free of charge, provided the copyright notices come with every copy. The full licence texts, with a list of which program contains what, are in [`LICENSES/`](LICENSES/README.md): ship that folder with any binary. It also covers the X libraries, zlib, giflib and uClibc (LGPL 2.1), which a static build links in.

## Credits

Chimera was written by John Kilburg, with contributions from many others. Version 1.70 was put together by Winston Edmond. The HTML widget comes from NCSA Mosaic, the image loaders from Jim Frost's xloadimage. Thanks also to the authors of XawPlus, stb, NanoSVG, curl, mbedTLS, libtiff, libwebp, text2pdf and menu.
