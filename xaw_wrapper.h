#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>  /* Needed here because the use of P.h files */
#include <X11/ConstrainP.h>

#ifdef USE_XAWPLUS
#	include <X11/XawPlus/Box.h>
#	include <X11/XawPlus/Command.h>
#	include <X11/XawPlus/Label.h>
#	include <X11/XawPlus/AsciiText.h>
#	include <X11/XawPlus/Paned.h>
#	include <X11/XawPlus/Viewport.h>
#	include <X11/XawPlus/Cardinals.h>
#	include <X11/XawPlus/SimpleMenu.h>
#	include <X11/XawPlus/SmeBSB.h>
#	include <X11/XawPlus/SmeLine.h>
#	include <X11/XawPlus/MenuButton.h>
#	include <X11/XawPlus/Toggle.h>
#	include <X11/XawPlus/List.h>
#	include <X11/XawPlus/DialogP.h>
#	include <X11/XawPlus/Form.h>
#	include <X11/XawPlus/Text.h>
#	include <X11/XawPlus/Scrollbar.h>
#	include <X11/XawPlus/IconList.h>
   // ... other XawPlus headers
#elif defined(USE_XAW3D)
#	include <X11/Xaw3d/Box.h>
#	include <X11/Xaw3d/Command.h>
#   include <X11/Xaw3d/Label.h>
#	include <X11/Xaw3d/AsciiText.h>
#	include <X11/Xaw3d/Paned.h>
#	include <X11/Xaw3d/Viewport.h>
#	include <X11/Xaw3d/Cardinals.h>
#	include <X11/Xaw3d/SimpleMenu.h>
#	include <X11/Xaw3d/SmeBSB.h>
#	include <X11/Xaw3d/SmeLine.h>
#	include <X11/Xaw3d/MenuButton.h>
#	include <X11/Xaw3d/Toggle.h>
#	include <X11/Xaw3d/List.h>
#	include <X11/Xaw3d/DialogP.h>
#	include <X11/Xaw3d/Form.h>
#	include <X11/Xaw3d/Text.h>
#else
#	include <X11/Xaw/Box.h>
#	include <X11/Xaw/Command.h>
#	include <X11/Xaw/Label.h>
#	include <X11/Xaw/AsciiText.h>
#	include <X11/Xaw/Paned.h>
#	include <X11/Xaw/Viewport.h>
#	include <X11/Xaw/Cardinals.h>
#	include <X11/Xaw/SimpleMenu.h>
#	include <X11/Xaw/SmeBSB.h>
#	include <X11/Xaw/SmeLine.h>
#	include <X11/Xaw/MenuButton.h>
#	include <X11/Xaw/Toggle.h>
#	include <X11/Xaw/List.h>
#	include <X11/Xaw/DialogP.h>
#	include <X11/Xaw/Form.h>
#	include <X11/Xaw/Text.h>
#endif
