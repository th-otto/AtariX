#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>
#import <Cocoa/Cocoa.h>
#include "PreferencesWindowController.h"

@interface AppDelegate : NSObject
{
    PreferencesWindowController *preferencesController;
	BOOL m_configEmulationDone;
}
- (IBAction)actionPreferences:(id)sender;
- (IBAction)actionQuit:(id)sender;
- (IBAction)actionRun:(id)sender;
- (IBAction)actionChooseRootfs:(id)sender;
- (IBAction)actionRevertRootfs:(id)sender;
- (IBAction)actionCut:(id)sender;
- (IBAction)actionCopy:(id)sender;
- (IBAction)actionPaste:(id)sender;

@property (nonatomic, retain) NSWindowController *preferencesController;

- (void) showPreferences:sender;
- (void) changeRootfsUrl:(NSString *)newUrl;
- (NSString *) getAtariKernelUrl:(NSInteger)atariLanguage;
- (BOOL) checkRootfs;
- (void) configEmulation;
@end
