WaitKey v1.03 - Wait for arbitrary keys to be pressed and/or released

WaitKey is a small utility that will wait for any of a number of
user-supplied keystroke lists to be pressed or released.  It is designed to
be used inside batch files that run in the background.  For example, you
could have a batch file using WaitKey to run a script when the user presses
Control+Shift+F7 while using a different program.

Usage:   WaitKey.exe [-arb] keys [keys [keys ...]]

Options: -r  Wait for the keys to be released rather than pressed.
         -b  Wait for the keys to be both pressed and released.
         -a  Wait for any of the keys in a set, instead of all of them.

The keys can be any ASCII characters.  WaitKey will set the errorlevel to the
index of whichever set of keys was pressed/released first.

Also predefined are:
         \c, \s, \a, \w   (control, shift, alt, windows)
         \t, \n, \e  (tab, enter, escape)
         \l, \r, \u, \d  (arrow keys - left, right, up, down)
         \F#  Function key #, where # is a hex digit, 1-c
         \v## Predefined virtual key, where ## is a hex value, 00-ff

Example: WaitKey -r \c\s\F7


WaitKey is copyright (C) 2012 by Eric VanHeest (edv_ws@vanheest.org)

