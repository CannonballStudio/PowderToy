//
//  uiwrapper.m
//  powdertoy
//
//  Created by Marcin Chojnacki on 17.11.2013.
//  Copyright (c) 2013 Marcin Chojnacki. All rights reserved.
//

#include <UIKit/UIKit.h>

#include "uiwrapper.h"

#define OVERWRITE_DIALOG(pwner) { UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Would you like to overwrite your simulation?" message:nil delegate:[pwner new] cancelButtonTitle:@"No" otherButtonTitles:@"Yes",nil]; [alert show]; [alert release]; }

/// integracja z czescia gry ///
extern void undo();
extern void erase_all();
extern void save_simulation(const char *filename);
extern void load_simulation(const char *filename);

extern int sys_pause;

int pauser;
NSString *saver;
NSString *fukinpath;
/// integracja z czescia gry ///

UIViewController *rootViewController;

NSString *GetDocumentsPath() {
    return [NSHomeDirectory() stringByAppendingString: @"/Documents/"];
}

void ReleaseAndNil() {
    [saver release];
    saver=nil;
}

void ShowSaveUI();

@interface OverwriteController1 : NSObject <UIAlertViewDelegate>
@end
@implementation OverwriteController1
-(void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
    if(buttonIndex==0) ShowSaveUI();
    else if(buttonIndex==1) {
        sys_pause=pauser;
        save_simulation([saver UTF8String]);
    }
}
@end

@interface OverwriteController2 : NSObject <UIAlertViewDelegate>
@end
@implementation OverwriteController2
-(void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
    sys_pause=pauser;
    if(buttonIndex==1) save_simulation([fukinpath UTF8String]);
    [fukinpath release];
}
@end

@interface SaveController : NSObject <UIAlertViewDelegate>
@end
@implementation SaveController
-(void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
    if(buttonIndex==1) {
        NSString *name=[[alertView textFieldAtIndex:0] text];
        if(name.length>0) {
            NSString *path=[[GetDocumentsPath() stringByAppendingString: name] stringByAppendingString:@".pts"];
            if([[NSFileManager defaultManager] fileExistsAtPath:path]) {
                fukinpath=[path copy];
                OVERWRITE_DIALOG(OverwriteController2);
            }
            else {
                sys_pause=pauser;
                save_simulation([path UTF8String]);
                saver=[path copy];
            }
        }
    }
    else sys_pause=pauser;
}
@end

@interface LoadController : NSObject <UIActionSheetDelegate>
@end
@implementation LoadController
-(void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex {
    sys_pause=pauser;
    if(buttonIndex!=-1) {
        NSString *path=[GetDocumentsPath() stringByAppendingString:[[actionSheet buttonTitleAtIndex:buttonIndex] stringByAppendingString:@".pts"]];
        load_simulation([path UTF8String]);
        saver=[path copy];
    }
}
@end

@interface AlertController : NSObject <UIAlertViewDelegate>
@end
@implementation AlertController
-(void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
    sys_pause=pauser;
}
@end

@interface DeleteController : NSObject <UIActionSheetDelegate>
@end
@implementation DeleteController
-(void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex {
    sys_pause=pauser;
    if(buttonIndex!=-1) {
        NSString *path=[GetDocumentsPath() stringByAppendingString:[[actionSheet buttonTitleAtIndex:buttonIndex] stringByAppendingString:@".pts"]];
        remove([path UTF8String]);
        ReleaseAndNil();
    }
}
@end

@interface ActionSheetController : NSObject <UIActionSheetDelegate>
@end
@implementation ActionSheetController
-(void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex {
    if(buttonIndex==0) {
        sys_pause=pauser;
        undo();
    }
    else if(buttonIndex==1) sys_pause=!pauser;
    else if(buttonIndex==2) {
        sys_pause=pauser;
        ReleaseAndNil();
        erase_all();
    }
    else if(buttonIndex==3) {
        if(saver==nil) ShowSaveUI();
        else OVERWRITE_DIALOG(OverwriteController1);
    }
    else if(buttonIndex==4||buttonIndex==5) {
        UIActionSheet *sheet = [UIActionSheet new];
        if(buttonIndex==4) {
            [sheet setTitle:@"Select simulation to load:"];
            [sheet setDelegate:[LoadController new]];
        }
        else {
            [sheet setTitle:@"Select simulation to delete:"];
            [sheet setDelegate:[DeleteController new]];
        }
        
        NSArray *dirContents=[[NSFileManager defaultManager] directoryContentsAtPath:GetDocumentsPath()];
        for(NSString *save in dirContents) {
            if(![[save substringFromIndex:save.length-4] compare:@".pts"]) {
                NSString *simulation=[save substringToIndex:save.length-4];
                [sheet addButtonWithTitle:simulation];
            }
        }
        
        if([sheet buttonTitleAtIndex:0]==NULL) {
            UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"No simulations available!"
                                                      message:nil
                                                      delegate:[AlertController new]
                                                      cancelButtonTitle:@"OK"
                                                      otherButtonTitles: nil];
            
            [alert show];
            [alert release];
        }
        else [sheet showInView:rootViewController.view];
        [sheet release];
    }
    else sys_pause=pauser;

    [self release];
}
@end

void ShowSaveUI() {
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Enter simulation name:"
                                                    message:nil
                                                   delegate:[SaveController new]
                                          cancelButtonTitle:@"Cancel"
                                          otherButtonTitles:@"Save",nil];
    
    alert.alertViewStyle = UIAlertViewStylePlainTextInput;
    [alert show];
    [alert release];
}

// Retrieve SDL's root UIViewController (iOS only!)
// This function is completely NOT guaranteed to work in the future.
// Use it at your own risk!
UIViewController * GetSDLViewController(SDL_Window * sdlWindow)
{
    SDL_SysWMinfo systemWindowInfo;
    SDL_VERSION(&systemWindowInfo.version);
    if ( ! SDL_GetWindowWMInfo(sdlWindow, &systemWindowInfo)) {
        // consider doing some kind of error handling here
        return nil;
    }
    UIWindow * appWindow = systemWindowInfo.info.uikit.window;
    UIViewController * rootViewController = appWindow.rootViewController;
    return rootViewController;
}

void DoActionSheet(SDL_Window * sdlWindow,int pausa)
{
    rootViewController = GetSDLViewController(sdlWindow);
    if ( rootViewController ) {
        pauser=pausa;
        NSString *pausetext;
        if(pauser) pausetext=@"Resume";
        else pausetext=@"Pause";
        
        UIActionSheet *sheet = [[UIActionSheet alloc] initWithTitle:@"Main menu"
                                                      delegate:[ActionSheetController new]
                                                      cancelButtonTitle:@""
                                                      destructiveButtonTitle:nil
                                                      otherButtonTitles:@"Undo", pausetext,  @"Erase all", @"Save simulation", @"Load simulation", @"Delete simulation", nil];
        
        [sheet showInView:rootViewController.view];
        [sheet release];
    }
}