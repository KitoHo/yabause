/*  Copyright 2006 Guillaume Duhamel

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <unistd.h>
#include <Carbon/Carbon.h>
#include "settings.h"

#define		TAB_ID 	 	128
#define		TAB_SIGNATURE	'tabs'
int tabList[] = {129, 130, 131, 132};

ControlRef oldTab;

void SelectItemOfTabControl(ControlRef tabControl)
{
    ControlRef userPane;
    ControlID controlID;

    GetControlID(tabControl, &controlID);
    if (controlID.id != TAB_ID) return;

    controlID.signature = TAB_SIGNATURE;

    controlID.id = tabList[GetControlValue(tabControl) - 1];
    GetControlByID(GetControlOwner(tabControl), &controlID, &userPane);
       
    DisableControl(oldTab);
    SetControlVisibility(oldTab, false, false);
    EnableControl(userPane);
    SetControlVisibility(userPane, true, true);
    oldTab = userPane;

    Draw1Control(tabControl);
}

pascal OSStatus TabEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
    ControlRef control;
    
    GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef,
	NULL, sizeof(ControlRef), NULL, &control );

    SelectItemOfTabControl(control);
    
    return eventNotHandledErr;
}

void InstallTabHandler(WindowRef window)
{
    EventTypeSpec	controlSpec = { kEventClassControl, kEventControlHit };
    ControlRef 		tabControl;
    ControlID 		controlID;
    int i;

    controlID.signature = TAB_SIGNATURE;

    for(i = 0;i < 4;i++) {
       controlID.id = tabList[i];
       GetControlByID(window, &controlID, &tabControl);
       DisableControl(tabControl);
       SetControlVisibility(tabControl, false, false);
    }

    controlID.id = TAB_ID;
    GetControlByID(window, &controlID, &tabControl);

    InstallControlEventHandler(tabControl,
                        NewEventHandlerUPP( TabEventHandler ),
                        1, &controlSpec, 0, NULL);

    SetControl32BitValue(tabControl, 1);

    SelectItemOfTabControl(tabControl); 
}

CFStringRef get_settings(WindowRef window, int i) {
	ControlID id;
	ControlRef control;
	CFStringRef s;

	id.signature = 'conf';
	id.id = i;
	GetControlByID(window, &id, &control);
	GetControlData(control, kControlEditTextPart,
		kControlEditTextCFStringTag, sizeof(CFStringRef), &s, NULL);

	return s;
}

CFStringRef get_settings_c(WindowRef window, int i) {
	ControlID id;
	ControlRef control;
	CFStringRef s;

	id.signature = 'conf';
	id.id = i;
	GetControlByID(window, &id, &control);
	s = CFStringCreateWithFormat(kCFAllocatorDefault, NULL,
		CFSTR("%d"), GetControl32BitValue(control));

	return s;
}

void set_settings(WindowRef window, int i, CFStringRef s) {
	ControlID id;
	ControlRef control;

	if (s) {
		id.signature = 'conf';
		id.id = i;
		GetControlByID(window, &id, &control);
		SetControlData(control, kControlEditTextPart,
			kControlEditTextCFStringTag, sizeof(CFStringRef), &s);
	}
}

void set_settings_c(WindowRef window, int i, CFStringRef s) {
	ControlID id;
	ControlRef control;

	if (s) {
		id.signature = 'conf';
		id.id = i;
		GetControlByID(window, &id, &control);
		SetControl32BitValue(control, CFStringGetIntValue(s));
	}
}

void save_settings(WindowRef window) {
	int i;
	CFStringRef s;

	CFPreferencesSetAppValue(CFSTR("BiosPath"), get_settings(window, 1),
		kCFPreferencesCurrentApplication);
	CFPreferencesSetAppValue(CFSTR("CDROMDrive"), get_settings(window, 2),
		kCFPreferencesCurrentApplication);
	CFPreferencesSetAppValue(CFSTR("CDROMCore"), get_settings_c(window, 3),
		kCFPreferencesCurrentApplication);
	CFPreferencesSetAppValue(CFSTR("Region"), get_settings_c(window, 4),
		kCFPreferencesCurrentApplication);
	CFPreferencesSetAppValue(CFSTR("VideoCore"), get_settings_c(window, 5),
		kCFPreferencesCurrentApplication);
	CFPreferencesSetAppValue(CFSTR("SoundCore"), get_settings_c(window, 6),
		kCFPreferencesCurrentApplication);
	CFPreferencesSetAppValue(CFSTR("CartPath"), get_settings(window, 7),
		kCFPreferencesCurrentApplication);
	CFPreferencesSetAppValue(CFSTR("CartType"), get_settings_c(window, 8),
		kCFPreferencesCurrentApplication);
	CFPreferencesSetAppValue(CFSTR("BackupRamPath"),
		get_settings(window, 9), kCFPreferencesCurrentApplication);
	CFPreferencesSetAppValue(CFSTR("MpegRomPath"),
		get_settings(window, 10), kCFPreferencesCurrentApplication);

	i = 0;
	while(key_names[i]) {
		s = get_settings(window, 31 + i);
		CFPreferencesSetAppValue(
			CFStringCreateWithCString(0, key_names[i], 0),
			s, kCFPreferencesCurrentApplication);
		PerSetKey(CFStringGetIntValue(s), key_names[i]);
		i++;
	}

	CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
}

void load_settings(WindowRef window) {
	int i;

	set_settings(window, 1, CFPreferencesCopyAppValue(CFSTR("BiosPath"),
		kCFPreferencesCurrentApplication));
	set_settings(window, 2, CFPreferencesCopyAppValue(CFSTR("CDROMDrive"),
		kCFPreferencesCurrentApplication));
	set_settings_c(window, 3, CFPreferencesCopyAppValue(CFSTR("CDROMCore"),
		kCFPreferencesCurrentApplication));
	set_settings_c(window, 4, CFPreferencesCopyAppValue(CFSTR("Region"),
		kCFPreferencesCurrentApplication));
	set_settings_c(window, 5, CFPreferencesCopyAppValue(CFSTR("VideoCore"),
		kCFPreferencesCurrentApplication));
	set_settings_c(window, 6, CFPreferencesCopyAppValue(CFSTR("SoundCore"),
		kCFPreferencesCurrentApplication));
	set_settings(window, 7, CFPreferencesCopyAppValue(CFSTR("CartPath"),
		kCFPreferencesCurrentApplication));
	set_settings_c(window, 8, CFPreferencesCopyAppValue(CFSTR("CartType"),
		kCFPreferencesCurrentApplication));
	set_settings(window, 9,
		CFPreferencesCopyAppValue(CFSTR("BackupRamPath"),
		kCFPreferencesCurrentApplication));
	set_settings(window, 10, CFPreferencesCopyAppValue(CFSTR("MpegRomPath"),
		kCFPreferencesCurrentApplication));

	i = 0;
	while(key_names[i]) {
		set_settings(window, 31 + i, CFPreferencesCopyAppValue(
			CFStringCreateWithCString(0, key_names[i], 0),
			kCFPreferencesCurrentApplication));
		i++;
	}
}

OSStatus SettingsWindowEventHandler (EventHandlerCallRef myHandler, EventRef theEvent, void* userData)
{
  OSStatus result = eventNotHandledErr;

  switch (GetEventKind (theEvent))
    {
    case kEventWindowClose:
      {
	WindowRef window;
        GetEventParameter(theEvent, kEventParamDirectObject, typeWindowRef,
	  0, sizeof(typeWindowRef), 0, &window);

	save_settings(window);

        DisposeWindow(window);
      }
      result = noErr;
      break;

    }
 
  return (result);
}

OSStatus KeyConfigHandler(EventHandlerCallRef h, EventRef event, void* data) {
	UInt32 key;
	CFStringRef s;
	Str255 tmp;
        GetEventParameter(event, kEventParamKeyCode,
		typeUInt32, NULL, sizeof(UInt32), NULL, &key);
	NumToString(key, tmp);
	s = CFStringCreateWithPascalString(0, tmp, 0);
	SetControlData(data, kControlEditTextPart,
		kControlEditTextCFStringTag, sizeof(CFStringRef), &s);
	Draw1Control(data);

	return noErr;
}

WindowRef CreateSettingsWindow() {

  WindowRef myWindow;
  IBNibRef nib;

  EventTypeSpec eventList[] = {
    { kEventClassWindow, kEventWindowClose }
  };

  CreateNibReference(CFSTR("preferences"), &nib);
  CreateWindowFromNib(nib, CFSTR("Dialog"), &myWindow);

  load_settings(myWindow);

  InstallTabHandler(myWindow);

  {
    int i;
    ControlRef control;
    ControlID id;
    EventTypeSpec elist[] = {
      { kEventClassKeyboard, kEventRawKeyDown },
      { kEventClassKeyboard, kEventRawKeyUp }
    };

    id.signature = 'conf';
    i = 0;
    while(key_names[i]) {
      id.id = 31 + i;
      GetControlByID(myWindow, &id, &control);

      InstallControlEventHandler(control, NewEventHandlerUPP(KeyConfigHandler),
	GetEventTypeCount(elist), elist, control, NULL);
      i++;
    }
  }

  ShowWindow(myWindow);

  InstallWindowEventHandler(myWindow,
			    NewEventHandlerUPP (SettingsWindowEventHandler),
			    GetEventTypeCount(eventList),
			    eventList, myWindow, NULL);

  return myWindow;
}