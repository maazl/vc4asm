# vc4asm - Macro assembler for Broadcom VideoCore IV aka Raspberry Pi GPU

The goal of the vc4asm project is a full featured macro assembler and disassembler with constraint checking.
The work is based on <a href="https://github.com/jetpacapp/qpu-asm">qpu-asm from Pete Warden</a> which itself is based on <a href="https://github.com/elorimer/rpi-playground/tree/master/QPU/assembler">Eman's work</a> and some ideas also taken from <a href="https://github.com/hermanhermitage/videocoreiv-qpu">Herman H Hermitage</a>.
But it goes further by far. First of all it supports macros and functions.

Unfortunately this area is highly undocumented in the public domain. And so the work uses only the code from <a href="https://github.com/raspberrypi/userland/tree/master/host_applications/linux/apps/hello_pi/hello_fft">hello_fft</a> which is available as source and binary as Rosetta Stone. However, I try to keep it downwardly compatible to the Broadcom tools.

<a href="http://www.maazl.de/project/vc4asm/doc/index.html#download">&rarr; Homepage & Download</a>

<a href="http://www.maazl.de/project/vc4asm/doc/changelog.html">&rarr; Changelog</a>

<a href="http://www.maazl.de/project/vc4asm/doc/index.html#vc4asm">&rarr; Assembler</a>

<a href="http://www.maazl.de/project/vc4asm/doc/index.html#vc4dis">&rarr; Disassembler</a>

<a href="http://www.maazl.de/project/vc4asm/doc/index.html#bugs">&rarr; Known problems</a>

<a href="http://www.maazl.de/project/vc4asm/doc/index.html#build">&rarr; Build instructions</a>

<a href="http://www.maazl.de/project/vc4asm/doc/index.html#sample">&rarr; Examples</a>
