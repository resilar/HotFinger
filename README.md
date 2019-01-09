You know that insecure fingerprint reader next to your laptop keyboard? Now you
can turn it into something marginally useful by repurposing it as an application
launcher with **HotFinger** (like hot*key* but for *finger*s, get it?)

![Screenshot](https://i.imgur.com/VAwqHb0.png)

System requirements include Windows 10 operating system and a working
fingerprint reader.

[Download the latest version
here](https://github.com/resilar/HotFinger/releases/latest).
***TODO*** *Prepare a release package*


## Features

- Bind commands to fingerprints
  - Supported commands include ...
    - Launching an application (with or without arguments)
    - Opening directory in the Windows file browser
    - Opening URL in your favorite web browser
    - Whatever else
      [`ShellExecute()`](https://docs.microsoft.com/en-us/windows/desktop/api/shellapi/nf-shellapi-shellexecutea)
      can do
  - All **ten** fingers supported!
- Written in assembler
  - So it's blazingly fast
  - And crazy small (42KB including an 18KB icon)
- High DPI aware GUI (come on, it's 2019)
- Uninstaller that sucks less
  - *Clean* uninstalls
    - No config files left lying around
    - ... nor WinBio database files
    - ... nor registry settings
  - Trigger via the menu: `Options > Uninstall`
- Minimize to tray


## Requirements

- **Windows 10** \
  Windows 7 is unsupported because HotFinger uses a few DPI-aware WinAPI
  functions that were first added in Windows 10 (version 1607). Thus, starting
  HotFinger on Windows 7 gives an error message titled: *Entry Point Not Found*.
  However, implementing Windows 7 support is a straightforward task: simply call
  DPI-**un**aware versions of the WinAPI functions if DPI-aware versions are
  unavailable. If someone implements Windows 7 support
  [cleverly](https://docs.microsoft.com/en-us/windows/desktop/Dlls/using-run-time-dynamic-linking)
  (that is, without affecting the high-DPI scaling behavior on Windows 10), feel
  free to submit a pull request to this repository.

- **Fingerprint reader** \
  There is always a good chance that the program will not work because of some
  subtle difference in your specific fingerprint reader or associated driver(s).
  If so, [opening a new issue](https://github.com/resilar/HotFinger/issues/new)
  would be highly appreciated. Remember to include error messages and other
  debug information if possible.


## Geeks

Compile: `fasm hotfinger.asm` (download FASM for Windows
[here](https://flatassembler.net/download.php)).

Contributors please use the flat editor `FASMW.EXE` to format your code.


## Q&A

This is "Rants of an Old x86 Assembly Coder" section disguised as "Questions and
Answers" section. Stop reading. Nothing significant follows.


#### Why x86 assembly in ~~2018~~THE CURRENT YEAR?

Mostly as a demonstration to show that it is still possible to write GUI
applications for Windows without tons of bloated libraries and dependencies.
Also because x86 assembly felt like a good match for this particular project;
thanks to the nice 32-bit WinAPI interface of WinBio offered by Windows.
Ultimately, writing x86 assembly is not more difficult than writing C code, so
why the hell not?


#### Why FASM instead of MASM or NASM?

My unbiased objective opinion: FASM is the best assembler out there, hands down.
It is the only assembler that truly gives the programmer total byte-level
control of the output. This is largely because FASM produces the target
executable directly, bypassing an external linker program required by other
assemblers. FASM is fully self-hosting which makes it extremely fast compared to
assemblers written in higher-level languages such as C. Finally, FASM has the
most powerful macro system by far.

OS developers may write bootloaders and interrupt handlers in MASM or NASM,
whereas full operating systems are typically written in FASM (see
[DexOS](http://dex-os.github.io/), [KolibriOS](http://www.kolibrios.org/en/),
[MenuetOS](http://menuetos.net/), ...). In other words, FASM is the
assembler-of-choice for large x86 assembly projects, while shitty assemblers can
be bearable in small projects like single functions.

MASM syntax is braindead for obfuscating memory operands (e.g., `LEA` and
`offset` stupidity); for fuck's sake, do not hide memory references from the
programmer, ever. NASM is acceptable, although relying on an external linker
sucks. GAS and AT&T syntax are full retard.

See also: [flat assembler - Design Principles (or why flat assembler is
different)](https://flatassembler.net/docs.php?article=design).


#### Why not 64 bit?

All common x86-64 calling conventions have been designed primarily for
compilers, in a way that makes function calls unintuitive and painful when
hand-writing assembly. For example, all common x86-64 calling conventions apply
fastcall-style argument passing, which improves the performance compared to
stdcall on average, but makes the caller responsible for reserving registers for
function calls. Consequently, the poor assembly programmer has fewer registers
available to use without spilling. Compilers do not care, of course, because
register allocation boils down to the same graph coloring problem anyways.

Microsoft x64 calling convention also requires that the caller allocates a
*shadow space*, i.e., 32 bytes of space from the top of the stack before a
function call. The space is intended for saving 4 fastcall-registers in order to
simplify the compiler support for C/C++ functions
([source](https://msdn.microsoft.com/en-us/library/ms235286.aspx)). For some
insane reason, the 32-byte shadow space is required even if the callee function
takes fewer than 4 fastcall arguments. Moreover, the fifth and subsequent
arguments are pushed onto the stack **on the top of the shadow space**, which
complicates the mess even further. As a result, the calling convention is very
cumbersome for humans writing x86-64 assembly by hand (macros can help a bit).

System V AMD64 ABI does not suffer from the shadow space madness. However, it
reserves `RSI` and `RDI` registers for passing fastcall arguments. This is a bad
design decision since `RSI` and `RDI` are treated as special *source* and
*destination* registers in the x86-64 instruction set. For example, the usage of
x86-64 string instructions (e.g., `LODSB`, `STOSW`, `CMPSD`) becomes more
annoying in some situations as a consequence.

In comparison, Win32 stdcall calling convention feels luxuriously easy and
intuitive from the perspective of an x86 programmer. Additionally, the
stack-based argument passing of stdcall often produces more compact code (for
example, the encoding of `MOV ECX, 42` is 5 bytes long, whereas `PUSH 42` takes
only 2 bytes). Shorter instruction encodings can yield significant performance
improvements due to potentially more efficient cache utilization.


#### Why no `invoke` or control-flow macros?

Macros are for pussies.
