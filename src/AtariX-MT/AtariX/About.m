#import "AppDelegate.h"
#include "EmulationMain.h"
#include "Debug.h"

/* these were renamed in SDK 10.12 and above */
#if !defined(MAC_OS_X_VERSION_10_12) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_12
#define NSWindowStyleMaskClosable NSClosableWindowMask
#define NSWindowStyleMaskTitled NSTitledWindowMask
#define NSTextAlignmentLeft NSLeftTextAlignment
#endif

#define autorelease self

#define PROGRAM    "AtariX"
#define COMPILE_DATE __DATE__
#define URL        "https://github.com/th-otto/AtariX/"
#define S_COPYRIGHT_SIGN "\xC2\xA9"
#define COPYRIGHT  "Copyright " S_COPYRIGHT_SIGN " 2014 Andreas Kromke"
#define AUTHOR     "Andreas Kromke, Thorsten Otto"
#define EMAIL      "admin@tho-otto.de"
#define HOMEPAGE   "https://www.tho-otto.de/"

#define _(x) x

@interface AboutPanel : NSWindow <NSWindowDelegate>
{
@public
@private
}
- (void)close;
@end

@implementation AboutPanel
- (void)close
{
	[super close];
}

- (void)dealloc
{
	[super dealloc];
}

/* ------------------------------------------------------------------------- */

