TO-DO List            

### Code Refactoring
When I read the code of unreal engine4, I thought my engine's code readability is really horrible. I should fix this!      

### Read back results of culling one frame later         
        
### Support D3D11, OpenGL completely

### Implement Hierarchical Shadow map

### Batch draw    
Fix up draw order      

### Reduce header dependencies ( Reduce build time )

### Implement config value setting window

### DONE: clscan now handles paths with spaces
`clscan.dll` used to split its command line on spaces with no quote handling, so a
project under a path like `...\Graphics Engines and Projects\...` produced 127130
`no such file or directory` errors and 132 clScan failures.

Fixed by rebuilding clReflect against LLVM 12.0.1 with a quote-aware tokenizer in
`argvSplit` (clscan, clmerge and clexport all had the same copy-pasted bug), and
restoring the matching quoting in `clReflect_automation`. A doubled quote is treated
as a literal quote so the `-DVCXPROJ_PATH="""..."""` idiom still yields a valid string
literal.

Verified on a path containing spaces: clScan 277/277, zero errors, clMerge success.

To rebuild the tools again:
  * `extern/get.bat` says `release/11.x` but the code needs **12.x** -
    `Main.cpp` calls `clang::tooling::CommonOptionsParser::create()`, a factory
    added in LLVM 12.
  * Configure LLVM into `extern/llvm-build` with `-DLLVM_ENABLE_PROJECTS=clang
    -DLLVM_TARGETS_TO_BUILD=X86`, generator "Visual Studio 16 2019".
  * Build only what is needed: `clangTooling clangFrontend clangSema clangParse
    clangAnalysis clangLex clangBasic clangSerialization clangDriver clangAST
    clangEdit LLVMSupport LLVMCore LLVMMC LLVMX86Info LLVMX86Desc
    LLVMX86AsmParser LLVMX86CodeGen` - about 11 minutes on 16 cores.
  * Then build the `clscan`, `clmerge` and `clexport` targets (shared libraries)
    and copy the DLLs into `Doom3/x64/<Config>/`.

### clexport overruns its buffers on a large .map file
`clexport -map` crashes on the Debug build's `map.map`. `MapFileParser.cpp` parses
into fixed `char[1024]` buffers, but the longest line in a Debug map is 2875
characters - long template symbol names blow straight past them. The prebuilt
clexport had the same buffers and merely corrupted memory quietly; a Release build
turns that into a hard crash.

This is a pre-existing clReflect bug, unrelated to the quoting work.

Workaround: clexport succeeds without `-map` and still produces a valid .cppbin -
what is lost is function call addresses, a limitation clReflect already documents.

Fix properly by making MapFileParser size its buffers from the input (std::string
or a dynamically grown buffer) rather than assuming 1024 characters.

Note the separate, unrelated constraint: clscan uses clang 12.0.1, which cannot parse
the VS2022 runtime headers, so the engine must be built with VS2019 / v142 and
`VCToolsInstallDir` pointed at the VS2019 tools directory.
