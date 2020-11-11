# SR Human Interface Guidelines

This is a basic document describing how to create a user friendly experience
when working on software with heavy use of GUI elements.

## Asset Checklist

### UI

- Icons should be made at 16px, 32px, 64px, and 128px sizes to accommodate various screen densities.
- Menu Items should use standard names, with a Title Capitalization scheme.
  - "File", "Edit", "View", "Window"
  - Use ellipses "..." if the menu item will require further user input.
    - If a dialog box is opened make the name of the dialog box match the name of the menu item.

### Interaction

- If more than one of the same object can be selected be sure to support multi-object editing.
- All controls should be accessible through keyboard (tab / arrow keys).
- Do not nest scroll views of the same direction in a window as it adds ambiguity and frustration when scrolling.
- If the software has distinct "Modes" / "Tools" make it clear which is selected to the user and make sure switching is instant.

### Feedback

- Be use to give users correct "Error" and "Warnings". Extra "Info" like logs should not be shown to the user as it just adds noise.
  - If the "Error" or "Warning" is fixable by the user provide steps for them to solve the problem.
- Do not spam messages, prefer a popup if the program absolutely cannot proceed without user intervention.
- Tooltips should be provided in as many places as possible.
  - Use complete sentences with correct grammar + punctuation.
  - Should be brief, 3 sentences or less.
  - Are not a replacement for formal documentation.
- For long running tasks display a progress bar.



----------------- MISC -------------------

UI Dialog Boxes: MacOS has the 'OK' / 'Confirm' on the right while Windows on the left.
