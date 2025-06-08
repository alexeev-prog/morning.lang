# ⚙️ morning.lang
<a id="readme-top"></a>

<div align="center">
  <p align="center">
  Aesthetic programming language in C++ (llvm)
    <br />
    <a href="https://alexeev-prog.github.io/morning.lang/"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    <a href="https://alexeev-prog.github.io/morning.lang/">Documentation</a>
    ·
    <a href="https://github.com/alexeev-prog/morning.lang/blob/main/LICENSE">License</a>
  </p>
</div>
<br>
<p align="center">
    <img src="https://img.shields.io/github/languages/top/alexeev-prog/morning.lang?style=for-the-badge">
    <img src="https://img.shields.io/github/languages/count/alexeev-prog/morning.lang?style=for-the-badge">
    <img src="https://img.shields.io/github/license/alexeev-prog/morning.lang?style=for-the-badge">
    <img src="https://img.shields.io/github/stars/alexeev-prog/morning.lang?style=for-the-badge">
    <img src="https://img.shields.io/github/issues/alexeev-prog/morning.lang?style=for-the-badge">
    <img src="https://img.shields.io/github/last-commit/alexeev-prog/morning.lang?style=for-the-badge">
    <img alt="GitHub contributors" src="https://img.shields.io/github/contributors/alexeev-prog/morning.lang?style=for-the-badge">
</p>

<div align='center'>
    <img src="https://github.com/alexeev-prog/morning.lang/actions/workflows/static.yml/badge.svg">
    <img src="https://github.com/alexeev-prog/morning.lang/actions/workflows/ci.yml/badge.svg">
</div>

<p align="center">
    <img src="https://raw.githubusercontent.com/alexeev-prog/morning.lang/refs/heads/main/docs/pallet-0.png">
</p>

<img src="https://raw.githubusercontent.com/alexeev-prog/morning.lang/refs/heads/main/docs/logo.png">

At the core of the MorningLang compiler lies a meticulously crafted hierarchy of LLVM components that orchestrate the translation from high-level language constructs to optimized machine code. The system's architecture revolves around three fundamental pillars that form the backbone of its code generation capabilities.

 > [!CAUTION]
 > At the moment, morning.lang is under active development, many things may not work, and this version is not recommended for use (all at your own risk)

 > [!NOTE]
 > Building and installion: See the [BUILDING](BUILDING.md) document.

 > [!NOTE]
 > Contributing: See the [CONTRIBUTING](CONTRIBUTING.md) document.

 > [!NOTE]
 > Licensing: [GNU GPL V3](./LICENSE)

Simple example:

```morning
[var [ALPHA !int] 42]

[scope
    [var [ALPHA !string] "Hello"]
    [fprint "ALPHA: %s\n" ALPHA]]

[fprint "ALPHA: %d\n" ALPHA]

[set ALPHA 100]

[fprint "ALPHA: %d\n" ALPHA]

[fprint "_VERSION: %d\n\n" _VERSION]
```

Advanced example:

```
[func square (x) (* x x)]

[func scoped_function (x) [scope
    [+ x 100]
    [while (> x 0)
        [scope
            [set x (- x 1)]
            [fprint "%d " x]]]
    ]
    [fprint "\n"]
]

[fprint "square 10: %d\n" (square 10)]
[fprint "square 0xA: %d\n" (square 0xA)]
[fprint "square 012: %d\n" (square 012)]
[fprint "square 0b1010: %d\n" (square 0b1010)]

[fprint "scoped_function 10: %d\n" (scoped_function 10)]

[var (a !int) 10]

[fprint "a: %d\n" a]

[check (== a 10)
    [set a 0]]

[fprint "a: %d\n" a]

[func sum ((first !int) (second !int)) -> !int (+ first second)]

[fprint "sum 100 1: %d\n\n" (sum 100 1)]
```

## Documentation
Online docs available on [this link](https://alexeev-prog.github.io/morning.lang/).

## Why Choose **Morning.lang**? 🚀

| 🌟 Feature                | 🔍 Why It Matters                                                      |
|---------------------------|------------------------------------------------------------------------|
| ⚡️ Fast & Lightweight     | Minimizes load times and maximizes runtime efficiency.                 |
| 🧩 Modular Architecture   | Enables easy extension, customization, and integration of new features.|
| 🎮 Vulkan API Integration | Leverages cutting-edge graphics API for maximum rendering performance. |
| 📚 Modern C++17 Codebase  | Clean, maintainable, and future-proof code for professional workflows. |
| 🌍 Cross-Platform         | Runs seamlessly on Windows, Linux, and macOS.                          |

## Why Use **Morning.lang**? ✨

- **Performance driven:** LLVM-based language, written in C++.
- **Extensible by design:** Modular, loosely coupled systems; add or replace features gracefully.
- **Modern standards:** C++17 compliance ensures clean, maintainable, future-proof code.
- **Cross-platform:** Write once, run on Windows, Linux, and macOS seamlessly.
- **Built-in key systems:** Sound, physics, map loading, events right out of the box.

## Examples

 + Conditions

```morning
[var b 100]

[var a (+ b 1)]

[check (== a 101)
    [check (> a 100)
        [set a 1000]
        [set a -1]]
    [set a 0]]

[fprint "A: %d\n\n" a]
```

 + Functions

```
[func square (x) (* x x)]

[fprint "square 10: %d\n" (square 10)]

[func sum ((first !int) (second !int)) <-> !int (+ first second)]

[fprint "sum 100 1: %d\n\n" (sum 100 1)]
```

 + While loop

```
[var a 10]

[while (> a 0)
    [scope
        [set a (- a 1)]
        [fprint "%d " a]]]

[fprint "\nA: %d\n\n" a]
```

 + Number systems

```
[func square (x) (* x x)]

[fprint "square 10: %d\n" (square 10)]
[fprint "square 0xA: %d\n" (square 0xA)]
[fprint "square 012: %d\n" (square 012)]
[fprint "square 0b1010: %d\n" (square 0b1010)]

[func sum ((first !int) (second !int)) -> !int (+ first second)]

[fprint "sum 100 1: %d\n\n" (sum 100 1)]
```

## Join the Community! 🌐

- **GitHub Issues** — for bug reporting and feature requests.
- **Contact Email** — for direct feedback, suggestions, and collaboration inquiries. (email: `alexeev.dev@mail.ru`. Please insert "Morning.lang Issue" at the beginning of the subject line.)

Thank you for exploring **Morning.lang** — let’s create extraordinary 3D experiences together! 🚀
