VideoTimeMeasure
================

Measure time interval in video

This application is intended to measure time in video clip. Motivation was to create application that can measure / verify air time and synchronization for trampolines.
Thanks to all people who created libraries, tutorials and tools used to create this application.

Use video from camcoders or hardware encoders. Software encoders can damage time information in video even running on strong hardware.

Usage:
1. Open video file
2. Select desired timestamp cell in table right to the video image.
3. Navigate to time using buttons below video.
4. Continue to next timestamp (Press enter on insert row and use mouse)

Requirements:
Qt 4.8
FFmpeg

Compilation:
Linux: qmake or qt creator
Windows: I do is using MXE cross compilation environment on linux

Sources:
https://github.com/jankolorenc/VideoTimeMeasure
