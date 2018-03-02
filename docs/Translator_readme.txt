Translator plugin v.0.1.2.4
Copyright © 2006-2009 Cristian Libotean

This plugin will try to find all translatable strings in Miranda. All you need to do is use Miranda and it will discover
translatable strings as they are being requested.

It will store the strings in a folder called Translations (you can configure this using folders plugin).

Some notes on Transations mode:
  This mode will consume large quantities of memory as it has to keep both the english text and the translation for all strings in memory - use only if you plan to do translations for Miranda.
  When in translations mode the plugin will read the current language pack on load.
    When reading strings from the language pack the module they belong to cannot be determined and it will be left blank.
    If a module later requests one of the strings read from the language pack which currently has no module set the entry will be assigned to that module.
    If you modify a string that currently has no module set (it was read from the language pack) it will be assigned a default module: 'language pack - modified'.
    If you've modified a string that currently doesn't belong to any module and afterwards a plugin requests that exact same string the entry will be assigned to that plugin and the changes you've made will be kept.
  If in translations mode the plugin will also read previous translations (from the current translations folder) after it has read the language pack file.
    Entries that exist in the current translation folder take precedence over entries in the language pack file.
    Entries read from the module 'language pack - modified' will be treated as not having a module defined.
  Reading of the language pack and previous translation files is done in another thread.
    You should not close miranda until the thread finishes.
  If you enable Translations mode and set the plugin to flush on exit it may take a while for all the strings to get flushed to disk so Miranda might be slow to exit.

Changes:

+ : new feature
* : changed
! : bufgix
- : feature removed or disabled because of pending bugs

version 0.1.2.4 - 2011/08/23
	+ made x64 version updater aware

version 0.1.2.3 - 2009/11/16
	+ x64 support (not tested !!)

version 0.1.2.2 - 2008/01/24
	* Changed beta versions server.

version 0.1.2.1 - 2007/12/21
	+ Allow multiple selections in Translation dialog - only useful for copy pasting.

version 0.1.2.0 - 2007/12/13
	+ Entries in the translation dialog can now be copy pasted (in translation format)

version 0.1.1.4 - 2007/12/10
	! Fixed "Filter" button position when resizing

version 0.1.1.3 - 2007/11/05
	+ Added option not to filter entries automatically
	+ Added option to customize the automatic filter timeout - the entries will not be filtered until the timeout - this will make the window more responsive.

version 0.1.1.2 - 2007/11/01
	* Make Translator more responsive when filtering translation strings.

version 0.1.1.1 - 2007/04/23
	* Increased size of buffers used for language pack export.

version 0.1.1.0 - 2007/04/11
	* Some internal code changes.
	* Merge known plugins with plugins found in the language pack file.
	+ Show popups when reading of old entries begins and when it completes.
	! Ignore only real duplicates when building language pack (both the english text and the translation are the same).

version 0.1.0.3 - 2007/04/06
	* When building language packs double entries are ignored.
	* Changed 'Last-Modified-Using' entry when building language packs to include Miranda version.

version 0.1.0.2 - 2007/03/13
	! Fix possible crash on quick sort.
	! Oops, forgot some debug code in.

version 0.1.0.1 - 2007/03/09
	! Fix for very big strings.

version 0.1.0.0 - 2007/03/09
	! Fix a crash that can be generated sometimes by multiple threads adding translations at the same time.
	* Use rich edit control instead of edit for translations (so Ctrl + Backspace and other hotkeys work :) )
	+ Backup the translation file when a change is performed instead of just deleting the file.
	+ Option not to add untranslated strings when building language pack.
	+ Added filtering option to translation manager dialog.
	+ Added main menu item to open translations manager (if enabled).
	+ Added icon.

version 0.0.2.1 - 2007/03/07
	* Use binary search and quick sort methods - performance should increase a bit.