static void compiler_version(char *strbuf)
{
#define bitvers (sizeof(int) < 4 ? "16-bit" : sizeof(void *) >= 16 ? "128-bit" : sizeof(void *) >= 8 ? "64-bit" : "32-bit")
#define stringify1(x) #x
#define stringify(x) stringify1(x)

#if defined(_MSC_VER)
#  if _MSC_VER > 1929
	sprintf(strbuf, "MS Visual Studio, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1929
	sprintf(strbuf, "Visual Studio 2019 16.9, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1928
	sprintf(strbuf, "Visual Studio 2019 16.8, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1927
	sprintf(strbuf, "Visual Studio 2019 16.7, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1926
	sprintf(strbuf, "Visual Studio 2019 16.6, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1925
	sprintf(strbuf, "Visual Studio 2019 16.5, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1924
	sprintf(strbuf, "Visual Studio 2019 16.4, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1923
	sprintf(strbuf, "Visual Studio 2019 16.3, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1922
	sprintf(strbuf, "Visual Studio 2019 16.2, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1921
	sprintf(strbuf, "Visual Studio 2019 16.1, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1920
	sprintf(strbuf, "Visual Studio 2019 16.0, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1916
	sprintf(strbuf, "Visual Studio 2017 15.9, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1915
	sprintf(strbuf, "Visual Studio 2017 15.8, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1914
	sprintf(strbuf, "Visual Studio 2017 15.7, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1913
	sprintf(strbuf, "Visual Studio 2017 15.6, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1912
	sprintf(strbuf, "Visual Studio 2017 15.5, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1911
	sprintf(strbuf, "Visual Studio 2017 15.3, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1910
	sprintf(strbuf, "Visual Studio 2017 15.0, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1900
	sprintf(strbuf, "Visual Studio 2015 14.0, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1800
	sprintf(strbuf, "Visual Studio 2013 12.0, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1700
	sprintf(strbuf, "Visual Studio 2012 11.0, Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1600
	sprintf(strbuf, "Visual Studio 2010 10.0 Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1500
	sprintf(strbuf, "Visual Studio 2008 9.0 Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1400
	sprintf(strbuf, "Visual Studio 2005 8.0 Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1310
	sprintf(strbuf, "Visual Studio 2003 7.1 Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1300
	sprintf(strbuf, "Visual Studio 2002 7.0 Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1200
	sprintf(strbuf, "Visual Studio 6.0 Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1100
	sprintf(strbuf, "Visual Studio 5.0 Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1020
	sprintf(strbuf, "Developer Studio 4.2 Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 1000
	sprintf(strbuf, "Developer Studio 4.0 Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 900
	sprintf(strbuf, "Developer Studio 2.0 Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  elif _MSC_VER >= 800
	sprintf(strbuf, "Developer Studio 1.0 Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  else
	sprintf(strbuf, "Ancient VC++ Compiler-Version %d.%d (%s)", _MSC_VER / 100, _MSC_VER % 100, bitvers);
#  endif
#elif defined(__INTEL_COMPILER)
	sprintf(strbuf, "ICC version %d.%d.%d (%s)", __INTEL_COMPILER / 100, (__INTEL_COMPILER / 10) %10), __INTEL_COMPILER % 10, bitvers);
#elif defined(__clang_version__)
	sprintf(strbuf, "clang version %s (%s)", __clang_version__, bitvers);
#elif defined(__clang__)
	sprintf(strbuf, "clang version %s.%s.%s (%s)", stringify(__clang_major__), stringify(__clang_minor__), stringify(__clang_patchlevel__), bitvers);
#elif defined(__GNUC__)
	sprintf(strbuf, "GNU-C version %s.%s.%s (%s)", stringify(__GNUC__), stringify(__GNUC_MINOR__), stringify(__GNUC_PATCHLEVEL__), bitvers);
#elif defined(__AHCC__)
	sprintf(strbuf, "AHCC version %d.%02x (%s)", __AHCC__ >> 8, __AHCC__ & 0xff, bitvers);
#elif defined(__PUREC__)
	sprintf(strbuf, "Pure-C version %d.%02x (%s)", __PUREC__ >> 8, __PUREC__ & 0xff, bitvers);
#elif defined(SOZOBON)
	sprintf(strbuf, "SOZOBON-C V2.00x10 (%s)", bitvers);
#else
	sprintf(strbuf, "Unknown Compiler (%s)", bitvers);
#endif

#undef bitvers
#undef stringify1
#undef stringify
}

static NSTextField *urllabel(const char *format, const char *display, const char *url)
{
	NSURL *nsurl = [NSURL URLWithString: [[[NSString alloc] initWithUTF8String: url] autorelease]];
	NSTextField *label;
	NSString *wformat = [[[NSString alloc] initWithUTF8String: format] autorelease];
	NSRange range;
	
	range = [wformat rangeOfString:@"%s"];
	NSString *str = [[[NSString alloc] initWithUTF8String: display] autorelease];
	wformat = [wformat stringByReplacingCharactersInRange:range withString:str];
	NSMutableAttributedString *attr = [[[NSMutableAttributedString alloc] initWithString:wformat] autorelease];
	range.length = [str length];
	[attr addAttribute:NSLinkAttributeName value:nsurl range:range];
	[attr addAttribute:NSForegroundColorAttributeName value:[NSColor blueColor] range:range];
	[attr addAttribute:NSUnderlineStyleAttributeName value:[NSNumber numberWithInt: NSUnderlineStyleSingle] range:range];
	label = [NSTextField labelWithAttributedString:attr];
	[label setAllowsEditingTextAttributes:YES];
	[label setSelectable: YES];
	/*
	 * prevent changing the font of the link to system default when activated
	 */
	range = NSMakeRange(0, [attr length]);
	[attr addAttribute:NSFontAttributeName value:[label font] range:range];
	[label setAttributedStringValue: attr];
	return label;
}

- (id)init
{
	NSUInteger windowStyle = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
	NSBackingStoreType bufferingType = NSBackingStoreBuffered;
	NSStackView *vbox, *hbox, *vbox2;
	NSImageView *image;
	NSTextField *label;
	NSButton *button;
	char strbuf[1024];

	if ((self = [super initWithContentRect:NSMakeRect(0, 0, 400, 300) styleMask: windowStyle backing: bufferingType defer: NO]) == nil)
	{
		return nil;
	}
	[self cascadeTopLeftFromPoint:NSMakePoint(20, 20)];
	[self setTitle: NSLocalizedString(@"AtariX Versionsinfo", nil)];

	vbox = [[[NSStackView alloc] init] autorelease];
	[vbox setSpacing: 10];
	[vbox setOrientation: NSUserInterfaceLayoutOrientationVertical];
	[vbox setAlignment: NSLayoutAttributeLeading];
	[vbox setEdgeInsets: NSEdgeInsetsMake(10, 10, 10, 10)];
	[self setContentView: vbox];
	hbox = [[[NSStackView alloc] init] autorelease];
	[hbox setSpacing: 10];
	[hbox setAlignment: NSLayoutAttributeCenterX];
	[vbox addView:hbox inGravity: NSStackViewGravityLeading];
	image = [NSImageView imageViewWithImage: NSApp.applicationIconImage];
	[image setAlignment: NSLayoutAttributeCenterX];
	[hbox addView:image inGravity: NSStackViewGravityLeading];

	hbox = [[[NSStackView alloc] init] autorelease];
	[hbox setSpacing: 10];
	[hbox setEdgeInsets: NSEdgeInsetsMake(10, 10, 10, 10)];
	[hbox setAlignment: NSLayoutAttributeCenterX];
	[vbox addView:hbox inGravity: NSStackViewGravityLeading];
	
	vbox2 = [[[NSStackView alloc] init] autorelease];
	[vbox2 setOrientation: NSUserInterfaceLayoutOrientationVertical];
	[vbox2 setAlignment: NSLayoutAttributeLeft];
	[vbox2 setEdgeInsets: NSEdgeInsetsMake(10, 10, 10, 10)];
	[hbox addView:vbox2 inGravity: NSStackViewGravityLeading];

	sprintf(strbuf, _("%s"), CFStringGetCStringPtr(CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), CFSTR("CFBundleShortVersionString")), kCFStringEncodingUTF8));
	label = [NSTextField labelWithString: [[[NSString alloc] initWithUTF8String: strbuf] autorelease]];
	[label setAlignment: NSTextAlignmentLeft];
	[vbox2 addView:label inGravity: NSStackViewGravityLeading];
	sprintf(strbuf, _("(compiled %s)"), COMPILE_DATE);
	label = [NSTextField labelWithString: [[[NSString alloc] initWithUTF8String: strbuf] autorelease]];
	[label setAlignment: NSTextAlignmentLeft];
	[vbox2 addView:label inGravity: NSStackViewGravityLeading];
	compiler_version(strbuf);
	label = [NSTextField labelWithString: [[[NSString alloc] initWithUTF8String: strbuf] autorelease]];
	[label setAlignment: NSTextAlignmentLeft];
	[vbox2 addView:label inGravity: NSStackViewGravityLeading];

	hbox = [[[NSStackView alloc] init] autorelease];
	[hbox setSpacing: 10];
	[hbox setEdgeInsets: NSEdgeInsetsMake(10, 10, 10, 10)];
	[vbox addView:hbox inGravity: NSStackViewGravityLeading];
	
	vbox2 = [[[NSStackView alloc] init] autorelease];
	[vbox2 setOrientation: NSUserInterfaceLayoutOrientationVertical];
	[vbox2 setAlignment: NSLayoutAttributeLeft];
	[hbox addView:vbox2 inGravity: NSStackViewGravityLeading];

	label = [NSTextField labelWithString: [[[NSString alloc] initWithUTF8String: COPYRIGHT] autorelease]];
	[label setAlignment: NSTextAlignmentLeft];
	[vbox2 addView:label inGravity: NSStackViewGravityLeading];

	sprintf(strbuf, _("%s is Open Source (see %%s for further information)."), PROGRAM);
	label = urllabel(strbuf, URL, URL);
	[label setAlignment: NSTextAlignmentLeft];
	[vbox2 addView:label inGravity: NSStackViewGravityLeading];

	sprintf(strbuf, _("Author: %s"), AUTHOR);
	label = [NSTextField labelWithString: [[[NSString alloc] initWithUTF8String: strbuf] autorelease]];
	[label setAlignment: NSTextAlignmentLeft];
	[vbox2 addView:label inGravity: NSStackViewGravityLeading];

	label = urllabel(_("Email: %s"), EMAIL, "mailto:" EMAIL);
	[label setAlignment: NSTextAlignmentLeft];
	[vbox2 addView:label inGravity: NSStackViewGravityLeading];
	
	label = urllabel(_("Homepage: %s"), HOMEPAGE, HOMEPAGE);
	[label setAlignment: NSTextAlignmentLeft];
	[vbox2 addView:label inGravity: NSStackViewGravityLeading];
	
	hbox = [[[NSStackView alloc] init] autorelease];
	[hbox setSpacing: 10];
	[hbox setEdgeInsets: NSEdgeInsetsMake(10, 10, 10, 10)];
	[vbox addView:hbox inGravity: NSStackViewGravityLeading];
	
	button = [NSButton buttonWithTitle:NSLocalizedString(@"Close", nil) target:self action:@selector(performClose:)];
	[hbox setAlignment: NSLayoutAttributeCenterY];
	[hbox addView:button inGravity: NSStackViewGravityTrailing];

	[self makeFirstResponder:button];
	[button setHighlighted:YES];
	[button setKeyEquivalent:@"\015"];

	return self;
}

@end

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

void About(id sender)
{
	AboutPanel *panel;
	
	panel = [[[AboutPanel alloc] init] autorelease];
	[panel makeKeyAndOrderFront: sender];
}
