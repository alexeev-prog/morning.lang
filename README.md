# ⚡ morning.lang - The Next-Gen Systems Programming Language
<a id="readme-top"></a>

<div align="center">
  <img src="https://raw.githubusercontent.com/alexeev-prog/morning.lang/refs/heads/main/docs/logo.png" width="250" alt="MorningLang Logo">

  <h3>A high-performance language combining C++ power with LLVM19 optimization</h3>

  <div>
    <a href="https://alexeev-prog.github.io/morning.lang/">
      <img src="https://img.shields.io/badge/Documentation-Online-success?style=for-the-badge&logo=gitbook" alt="Docs">
    </a>
    <a href="https://github.com/alexeev-prog/morning.lang/blob/main/LICENSE">
      <img src="https://img.shields.io/badge/License-GPL_v3-blue?style=for-the-badge&logo=gnu" alt="License">
    </a>
    <a href="https://github.com/alexeev-prog/morning.lang/stargazers">
      <img src="https://img.shields.io/github/stars/alexeev-prog/morning.lang?style=for-the-badge&logo=github" alt="Stars">
    </a>
  </div>
</div>

<br>

<div align="center">
  <img src="https://img.shields.io/github/languages/top/alexeev-prog/morning.lang?style=flat&color=blueviolet" alt="Top Language">
  <img src="https://img.shields.io/github/languages/count/alexeev-prog/morning.lang?style=flat&color=success" alt="Language Count">
  <img src="https://img.shields.io/github/license/alexeev-prog/morning.lang?style=flat" alt="License">
  <img src="https://img.shields.io/github/issues/alexeev-prog/morning.lang?style=flat&color=critical" alt="Issues">
  <img src="https://img.shields.io/github/last-commit/alexeev-prog/morning.lang?style=flat&color=informational" alt="Last Commit">
  <img src="https://img.shields.io/github/contributors/alexeev-prog/morning.lang?style=flat&color=ff69b4" alt="Contributors">
</div>

<div align="center" style="margin: 15px 0">
  <img src="https://github.com/alexeev-prog/morning.lang/actions/workflows/static.yml/badge.svg" alt="Static Analysis">
  <img src="https://github.com/alexeev-prog/morning.lang/actions/workflows/ci.yml/badge.svg" alt="CI Build">
</div>

<div align="center">
  <img src="https://raw.githubusercontent.com/alexeev-prog/morning.lang/refs/heads/main/docs/pallet-0.png" width="600" alt="Color Palette">
</div>

---

Read our [SHORT GUIDELINES](./SHORT_GUIDELINES.md)

## Powering Performance with Modern Tech
Morning.lang combines the raw power of C++ with the cutting-edge optimization capabilities of **LLVM 19** to deliver exceptional performance:

- ⚡️ **LLVM19 Backend**: Leverages the latest optimizations from the LLVM project
- 🚀 **Near-Native Speed**: Compiles to highly optimized machine code
- 🔧 **Modern C++17**: Built with contemporary C++ features and best practices
- 🧠 **Smart Memory Management**: Automatic scope-based resource handling
- 🌉 **Seamless Interop**: Easily integrate with existing C/C++ libraries

## Why Developers Love Morning.lang
```mermaid
graph LR
A[Clean Syntax] --> B[Developer Productivity]
C[LLVM Optimization] --> D[Runtime Performance]
E[Memory Safety] --> F[Reliable Applications]
B --> G[Happy Developers]
D --> G
F --> G
```

### Key Advantages Over Traditional Languages
| Feature                | Benefit                                                                 |
|------------------------|-------------------------------------------------------------------------|
| ⚡ **LLVM19-Powered**  | Access to latest compiler optimizations and CPU features                |
| 🎨 **Expressive Syntax** | Write complex operations with minimal boilerplate                      |
| 🧩 **Modular Design**  | Extend with custom optimizers and language features                    |
| 🌐 **Cross-Platform**  | Single codebase compiles to Windows, Linux, macOS binaries             |
| 🔒 **Memory Safety**   | Scope-based ownership eliminates common memory errors                  |
| 🔄 **Metaprogramming** | Powerful compile-time code generation capabilities                     |

---

## ⚠️ Important Notes