version 0.0.2.0 - 2007/03/07
	Added UUID ( {2fb94425-14a7-4af7-b3d9-43fdf6cf287d} )
	Added TRANSLATOR interface.

version 0.0.1.2 - 2007/03/01
	+ Added Unicode aware flag.

version 0.0.1.1 - 2007/01/31
	* Changed beta URL.

version 0.0.1.0 - 2007/01/29
	* Dynamic translation mode will now escape characters.
	* Dynamic translation mode will now work for ANSI strings.
	* Bigger memory requirements of course :)
	+ Added Wide flag to translation entries.

version 0.0.0.14 - 2007/01/08
	* Requires at least Miranda 0.6.

version 0.0.0.13 - 2007/01/07
	! Close thread handle.
	+ New version resource file.

version 0.0.0.12 - 2006/12/21
	* Workaround for Win9x users.
	! Fix for NULL translation strings.

version 0.0.0.11 - 2006/12/19
	+ Dynamic translation of strings.
		The plugin will override miranda's Translation() function and will try to provide the latest translation available.
		This will NOT work for plugins that request ANSI strings, it will only work for WIDE strings, menus and windows; no characters will be escaped in this mode.
		Restart required.
	+ Ignore double translations.
		This mode will try to detect strings that have been translated already and a translation is requested again.
		This will not work correctly with core modules that bypass the Translate() call and will only detect the already translated text for some of those calls.
		When activated the plugin will no longer detect multiple modules (plugins) that translate the same string, each entry will only appear once,
		for one plugin provided the english text and the translation are the same (if the english text and translation are different from one another then the string
		will be recorded).
		It might also slow your Miranda by a bit because it has to check all strings already present in the database.
		Restart required.

version 0.0.0.10 - 2006/12/08
	! Use the language pack code page to convert strings.
	! Translations window is resizable again.
	+ Can select custom code page used for conversions between wide and ansi strings.

version 0.0.0.9 - 2006/12/07
	* Changed controls tab order
	+ Can export language pack.

version 0.0.0.8 - 2006/11/26 - 12/05
	+ Added dll version info.
	* Translate Dialog and Menu strings
	+ Added Translations option
		Only use the option on an Unicode aware OS !!
		Enables the translations windows where you can manage translations on the fly.
		Added ability to save changes made on the fly to the output files.
	+ Resizable dialog.
	+ Detect changes & ability to autosave.
	! Fix for output files - they now use UTF-16LE
	* Can flush to disk even in Translations mode.
	! Don't read BOM if present
	+ Read language pack on load.
	+ Read previous translation files on load.
	* Added option to flush or not on exit.

version 0.0.0.7 - 2006/09/25
	+ Added option to flush to disk on demand.
	+ Show status info in options dialog.

version 0.0.0.6 - 2006/09/24
	* Rebased dll (0x2F700000)
	+ Updater support (beta versions)

version 0.0.0.5 - 2006/09/22
	* Created options dialog
	+ Added option to show only untranslated strings (thanks FREAK_THEMIGHTY for suggesting it)
		Filtering works from that point onwards, old strings will still remain in the files.
	+ Allow users to customize the number of strings to keep in memory before a flush to disk.
		Try not to use a big value, 150-200 should be enough and it won't have a very big impact on memory.
	+ Use custom functions to write to disk - now uses same format as language pack files.

version 0.0.0.4 - 2006/09/20
	* Speed improvements (memory requirements have gone up though :) )

version 0.0.0.3 - 2006/09/19
	+ Folders support
	* Every plugin has it's own file.

version 0.0.0.2 - 2006/09/18
	! Don't trim white spaces
	+ Show non printable characters (\n, \r, \t)
	+ Always write unicode strings.
	* Plugin now prints <"EnglishText">=<"Translation">
	+ Ability to list dialog strings (translation is not available)
	+ Ability to list menu strings (translation is not available)
	* Changed output file to "translation strings.txt"

version 0.0.0.1 - 2006/09/17
	+ First release
