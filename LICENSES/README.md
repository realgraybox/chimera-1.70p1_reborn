# Licences

The licence texts of everything in this repository, and of the libraries a
static build links in. Ship this whole folder with any binary built from
this repository: several of the licences require their notice to come with
every copy, including binaries.

## Programs and what they contain

| Program | Contains code under |
|---|---|
| `chimera` | `Chimera.txt`, `NCSA-Mosaic.txt`, `Newcastle-ScrollText.txt`, `MIT-X-Consortium.txt`, `xloadimage.txt`, `public-domain.txt`, `zlib-nanosvg.txt`, `MIT-graybox.txt`, plus the system libraries below |
| `chimera-menu` | `menu.txt` (GPL v2 or later, see `GPL-2.0.txt`) |
| `text2pdf` | `text2pdf.txt` |
| `xvl` | `zlib-graybox.txt`, `public-domain.txt` (stb), `libtiff.txt`, `libwebp.txt`, `libwebp-PATENTS.txt`, plus the system libraries below |
| `fits2gif` | `zlib-graybox.txt`, `system-giflib.txt` |
| `chimera-curl` (optional) | `curl.txt`, `Apache-2.0.txt` (mbedTLS 2.16.0) |
| Scripts (`https-helper.sh`, `chimera-open`, ...) | `zlib-graybox.txt` |

## Source files by licence

| Licence file | Files |
|---|---|
| `Chimera.txt` | John Kilburg's licence: `src/`, most of `mxw/`, `common/`, `lib/`, `util/` unless stated otherwise in the file |
| `NCSA-Mosaic.txt` | University of Illinois: `libhtmlw/HTML.c`, `HTML.h`, `HTMLP.h`, `HTMLamp.h`, `HTMLformat.c`, `HTMLimages.c`, `HTMLlists.c`, `HTMLparse.c`, `HTMLwidgets.c`; the icons `NoImage.xbm`, `DelayedImage.xbm`, `AnchoredImage.xbm` carry no notice but come from NCSA Mosaic |
| `public-domain.txt` | `src/md5.c`, the stb headers, `libhtmlw/HTML-PSformat.c` |
| `Newcastle-ScrollText.txt` | `mxw/ScrollText.*` |
| `MIT-X-Consortium.txt` | `mxw/Porthole.c`, the Xaw widgets in `XawPlus-2.0.2/` |
| `xloadimage.txt` | `xloadimage/` |
| `zlib-nanosvg.txt` | `src/nanosvg.h`, `src/nanosvgrast.h` |
| `MIT-graybox.txt` | `XawPlus-2.0.2/XmuStrToPixmap.c` |
| `zlib-graybox.txt` | `helpers/xvl/xvl.c`, `helpers/fits2gif/`, `helpers/curl/https-helper.sh`, `util/chimera-open.script` and other files marked with it |
| `menu.txt`, `GPL-2.0.txt` | `helpers/menu/` |
| `text2pdf.txt` | `helpers/text2pdf/` (kept unmodified, as its licence requires) |
| `libtiff.txt` | `helpers/xvl/tiff-3.7.0/` |
| `libwebp.txt`, `libwebp-PATENTS.txt` | `helpers/xvl/libwebp-0.4.4/` |
| `curl.txt`, `Apache-2.0.txt` | curl 7.47.0 and mbedTLS 2.16.0, downloaded by `make -C helpers/curl static-curl` |

Files with no licence notice of their own:

- `libhtmlw/DrawingArea.c`, `DrawingArea.h`, `DrawingAreaP.h`: written by
  Gustaf Neumann in 1993, a Chimera contributor.
- `XawPlus-2.0.2/Add3dExt.*`, `DrawingArea.*`: written by Roland Krause
  for XawPlus.
- `XawPlus-2.0.2/TextTr.c`: the X Consortium's Xaw text translation table.
- Changes made for Chimera 1.70 in 1997 by its contributors, marked "WBE"
  (Winston Edmond, who put 1.70 together) and "GN" (Gustaf Neumann) in
  the source.

They are distributed here as part of Chimera, as they were originally.

## System libraries

A dynamically linked build uses the system's own copies of these, and
nothing of them is included in the binary. A static build (for example
with the uClibc toolchain) includes them, so their notices must be
shipped too:

| File | Library |
|---|---|
| `system-libX11.txt`, `system-libXt.txt`, `system-libXmu.txt`, `system-libXpm.txt`, `system-libXext.txt`, `system-libSM.txt`, `system-libICE.txt` | X.Org libraries (MIT / X11 style) |
| `system-zlib.txt` | zlib, used by `xvl` |
| `system-giflib.txt` | giflib, used by `fits2gif` |
| `LGPL-2.1.txt` | uClibc, the C library of the 32-bit static build |

The `system-*.txt` files are the copyright files of the Debian/Ubuntu
packages; the licences are the same for other builds of these libraries.

uClibc is licensed under the GNU LGPL 2.1. A program linked statically
with it must be distributed so that users can relink it with a modified
uClibc. For Chimera this is met by the complete source code of every
program, with its makefiles, being available in this repository.