> [!CAUTION]
> **Active Development Notice**
> Morning.lang is currently under active development. While core features are stable, some advanced capabilities are still being perfected. We welcome early adopters to help shape the language!

> [!NOTE]
> **Building & Installation**
> See the [BUILDING GUIDE](BUILDING.md) for detailed instructions on compiling from source.

> [!NOTE]
> **Want to Contribute?**
> Check out our [CONTRIBUTING GUIDELINES](CONTRIBUTING.md). We especially welcome LLVM and compiler experts!

> [!NOTE]
> **License**
> Morning.lang is licensed under [GNU GPL V3](./LICENSE) - Free to use, modify, and distribute

---

## 💡 Why Choose Morning.lang for Your Next Project?

### 1. Uncompromising Performance
Built on **LLVM 19**, Morning.lang delivers:
- Advanced optimization pipelines
- Architecture-specific code generation
- LTO (Link Time Optimization) support
- Auto-vectorization for SIMD operations

### 2. Modern Language Features
- Pattern matching
- Algebraic data types
- First-class functions
- Hygienic macros
- Custom operator definitions

### 3. Safety Without Sacrifice
- Ownership system prevents memory leaks
- Bounds checking (optional)
- Null safety by design
- Immutable-by-default variables
<!--
### 4. Seamless C/C++ Integration

```morning
// Easily call C functions
[extern "libm" sin (x !double) -> !double]

[func calculate (angle)
    (sin (* angle (/ 3.14159 180.0)))]
``` -->

---

## 💡 Language Highlights

### 🔄 Scoped Variables
```morning
[var [ALPHA !int] 42]  // Global variable

[scope
    [var [ALPHA !string] "Hello"]  // Local shadowing
    [fprint "Local ALPHA: %s\n" ALPHA]]

[fprint "Global ALPHA: %d\n" ALPHA]
```

### 🧩 Advanced Functions
```morning
// Higher-order function
[func apply (f x) (f x)]

[fprint "Result: %d" (apply increment 10)]
```
<!--
### 🔁 Concurrency Support
```morning
// Lightweight coroutines
[coroutine data_producer ()
    [loop
        [var data (fetch_data)]
        [yield data]]]

// Parallel execution
[parallel
    [task process_data data]
    [task log_metrics]]
``` -->

---

## 📚 Documentation & Learning Resources

Explore comprehensive documentation at:
[https://alexeev-prog.github.io/morning.lang/](https://alexeev-prog.github.io/morning.lang/)

### Learning Path:
1. **Language Tour**: Quick overview of syntax and features
2. **LLVM Integration**: How Morning.lang leverages LLVM internals
3. **Performance Guide**: Optimization techniques and benchmarks
4. **C++ Interop**: Working with existing C/C++ libraries
5. **Compiler Development**: Contributing to the language core

---

## 💻 Getting Started in 5 Minutes

### 1. Build from Source
```bash
# Clone repository (requires LLVM 19 development files)
git clone --recurse-submodules https://github.com/alexeev-prog/morning.lang.git
cd morning.lang

# build
./build.sh all
```

---

## 🏗️ Real-World Applications

Morning.lang excels in:
- **High-performance computing**
- **System utilities development**
- **Game engines**
- **Blockchain infrastructure**
- **Compiler development**
- **Embedded systems programming**
<!--
```mermaid
pie
    title Industry Applications
    “System Software” : 35
    “Game Development” : 25
    “Fintech” : 20
    “AI/ML” : 15
    “Embedded” : 5
``` -->

---

## 🤝 Join Our Growing Community

We're building Morning.lang for developers who care about performance, elegance, and control. Join us!

- **GitHub Discussions**: [Share ideas and ask questions](https://github.com/alexeev-prog/morning.lang/discussions)
- **Core Team Email**: `alexeev.dev@mail.ru` (Use "Morning.lang Contribution" in subject)
- **Weekly Development Streams**: Live coding sessions on Twitch
- **Contribution Guide**: [CONTRIBUTING.md](CONTRIBUTING.md)

---

## 📜 License
```text
Aesthetic programming language in C++ (llvm)
Copyright (C) 2025  Alexeev Bronislav

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```

<div align="center">
  <br>
  <a href="#readme-top">↑ Back to Top ↑</a>
  <br>
  <sub>Built with ❤️ and LLVM 19</sub>
</div>
