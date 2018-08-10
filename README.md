## About Flatton-Offline

Flatton-Offline is an utility that allows you to quickly "flatten" an inked comics page. You pass in a png image with line art and Flatton fills each white area with a random color. It then grows each area so that it bleeds under the line art, touching the other neighboring areas.

It is derived from the online utility which you can find [here](https://www.ayalpinkus.nl/Flatton.html).

The online version does not store images in files as it does the entire processing in memory.

This offline version *does* store the result in a file (locally on your computer).

The advantages of this offline version over the online version are:

- No need to send sensitive line art over the internet to a strange server. It is all processed locally on your computer.
- Images can be any size. The online version only allows images up to a specific size, both in pixels and in bytes. These limits are still specified in the code, but you can increase these limits if you want to. Just change the defines LIMIT_NR_PIXELS abd LIMIT_PNG_SIZE in flatton.cpp.
- In the future, different color schemes might be supported.

The disadvantage of this offline version is that it takes more effort to use: you have to compile it first for your computer.


## Tips

- Flatton will close opened lines if the opening is four pixels or less, but if lines are open in certain places, you can direct the flatting by closing lines first in the image you send, for best results. (You don't need to use the version with the closed lines for your final image of course.)
-  You can also open up crosshatched areas with a quick white line, to open up areas that should have the same color.
- Areas that are marked red were initially too small, and were then either grouped into areas that were too big to remove, or they had more than one neighbor so Flatton could not decide which area to group them with.
- Flatton occasionally returns "indexed" .PNG files for reduced file size. You may need to change the image to RGB mode before you can use it.

Currently only png is supported.


## Installing from the source code

Flatton-Offline comes in source code form. On Unix-like systems (Linux, Mac OS X and others), in the source directory, type:

    make

This should build the executable flatton.a, which can be used from the command line as follows:

    ./flatton.a [input png file] [output png file]

For example, there is an example01.png image file in the source directory, which you can flatten as follows:

    ./flatton.a example01.png output.png

This creates the file output.png, which should contain the flats.

If you leave out the file name for the output file, the utility will save the result to "flats.png". If you leave out the file name of the input file also, it will try to load the input line art from the file "lines.png".



## Using the pre-compiled Windows binaries


The repository contains two Windows executables: https://github.com/ayalpinkus/flatton-offline/blob/master/flatton32.exe and     https://github.com/ayalpinkus/flatton-offline/blob/master/flatton64.exe .

If you place the executables in a folder with a file "lines.png" that contains the input line art, and you double-click on the suitable executable, you should see a file called "flats.txt" that contains the flats, after the program completes. 

Alternatively, you can run it from a command prompt and choose file names.

Here's how you can open a command line prompt in Windows: https://www.digitalcitizen.life/7-ways-launch-command-prompt-windows-7-windows-8

you can do

    cd ..

to go to a previous directory, and

   cd ( SomeDirectoryName )

to go to some directory. If you have these executables in a directory, and a file lines.png, you can type

    flatton32.exe lines.png flats.png

and a file called flats.png should appear, with the flats.


## Contact

Please feel free to contact me at shortcomics@ayalpinkus.nl




## License

Flatton copyright 2017-2018 Ayal Pinkus (https://www.ayalpinkus.nl)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see https://www.gnu.org/licenses.

Flatton uses LodePNG, which is Copyright (c) 2005-2018 [Lode Vandevenne](https://lodev.org/lodepng/) .

