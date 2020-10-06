# New fonts system update

This New fonts update is aiming to make fonts customizable, with a blurred shadow and neon effect.
Customizable also mean that the HUD keep consistent if the font differ from the original one.


## Features

- Old fonts compatibility
- 3 gaussian blur level
- Emoticon shadow
- 6 default fonts instead of 3. News one are teams fonts (aliens and humans) and chat font. Others were remplaced with known legal ones.
- It also replace "Big Numbers" in huds. So counters are now using team's fonts.


## Usage

The synthax is `[font name] [font file] [font simple basename] [point size]`.

Font name is one of:
- `font`: The font used for most UI, like text in main menu or most of menus.
- `smallFont`: The font used for condensed text, like item's description.
- `bigFont`: The big font which is used for big text printing as spectator (as warmup for instance).
- `chatFont`: The font used for chatting. It may be less condensed as smallFont as Tremulous is not a chatting app.
- `alienFont`: Font dedicated for aliens things, mainly replacing big numbers (aka counters), print screen (which was bigFont), and some estetic things.
- `humanFont`: Same as alienFont, for humans.

Font file is the location of the original font file, from which font image will be generated.

Point size rougly define the optimized size of the font. You should leave it as is.

```
font "fonts/font" standard 26         // standard font
smallFont "fonts/smallfont" small 20  // small font (console mainly)
bigFont "fonts/bigfont" big 34        // big font
chatFont "fonts/chat" chat 26         // chat font
alienFont "fonts/alien" alien 34      // alien's font
humanFont "fonts/human" human 34      // human's font
```


## Generating new fonts

You must recompile tremulous with freetype library: `BUILD_FREETYPE=1 make`
You have to remove old fonts from `fonts` directory of the tremulous directory (including in assets).
Put your fonts where you defined it in UI (the `font file` argument), you should remove the extension from the file.
Launch Tremulous; fonts should be generated during launch. You should launch it from a terminal to get error output.
If it success you should found your generated fonts in `~/.tremulous/fonts`.

For more help, read the sourcecode: `src/renderercommon/tr_font.cpp`


## Generating emoticons shadow

Run `generate.sh` file located in `assets/emoticons/shadows` in its own directory. ImageMagick's `convert` is needed.

It will generate a shadow for all emoticons located in `assets/emoticons`.
Overlay doesn't got shadow for estetical choice.

## Old fonts compatibility

However, if you use old syntax, it will automatically use old fonts found in old assets.
```
font "fonts/font" 26
smallFont "fonts/smallfont" 20
bigFont "fonts/bigfont" 34
```