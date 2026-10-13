# chimera-1.70p1_reborn

A revival of **Chimera 1.70p1**, the small X11 web browser written by John Kilburg at UNLV in the 1990s. Chimera stopped being maintained around 2000. This repository takes the 1.70p1 source and makes it build with current and very old toolchains, and makes it usable on the web as it is in 2026, within the limits of what a browser without JavaScript or CSS can do.

Chimera uses only Xlib, Xt and an Athena-style widget set. Statically linked against uClibc, the browser binary is about 780 kB.

## What works

- **Protocols handled by Chimera itself:** HTTP, FTP, Gopher and local files.
- **HTTPS:** through an external helper script that calls a bundled, statically linked `curl` (see [HTTPS](#https)).
- **Other protocols:** `telnet:` and `mailto:` are handed to external programs through the `mailcap` and `protocol` files.
- **HTML:** the NCSA Mosaic HTML widget, covering headings, lists, text styles, forms, tables and inline images.
- **Inline images decoded in the browser:** JPEG, PNG, BMP, GIF, TGA and PSD (stb_image), SVG (NanoSVG), and XBM, XPM and PBM/PGM/PPM (xloadimage). True-colour images are reduced to a 256-colour palette with xloadimage's median-cut reducer; images with 256 colours or fewer keep their exact colours.
- **Inline images through converters:** TIFF, WebP and SGI (converted to PNG by `xvl -c`), FITS, PostScript and PDF, using the rules in the `convert` file.
- **External viewers:** anything else is passed to the program named in `mailcap`. `xvl`, a small image viewer included here, handles most image formats, including TIFF and WebP.
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
- **HTTPS:** added through the `protocol` mechanism and the scripts in `helpers/curl`.
- **Widget set:** XawPlus 2.0.2 is bundled and used by default. Plain Xaw or Xaw3d can be selected in `mk.config`.
- **Resizing:** window resize events are debounced so the page is not reformatted on every intermediate size (compile flag `REBOUNCH`).
- **Parser:** faster tag matching in the HTML parser, found by profiling (compile flag `GPROFOPT`).
- **Locale:** the browser calls `setlocale()` at startup so the system character set is used.
- **Helpers:** new small programs `xvl` (image viewer, and converter for inline images with `-c`) and `fits2gif`, plus bundled `text2pdf` and `menu`.

## Building

### Requirements

- GNU make and a C compiler that accepts `-std=gnu99`.
- X11 development headers and libraries: `libX11`, `libXt`, `libXmu`, `libXext`, `libXpm`, `libSM`, `libICE`.
- `giflib` for `fits2gif` and `menu`.
- `zlib` for `xvl`.

On Debian or Ubuntu:

```sh
sudo apt install build-essential libx11-dev libxt-dev libxmu-dev libxext-dev \
                 libxpm-dev libsm-dev libice-dev libgif-dev zlib1g-dev
```

XawPlus, curl, mbedTLS, libtiff and libwebp are built from the sources in this repository.

### Build

```sh
make
```

This builds, in order, the support libraries, XawPlus, the browser (`src/chimera`) and the helpers. Building curl and mbedTLS takes most of the time.

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

### Install

```sh
make install
```

Programs go to `$(PREFIX)/bin` (default `/usr/bin`). The data files go to `$HOME/chimera`.

**The data directory is fixed at build time.** `mk.config` expands `$HOME` while compiling, so the paths to `home.html`, `help.html`, `mime.types`, `mailcap`, `convert`, `protocol` and `cacheinfo` are compiled in for the user who ran `make`. Build as the user who will run the browser. If you install with `sudo`, check that the data files ended up in that user's `~/chimera` and not in root's home directory.

## Running

```sh
chimera [URL]
```

Without a URL, Chimera opens the address in the `WWW_HOME` environment variable if it is set, and otherwise `~/chimera/home.html`. The Help button opens `~/chimera/help.html`, the original Chimera manual, which describes the buttons, keyboard shortcuts, environment variables and X resources.

### Files in `~/chimera`

| File | Purpose |
|---|---|
| `home.html`, `help.html` | Start page and manual |
| `mime.types` | File extension to content type |
| `mailcap` | Content type to external viewer |
| `convert` | Conversion rules, including converters for inline images |
| `protocol` | Protocol to helper program, used for `https` and `mailto` |
| `cacheinfo` | Cache directories and limits |
| `cookies/cookies.txt` | Cookie jar written by the HTTPS helper |

## HTTPS

Chimera has no TLS code. The `protocol` file maps `https` to `/usr/bin/https-helper_cookie.sh`. Chimera sends the request to that script, which fetches the page with the bundled curl and returns it as a plain HTTP/1.0 response.

curl 7.47.0 and mbedTLS 2.16.0 are included as source on purpose, so the helper can be built statically with the same toolchain as the browser. curl is configured for HTTP and HTTPS only, without zlib and without IPv6.

Two scripts are provided:

- `https-helper_cookie.sh` keeps cookies between requests and forwards the real HTTP status. This is the default.
- `https-helper_nocookie.sh` is a simpler variant without cookies.

**Read this before using HTTPS for anything that matters:**

- **Certificates are not verified.** The scripts call curl with `-k`. The connection is encrypted, but nothing checks that the server is the one it claims to be.
- **Requests are logged in clear text.** Each request overwrites `/tmp/chimera.log` with its headers and, for forms, the submitted body. That includes any password typed into a form.
- **The TLS library is old.** mbedTLS 2.16.0 and curl 7.47.0 no longer receive security fixes.
- **Some URLs are blocked.** The cookie script answers any URL containing `flattr`, `badge`, `counter` or `analytics` with a 1x1 transparent GIF, and does the same for images that fail to load.
- **Timeouts are short.** 3 seconds to connect, 10 seconds in total.

Treat HTTPS here as a way to read public pages, not as a secure channel.

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
| `util/` | Original helper scripts (telnet, 3270, audio, mailto, bookmark conversion) |
| `helpers/curl/` | curl 7.47.0, mbedTLS 2.16.0 and the HTTPS helper scripts |
| `helpers/xvl/` | `xvl` image viewer, with libtiff 3.7.0 and libwebp 0.4.4 |
| `helpers/fits2gif/` | Converter for inline FITS images |
| `helpers/text2pdf/` | text2pdf 1.1, used for printing |
| `helpers/menu/` | Small Xlib pop-up menu |

## Known rough edges

- `make install` does not install `menu`, `text2pdf` or `xvl`. The scripts in `util/` are not installed either.
- `lib/protocol` points `mailto` at `/usr/X11/bin/mailto.pl`. Adjust it to what is installed.
- Some comments and script messages are in Danish.

## Licensing

This repository combines code under several licences. **Chimera itself is not open-source software in the usual sense:** its licence allows free redistribution for academic, research and internal business use only, and forbids charging for copies. The HTML widget from NCSA Mosaic has similar terms.

| Component | Licence, as stated in its files |
|---|---|
| Chimera (`src/`, most of `mxw/`) | John Kilburg's licence: non-commercial use, no fee. See `src/copyright.h` |
| `libhtmlw/` | NCSA Mosaic licence: academic, research and internal business use, no fee |
| `mxw/ScrollText.*` | University of Newcastle upon Tyne: any purpose except commercial exploitation |
| `xloadimage/`, `mxw/Porthole.c`, XawPlus | MIT / X Consortium style permissive licences |
| `src/md5.c` | Public domain |
| stb_image, stb_image_resize2, stb_image_write | Public domain |
| NanoSVG | zlib licence |
| curl 7.47.0 | curl licence (MIT style) |
| mbedTLS 2.16.0 | Apache 2.0 |
| libtiff 3.7.0 | libtiff licence (BSD style) |
| libwebp 0.4.4 | BSD 3-clause |
| text2pdf 1.1 | Free redistribution; modification only with the author's permission |
| `menu` | GPL v2 or later |
| `xvl`, `fits2gif`, HTTPS helper scripts | zlib licence |
| `XawPlus-2.0.2/XmuStrToPixmap.c` | MIT |

Check the individual files before redistributing a build.

## Credits

Chimera was written by John Kilburg, with contributions from many others. Version 1.70 was put together by Winston Edmond. The HTML widget comes from NCSA Mosaic, the image loaders from Jim Frost's xloadimage. Thanks also to the authors of XawPlus, stb, NanoSVG, curl, mbedTLS, libtiff, libwebp, text2pdf and menu.
