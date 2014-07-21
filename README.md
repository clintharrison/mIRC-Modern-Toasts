mIRC-Modern-Toasts
==================
This DLL (and accompanying mIRC script, if so desired) brings Windows 8's modern user interface toast notifications to mIRC.

![Modern toast example](http://i.imgur.com/rnOEoFE.png)

## Building
* Requires Windows 8 SDK for C++/CX libraries
* Build with Visual Studio
* Hope and pray that it works, because those are the only steps I took

## Notes
The DLL attempts to find a shortcut with the same target as your mIRC executable to use its AppUserModelID.
If such a shortcut does not exist (or is otherwise not found), a shortcut will be automatically created.
This is a requirement enforced by Windows itself to show toast notifications from a Desktop application.

Additionally, included is a `mIRC.VisualElementsManifest.xml` file that will give mIRC's notifications a more pleasing gray background. Place this (along with the compiled DLL and the .mrc script) in the same directory as mIRC.exe. In order to effect these changes, you will need to update the last modified time of the shortcut. One of the ways this could be done is with the PowerShell command as follows:
```(ls "$env:AppData\Microsoft\Windows\Start Menu\Programs\mIRC.lnk").LastWriteTime = Get-Date```
Adjust this for the name of the shortcut on your system. This is only necessary if you do not permit the DLL to create a shortcut.

## Questions or Comments
Find me on Freenode under the nickname Clinteger.
